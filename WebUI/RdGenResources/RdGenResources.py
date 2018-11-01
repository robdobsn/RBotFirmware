
# Generator for C/C++ header file containing web resources to be
# served statically from program memory in a microcontroller

import logging as log
import os, os.path
import shutil
import subprocess
import argparse

log.basicConfig(level=log.DEBUG)

def getMimeTypeFromFileExt(fileExt):
    mimeType = "text/plain"
    if fileExt == ".ico":
        mimeType = "image/ico"
    elif fileExt == ".gif":
        mimeType = "image/gif"
    elif fileExt == ".png":
        mimeType = "image/png"
    elif fileExt == ".jpg" or fileExt == ".jpeg":
        mimeType = "image/jpg"
    elif fileExt == ".bmp":
        mimeType = "image/bmp"
    elif fileExt == ".html":
        mimeType = "text/html"
    elif fileExt == ".css":
        mimeType = "text/css"
    elif fileExt == ".js":
        mimeType = "text/javascript"
    elif fileExt == ".xml":
        mimeType = "application/xml"
    return mimeType

def writeFileContentsAsHex(filePath, outFile, minifyHtml, lineHexIndentChars, lineHexBytesLen):
    filename, file_extension = os.path.splitext(filePath)
    # print("File", filePath, "name", filename, "ext", file_extension)
    inFileName = filePath
    if minifyHtml and file_extension.upper()[:4] == ".HTM":
        print("Minifying")
        inFileName = filename + ".tmp"
        # Copy file
        print("Copying", filePath, "to", inFileName)
        shutil.copyfile(filePath, inFileName)
        # Try to minify using npm html-minifier
        print("Running minifier on", filePath)
        rslt = subprocess.run(["html-minifier", filePath, "--minify-js", "--minify-css", "--remove-comments"], shell=True, stdout=subprocess.PIPE)
        if (rslt.returncode == 0):
            # print(rslt)
            with open(inFileName, "wb") as text_file:
                text_file.write(rslt.stdout)
            print("Input HTML was",os.stat(filePath).st_size,"bytes, minified file is",os.stat(inFileName).st_size, "bytes")
        else:
            print("MINIFY FAILED returncode", rslt.returncode)

    # Iterate through file in binary
    chCount = 0
    lineChIdx = 0
    with open(inFileName, "rb") as inFile:
        byte = inFile.read(1)
        while byte:
            if chCount != 0:
                outFile.write(",")
            if lineChIdx == 0:
                outFile.write("\n" + " " * lineHexIndentChars)
            outFile.write("0x" + format(ord(byte), "02x"))
            byte = inFile.read(1)
            lineChIdx += 1
            if lineChIdx >= lineHexBytesLen:
                lineChIdx = 0
            chCount += 1

    if minifyHtml and file_extension.upper()[:4] == ".HTM":
        print("Removing", inFileName)
        os.remove(inFileName)

def generateResourceFile(settings = None):
    # Defaults
    DEFAULT_TITLE = "Standard"
    DEFAULT_UI = "Standard"
    # Path to the src folder which is to contain the GenResources.h file
    DEFAULT_DESTPATH = "../PlatformIO/src"
    # NOTE that if MINIFY_HTML is True then the Node package html-minifier needs to be installed globally
    # npm install html-minifier -g
    MINIFY_HTML = False
    if settings is not None:
        if "DEFAULT_UI" in settings:
            DEFAULT_UI = settings["DEFAULT_UI"]
        if "DEFAULT_TITLE" in settings:
            DEFAULT_TITLE = settings["DEFAULT_TITLE"]
        if "DEFAULT_DESTPATH" in settings:
            DEFAULT_DESTPATH = settings["DEFAULT_DESTPATH"]
        if "MINIFY" in settings:
            MINIFY_HTML = settings["MINIFY"]
        uiFolder = "./" + DEFAULT_UI
    else:
        # Get command line argument to determine which UI to generate
        parser = argparse.ArgumentParser(description='Generate Compressed Web UI for Microcontroller')
        parser.add_argument('--UI', type=str, default=DEFAULT_UI, help=GEN_HELP_TEXT)
        parser.add_argument('--TITLE', type=str, default=DEFAULT_TITLE, help=GEN_HELP_TEXT)
        parser.add_argument('--DESTPATH', type=str, default=DEFAULT_DESTPATH, help=GEN_HELP_TEXT)
        args = parser.parse_args()
        uiFolder = "./" + args.UI + "UI"
    print("Generating UI from " + uiFolder + " minify = " + str(MINIFY_HTML))

    resFileInfo = []
    lineNormalIndentChars = 4
    lineHexBytesLen = 16
    lineHexIndentChars = 4
    with open(os.path.join(DEFAULT_DESTPATH, "WebAutogenResources.h"), "w") as outFile:
        outFile.write("// Auto-Generated file containing res folder binary contents\n")
        outFile.write("// Generated using GenResource.py - please don't edit manually\n")
        outFile.write("#pragma once\n\n")
        outFile.write("#include \"WebServerResource.h\"\n\n")
        walkGen = os.walk(uiFolder)
        for root,folders,fileNames in walkGen:
            for fileName in fileNames:
                filePath = os.path.join(root, fileName)
                # Get file parts to create variable name
                fileExtSplit = os.path.splitext(fileName)
                fileExt = fileExtSplit[1]
                fileOnly = os.path.split(fileExtSplit[0])[1]
                fileOnly = fileOnly.replace("-", "_")
                fileOnly = fileOnly.replace(".", "_")
                cIdent = "__webAutogenResource_" + fileOnly + "_" + fileExt[1:]
                print(cIdent, fileName)
                # Write variable def
                outFile.write("static const uint8_t " + cIdent + "[] PROGMEM {")
                # Write file contents as hex
                writeFileContentsAsHex(filePath, outFile, MINIFY_HTML, lineHexIndentChars, lineHexBytesLen)
                outFile.write("\n" + " " * lineHexIndentChars + "};\n\n")
                # Form the file info to be added to webUI
                fileInfoRec = {
                    "fileName": fileName,
                    "filePath": filePath,
                    "fileExt": fileExt,
                    "fileCIdent": cIdent
                }
                resFileInfo.append(fileInfoRec)

        # Now generate the list of resource descriptions
        isFirstLine = True
        outFile.write("// Web resource descriptions\n")
        outFile.write("static WebServerResource __webAutogenResources[] = {\n")
        for fileInf in resFileInfo:
            mimeType = getMimeTypeFromFileExt(fileInf["fileExt"])
            if not isFirstLine:
                outFile.write(",\n")
            isFirstLine = False
            outFile.write(" " * lineNormalIndentChars + "WebServerResource(")
            outFile.write("\"" + fileInf["fileName"] + "\", ")
            outFile.write("\"" + mimeType + "\", ")
            outFile.write("\"\", ")
            outFile.write("\"\", ")
            outFile.write(fileInf["fileCIdent"] + ", ")
            outFile.write("sizeof(" + fileInf["fileCIdent"] + ")-1)")
        outFile.write("\n" + " " * lineNormalIndentChars + "};\n\n")

        # Write the sixe of the resource list
        outFile.write("static int __webAutogenResourcesCount = sizeof(__webAutogenResources) / sizeof(WebServerResource);\n\n");
