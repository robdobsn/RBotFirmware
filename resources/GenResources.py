

import os.path

# f = open("res/favicon.ico")
# for s in f:
#     for c in s:
#         print("{0}   {0:02x}".format(ord(c)))
# f.close()
#
# # Create output file
# with outFile = open("GenResources.h", create)
#
# writeLine(outFile, "// Auto-Generated file containing res folder binary contents")
# writeLine(outFile, "#include \"RdWebServerResources.h\"")
# writeLine(outFile, "")
#
# for each file in "./res":
#

resFileInfo = []

lineNormalIndentChars = 4
lineHexBytesLen = 16
lineHexIndentChars = 4
with open("../RbotFirmware/GenResources.h", "w") as outFile:
    outFile.write("// Auto-Generated file containing res folder binary contents\n")
    outFile.write("#include \"RdWebServerResources.h\"\n\n")
    walkGen = os.walk("./res")
    for root,folders,fileNames in walkGen:
        for fileName in fileNames:
            filePath = os.path.join(root, fileName)
            # Get file parts to create variable name
            fileExtSplit = os.path.splitext(fileName)
            fileExt = fileExtSplit[1]
            fileOnly = os.path.split(fileExtSplit[0])[1]
            cIdent = "reso_" + fileOnly + "_" + fileExt[1:]
            print(cIdent, fileName)
            # Write variable def
            outFile.write("static const uint8_t " + cIdent + "[] {")
            # Iterate through file in binary
            chCount = 0
            lineChIdx = 0
            with open(filePath, "rb") as inFile:
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
            outFile.write("\n" + " " * lineHexIndentChars + "};\n\n")
            # Form the file info to be added to resources
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
        mimeType = "text/plain"
        if fileInf["fileExt"] == ".ico":
            mimeType = "image/ico"
        elif fileInf["fileExt"] == ".html":
            mimeType = "text/html"
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

