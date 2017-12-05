
import logging as log
import os, os.path
import shutil
import subprocess
import argparse

# Title of program
GEN_RESOURCES_TITLE = "RdWebServer"

# UI Variants
DEFAULT_UI = "SandTable"
GEN_HELP_TEXT = '--UI [CNC | SandTable]'

# Path to the src folder which is to contain the GenResources.h file
GEN_RESOURCES_H_FOLDER = "../ParticleSW/src"

# NOTE that if --compress is True then the Node package html-minifier and minify need to be installed globally
# npm install html-minifier -g
# npm install -g minifier

log.basicConfig(level=log.DEBUG)

def getMimeTypeFromFileExt(fileExt):
    mimeType = "text/plain"
    if fileInf["fileExt"] == ".ico":
        mimeType = "image/ico"
    elif fileInf["fileExt"] == ".gif":
        mimeType = "image/gif"
    elif fileInf["fileExt"] == ".png":
        mimeType = "image/png"
    elif fileInf["fileExt"] == ".jpg" or fileInf["fileExt"] == ".jpeg":
        mimeType = "image/jpg"
    elif fileInf["fileExt"] == ".bmp":
        mimeType = "image/bmp"
    elif fileInf["fileExt"] == ".html":
        mimeType = "text/html"
    elif fileInf["fileExt"] == ".css":
        mimeType = "text/css"
    elif fileInf["fileExt"] == ".js":
        mimeType = "text/javascript"
    elif fileInf["fileExt"] == ".xml":
        mimeType = "application/xml"
    return mimeType

def writeFileContentsAsHex(filePath, outFile, compress):
    filename, file_extension = os.path.splitext(filePath)
    # print("File", filePath, "name", filename, "ext", file_extension)
    inFileName = filePath
    removeReqd = False
    if compress and file_extension.upper()[:4] == ".HTM":
        removeReqd = True
        print("Minifying")
        inFileName = filename + ".tmp"
        # Copy file
        print("Copying", filePath, "to", inFileName)
        shutil.copyfile(filePath, inFileName)
        # Try to minify using html-minifier
        print("Running minifier on", filePath)
        rslt = subprocess.run(["html-minifier", filePath, "--minify-js", "--minify-css", "--remove-comments"], shell=True, stdout=subprocess.PIPE)
        if (rslt.returncode == 0):
            # print(rslt)
            with open(inFileName, "wb") as text_file:
                text_file.write(rslt.stdout)
            print("Input HTML was",os.stat(filePath).st_size,"bytes, minified file is",os.stat(inFileName).st_size, "bytes")
        else:
            print("MINIFY FAILED returncode", rslt.returncode)
    elif compress and (file_extension.upper()[:3] == ".JS" or file_extension.upper()[:4] == ".CSS"):
        removeReqd = True
        print("Uglifying JS/CSS")
        print("Running minifier on", filePath)
        inFileName = os.path.splitext(filePath)[0] + ".min" + os.path.splitext(filePath)[1]
        rslt = subprocess.run(["minify", filePath], shell=True)
        if (rslt.returncode == 0):
            # print(rslt)
            print("Input was", os.stat(filePath).st_size, "bytes, minified file is",
                  os.stat(inFileName).st_size, "bytes")
        else:
            print("uglify FAILED returncode", rslt.returncode)

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

    if removeReqd:
        print("Removing", inFileName)
        os.remove(inFileName)

# Get command line argument to determine which UI to generate
parser = argparse.ArgumentParser(description='Generate ' + GEN_RESOURCES_TITLE + 'UI')
parser.add_argument('--UI', type=str, default=DEFAULT_UI, help=GEN_HELP_TEXT)
parser.add_argument('--COMPRESS', type=bool, default=True)
args = parser.parse_args()
uiFolder = "./" + args.UI + "UI"
compress = args.COMPRESS
print("Generating UI from " + uiFolder + " and compressing" if args.COMPRESS else "")

resFileInfo = []
lineNormalIndentChars = 4
lineHexBytesLen = 16
lineHexIndentChars = 4
with open(os.path.join(GEN_RESOURCES_H_FOLDER, "GenResources.h"), "w") as outFile:
    outFile.write("// Auto-Generated file containing res folder binary contents\n")
    outFile.write("#include \"RdWebServerResources.h\"\n\n")
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
            cIdent = "reso_" + fileOnly + "_" + fileExt[1:]
            print(cIdent, fileName)
            # Write variable def
            outFile.write("static const uint8_t " + cIdent + "[] {")
            # Write file contents as hex
            writeFileContentsAsHex(filePath, outFile, compress)
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
    outFile.write("// Resource descriptions\n")
    outFile.write("static RdWebServerResourceDescr genResources[] = {\n")
    for fileInf in resFileInfo:
        mimeType = getMimeTypeFromFileExt(fileInf["fileExt"])
        if not isFirstLine:
            outFile.write(",\n")
        isFirstLine = False
        outFile.write(" " * lineNormalIndentChars + "RdWebServerResourceDescr(")
        outFile.write("\"" + fileInf["fileName"] + "\", ")
        outFile.write("\"" + mimeType + "\", ")
        outFile.write(fileInf["fileCIdent"] + ", ")
        outFile.write("sizeof(" + fileInf["fileCIdent"] + "))")
    outFile.write("\n" + " " * lineNormalIndentChars + "};\n\n")

    # Write the sixe of the resource list
    outFile.write("static int genResourcesCount = sizeof(genResources) / sizeof(RdWebServerResourceDescr);\n\n");

