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

#pragma once

#include "stddef.h"
#include <time.h>

#define TEMPLATE_PLACEHOLDER '%'
#define TEMPLATE_PARAM_NAME_LENGTH 32
class AsyncStaticFileResponse: public AsyncAbstractResponse {
  private:
    FILE* _pFile;
    String _path;
    void _setContentType(const String& path);
  public:
    AsyncStaticFileResponse(const String& foundFileName, const String& path, const String& contentType=String(), bool download=false, AwsTemplateProcessor callback=nullptr);
    ~AsyncStaticFileResponse();
    bool _sourceValid() const { return !!(_pFile); }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override;
};

class AsyncStaticFileHandler: public AsyncWebHandler {
  private:
    bool _getFile(AsyncWebServerRequest *request);
    bool _fileExists(AsyncWebServerRequest *request, const String& path);
    uint8_t _countBits(const uint8_t value) const;

  protected:
    String _uri;
    String _path;
    String _default_file;
    String _cache_control;
    String _last_modified;
    AwsTemplateProcessor _callback;
    bool _isDir;
    bool _gzipFirst;
    String _foundFileName;
    uint8_t _gzipStats;
  public:
    AsyncStaticFileHandler(const char* uri, const char* path, const char* cache_control);
    virtual bool canHandle(AsyncWebServerRequest *request) override final;
    virtual void handleRequest(AsyncWebServerRequest *request) override final;
    AsyncStaticFileHandler& setIsDir(bool isDir);
    AsyncStaticFileHandler& setDefaultFile(const char* filename);
    AsyncStaticFileHandler& setCacheControl(const char* cache_control);
    AsyncStaticFileHandler& setLastModified(const char* last_modified);
    AsyncStaticFileHandler& setLastModified(struct tm* last_modified);
  #ifdef ESP8266
    AsyncStaticFileHandler& setLastModified(time_t last_modified);
    AsyncStaticFileHandler& setLastModified(); //sets to current time. Make sure sntp is runing and time is updated
  #endif
    AsyncStaticFileHandler& setTemplateProcessor(AwsTemplateProcessor newCallback) {_callback = newCallback; return *this;}

    static bool existsAndIsAFile(const String& fileName);
    static size_t fileSizeInBytes(const String& fileName);
};
