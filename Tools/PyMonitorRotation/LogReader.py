import logging

logger = logging.getLogger(__name__)

class LogReader:
    def __init__(self, serial, stepsPerRot1, stepsPerRot2):
        self.serial = serial
        self.curLine = ""
        self.stepsPerRot1 = stepsPerRot1
        self.stepsPerRot2 = stepsPerRot2

    def getNewValue(self, maxChars):
        charIdx = 0
        while charIdx < maxChars:
            charIdx += 1
            if self.serial.in_waiting == 0:
                break
            chRead = self.serial.read(1)
            if len(chRead) <= 0:
                break
            # print(chr(chRead[0]), end="")
            ch = chr(chRead[0])
            if ch == '\n':
                lin = self.curLine
                self.curLine = ""
                return self._parseLine(lin)
            self.curLine += ch
            if len(self.curLine) > 250:
                self.curLine = ""
        return -1,0,0

    def _parseLine(self, lineToParse):
        print(lineToParse)
        if not ("~M" in lineToParse):
            return -1,0,0
        valid = False
        fieldsOnly = lineToParse[lineToParse.index("~")+2:]
        lineFields = fieldsOnly.split(" ")
        if len(lineFields) >= 3:
            try:
                millis = int(lineFields[0])
                a1 = 360 * int(lineFields[1]) / self.stepsPerRot1
                a2 = 360 * int(lineFields[2]) / self.stepsPerRot2
                return millis, a1, a2
            except Exception as excp:
                print(excp)
        return -1,0,0
