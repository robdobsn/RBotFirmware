import logging
import time
import threading
import math

logger = logging.getLogger(__name__)

class LogReader:
    def __init__(self, serial):
        self.serial = serial
        self.axisGot = [False, False]
        self.axisVals = [0,0]
        self.curMillis = 0
        self.dataValid = False
        self.curLine = ""
        self.measLock = threading.Lock()

    def getNewValue(self, axisIdx):
        # print("--------------", axisIdx, end="")
        time.sleep(0.01)
        with self.measLock:
            if not self.dataValid:
                if axisIdx == 0:
                    self.checkForNewData()
            #     print(" invalid")
            # else:
            #     print(" valid")

        if not self.dataValid:
            return False, 0, 0

        if self.axisGot[axisIdx]:
            return False, 0, 0

        axVal = self.axisVals[axisIdx]
        curMillis = self.curMillis
        self.axisGot[axisIdx] = True
        if self.axisGot[0] and self.axisGot[1]:
            self.dataValid = False

        return True, curMillis, axVal

    def _parseLine(self):
        print(self.curLine)
        if not ("~M" in self.curLine):
            return False
        valid = False
        fieldsOnly = self.curLine[self.curLine.index("~")+2:]
        lineFields = fieldsOnly.split(" ")
        if len(lineFields) >= 3:
            try:
                self.curMillis = int(lineFields[0])
                prev0 = self.axisVals[0]
                prev1 = self.axisVals[1]
                dest0 = int(lineFields[1])
                dest1 = int(lineFields[2])
                self.axisVals[0] = dest0
                self.axisVals[1] = dest1
                self.axisGot[0] = False
                self.axisGot[1] = False
                self.dataValid = True
                # print(f"{self.curLine} => {self.axisVals[0]} {self.axisVals[1]}")
                valid = True
            except Exception as excp:
                print(excp)
        return valid

    def _readChars(self, maxChars):
        while maxChars > 0:
            chRead = self.serial.read(1)
            if len(chRead) <= 0:
                break
            # print(chr(chRead[0]), end="")
            ch = chr(chRead[0])
            maxChars -= 1
            if ch == '\n':
                if self._parseLine():
                    self.curLine = ""
                    break
                # print(self.curLine)
                self.curLine = ""
            else:
                self.curLine += ch
                if len(self.curLine) > 2000:
                        self.curLine = ""

    def checkForNewData(self):
        self._readChars(1000)

