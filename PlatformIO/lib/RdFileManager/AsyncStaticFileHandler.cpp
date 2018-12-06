/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified to support SD and SPIFFS file systems.
  Rob Dobson
  2018

*/

#include "ESPAsyncWebServer.h"
#include "AsyncStaticFileHandler.h"

AsyncStaticFileHandler::AsyncStaticFileHandler(const char* uri, const char* path, const char* cache_control)
  : _uri(uri), _path(path), _default_file("index.htm"), _cache_control(cache_control), _last_modified(""), _callback(nullptr)
{
  // Ensure leading '/'
  if (_uri.length() == 0 || _uri[0] != '/') _uri = "/" + _uri;
  if (_path.length() == 0 || _path[0] != '/') _path = "/" + _path;

  // If path ends with '/' we assume a hint that this is a directory to improve performance.
  // However - if it does not end with '/' we, can't assume a file, path can still be a directory.
  _isDir = _path[_path.length()-1] == '/';

  // Remove the trailing '/' so we can handle default file
  // Notice that root will be "" not "/"
  if (_uri[_uri.length()-1] == '/') _uri = _uri.substring(0, _uri.length()-1);
  if (_path[_path.length()-1] == '/') _path = _path.substring(0, _path.length()-1);

  // Reset stats
  _gzipFirst = false;
  _gzipStats = 0xF8;
  _foundFileName = "";
}

AsyncStaticFileHandler& AsyncStaticFileHandler::setIsDir(bool isDir){
  _isDir = isDir;
  return *this;
}

AsyncStaticFileHandler& AsyncStaticFileHandler::setDefaultFile(const char* filename){
  _default_file = String(filename);
  return *this;
}

AsyncStaticFileHandler& AsyncStaticFileHandler::setCacheControl(const char* cache_control){
  _cache_control = String(cache_control);
  return *this;
}

AsyncStaticFileHandler& AsyncStaticFileHandler::setLastModified(const char* last_modified){
  _last_modified = String(last_modified);
  return *this;
}

AsyncStaticFileHandler& AsyncStaticFileHandler::setLastModified(struct tm* last_modified){
  char result[30];
  strftime (result,30,"%a, %d %b %Y %H:%M:%S %Z", last_modified);
  return setLastModified((const char *)result);
}

#ifdef ESP8266
AsyncStaticFileHandler& AsyncStaticFileHandler::setLastModified(time_t last_modified){
  return setLastModified((struct tm *)gmtime(&last_modified));
}

AsyncStaticFileHandler& AsyncStaticFileHandler::setLastModified(){
  time_t last_modified;
  if(time(&last_modified) == 0) //time is not yet set
    return *this;
  return setLastModified(last_modified);
}
#endif
bool AsyncStaticFileHandler::canHandle(AsyncWebServerRequest *request){
  if(request->method() != HTTP_GET 
    || !request->url().startsWith(_uri) 
    || !request->isExpectedRequestedConnType(RCT_DEFAULT, RCT_HTTP)
  ){
    return false;
  }
  if (_getFile(request)) {
    // We interested in "If-Modified-Since" header to check if file was modified
    if (_last_modified.length())
      request->addInterestingHeader("If-Modified-Since");

    if(_cache_control.length())
      request->addInterestingHeader("If-None-Match");

    DEBUGF("[AsyncStaticFileHandler::canHandle] TRUE\n");
    return true;
  }
  return false;
}

bool AsyncStaticFileHandler::_getFile(AsyncWebServerRequest *request)
{
  // Remove the found uri
  String path = request->url().substring(_uri.length());

  // We can skip the file check and look for default if request is to the root of a directory or that request path ends with '/'
  bool canSkipFileCheck = (_isDir && path.length() == 0) || (path.length() && path[path.length()-1] == '/');

  path = _path + path;

  // Do we have a file or .gz file
  if (!canSkipFileCheck && _fileExists(request, path))
    return true;

  // Can't handle if not default file
  if (_default_file.length() == 0)
    return false;

  // Try to add default file, ensure there is a trailing '/' ot the path.
  if (path.length() == 0 || path[path.length()-1] != '/')
    path += "/";
  path += _default_file;

  return _fileExists(request, path);
}

#ifdef ESP32
#define FILE_IS_REAL(f) (f == true && !f.isDirectory())
#else
#define FILE_IS_REAL(f) (f == true)
#endif

bool AsyncStaticFileHandler::existsAndIsAFile(const String& fileName)
{
    struct stat st;
    if ((stat(fileName.c_str(), &st) != 0) || !S_ISREG(st.st_mode))
        return false;
    return true;
}

size_t AsyncStaticFileHandler::fileSizeInBytes(const String& fileName)
{
    struct stat st;
    if ((stat(fileName.c_str(), &st) != 0) || !S_ISREG(st.st_mode))
        return 0;
    return st.st_size;
}

bool AsyncStaticFileHandler::_fileExists(AsyncWebServerRequest *request, const String& path)
{
  bool fileFound = false;
  bool gzipFound = false;

  String gzip = path + ".gz";

  if (_gzipFirst) {
    gzipFound = existsAndIsAFile(gzip);
    if (!gzipFound){
      fileFound = existsAndIsAFile(path);
    }
  } else {
    fileFound = existsAndIsAFile(path);
    if (!fileFound){
      gzipFound = existsAndIsAFile(gzip);
    }
  }

  _foundFileName = gzipFound ? gzip : path;

  bool found = fileFound || gzipFound;

  if (found) {
    // Extract the file name from the path and keep it in _tempObject
    size_t pathLen = path.length();
    char * _tempPath = (char*)malloc(pathLen+1);
    snprintf(_tempPath, pathLen+1, "%s", path.c_str());
    request->_tempObject = (void*)_tempPath;

    // Calculate gzip statistic
    _gzipStats = (_gzipStats << 1) + (gzipFound ? 1 : 0);
    if (_gzipStats == 0x00) _gzipFirst = false; // All files are not gzip
    else if (_gzipStats == 0xFF) _gzipFirst = true; // All files are gzip
    else _gzipFirst = _countBits(_gzipStats) > 4; // IF we have more gzip files - try gzip first
  }

  return found;
}

uint8_t AsyncStaticFileHandler::_countBits(const uint8_t value) const
{
  uint8_t w = value;
  uint8_t n;
  for (n=0; w!=0; n++) w&=w-1;
  return n;
}

void AsyncStaticFileHandler::handleRequest(AsyncWebServerRequest *request)
{
  // Get the filename from request->_tempObject and free it
  String filename = String((char*)request->_tempObject);
  free(request->_tempObject);
  request->_tempObject = NULL;
 
  if((_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str()))
      return request->requestAuthentication();

  size_t fileSize = fileSizeInBytes(filename);
  if (fileSize > 0) {
    String etag = String(fileSize);
    if (_last_modified.length() && _last_modified == request->header("If-Modified-Since")) {
      request->send(304); // Not modified
    } else if (_cache_control.length() && request->hasHeader("If-None-Match") && request->header("If-None-Match").equals(etag)) {
      AsyncWebServerResponse * response = new AsyncBasicResponse(304); // Not modified
      response->addHeader("Cache-Control", _cache_control);
      response->addHeader("ETag", etag);
      request->send(response);
    } else {
      String origUrl = request->url().substring(_uri.length());
      AsyncWebServerResponse * response = new AsyncStaticFileResponse(_foundFileName, filename, String(), false, _callback);
      if (_last_modified.length())
        response->addHeader("Last-Modified", _last_modified);
      if (_cache_control.length()){
        response->addHeader("Cache-Control", _cache_control);
        response->addHeader("ETag", etag);
      }
      request->send(response);
    }
  } else {
    request->send(404);
  }
}

/*
 * File Response
 * */

AsyncStaticFileResponse::~AsyncStaticFileResponse(){
  if(_pFile)
    fclose(_pFile);
  _pFile = NULL;
}

void AsyncStaticFileResponse::_setContentType(const String& path){
  if (path.endsWith(".html")) _contentType = "text/html";
  else if (path.endsWith(".htm")) _contentType = "text/html";
  else if (path.endsWith(".css")) _contentType = "text/css";
  else if (path.endsWith(".json")) _contentType = "text/json";
  else if (path.endsWith(".js")) _contentType = "application/javascript";
  else if (path.endsWith(".png")) _contentType = "image/png";
  else if (path.endsWith(".gif")) _contentType = "image/gif";
  else if (path.endsWith(".jpg")) _contentType = "image/jpeg";
  else if (path.endsWith(".ico")) _contentType = "image/x-icon";
  else if (path.endsWith(".svg")) _contentType = "image/svg+xml";
  else if (path.endsWith(".eot")) _contentType = "font/eot";
  else if (path.endsWith(".woff")) _contentType = "font/woff";
  else if (path.endsWith(".woff2")) _contentType = "font/woff2";
  else if (path.endsWith(".ttf")) _contentType = "font/ttf";
  else if (path.endsWith(".xml")) _contentType = "text/xml";
  else if (path.endsWith(".pdf")) _contentType = "application/pdf";
  else if (path.endsWith(".zip")) _contentType = "application/zip";
  else if(path.endsWith(".gz")) _contentType = "application/x-gzip";
  else _contentType = "text/plain";
}

// AsyncStaticFileResponse::AsyncStaticFileResponse(FS &fs, const String& path, const String& contentType, bool download, AwsTemplateProcessor callback): AsyncAbstractResponse(callback){
//   _code = 200;
//   _path = path;

//   if(!download && !fs.exists(_path) && fs.exists(_path+".gz")){
//     _path = _path+".gz";
//     addHeader("Content-Encoding", "gzip");
//     _callback = nullptr; // Unable to process zipped templates
//     _sendContentLength = true;
//     _chunked = false;
//   }

//   _content = fs.open(_path, "r");
//   _contentLength = _content.size();

//   if(contentType == "")
//     _setContentType(path);
//   else
//     _contentType = contentType;

//   int filenameStart = path.lastIndexOf('/') + 1;
//   char buf[26+path.length()-filenameStart];
//   char* filename = (char*)path.c_str() + filenameStart;

//   if(download) {
//     // set filename and force download
//     snprintf(buf, sizeof (buf), "attachment; filename=\"%s\"", filename);
//   } else {
//     // set filename and force rendering
//     snprintf(buf, sizeof (buf), "inline; filename=\"%s\"", filename);
//   }
//   addHeader("Content-Disposition", buf);
// }

AsyncStaticFileResponse::AsyncStaticFileResponse(const String& foundFileName, const String& path, const String& contentType, bool download, AwsTemplateProcessor callback): AsyncAbstractResponse(callback){
  _code = 200;
  _path = path;
  _pFile = NULL;

  if(!download && foundFileName.endsWith(".gz") && !path.endsWith(".gz")){
    addHeader("Content-Encoding", "gzip");
    _callback = nullptr; // Unable to process gzipped templates
    _sendContentLength = true;
    _chunked = false;
  }

  _contentLength = AsyncStaticFileHandler::fileSizeInBytes(_path);
  _pFile = fopen(_path.c_str(), "rb");

  if(contentType == "")
    _setContentType(path);
  else
    _contentType = contentType;

  int filenameStart = path.lastIndexOf('/') + 1;
  char buf[26+path.length()-filenameStart];
  char* filename = (char*)path.c_str() + filenameStart;

  if(download) {
    snprintf(buf, sizeof (buf), "attachment; filename=\"%s\"", filename);
  } else {
    snprintf(buf, sizeof (buf), "inline; filename=\"%s\"", filename);
  }
  addHeader("Content-Disposition", buf);
}

size_t AsyncStaticFileResponse::_fillBuffer(uint8_t *data, size_t len){
    if (!_pFile)
        return 0;
  return fread(data, 1, len, _pFile);
}

