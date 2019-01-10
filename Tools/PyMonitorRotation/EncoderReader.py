import logging

logger = logging.getLogger(__name__)

class EncoderReader:
    def __init__(self, serial1, serial2):
        self.chan1 = { "idx":0, "serial": serial1, "curLine": "", "meas": 0, "ms": -1, "zeroPoint": 0}
        self.chan2 = { "idx":1, "serial": serial2, "curLine": "", "meas": 0, "ms": -1, "zeroPoint": 0}

    def getNewValue(self, maxChars):
        while (True):
            if not self.getOneVal(self.chan1, maxChars):
                break
        while (True):
            if not self.getOneVal(self.chan2, maxChars):
                break
        # print(self.chan1["ms"],self.chan2["ms"],self.chan1["meas"],self.chan1["zeroPoint"],self.chan2["meas"],self.chan2["zeroPoint"])
        return min(self.chan1["ms"],self.chan2["ms"]), \
               self.chan1["meas"] - self.chan1["zeroPoint"], \
               self.chan2["meas"] - self.chan2["zeroPoint"]

    def getOneVal(self, chan, maxChars):
        charIdx = 0
        while charIdx < maxChars:
            charIdx += 1
            if chan["serial"].in_waiting == 0:
                break
            chRead = chan["serial"].read(1)
            if len(chRead) <= 0:
                break
            # print(chr(chRead[0]), end="")
            ch = chr(chRead[0])
            if ch == '\n':
                lin = chan["curLine"]
                chan["curLine"] = ""
                # if chan["idx"] == 0:
                # print("LINE ", lin)
                ms, a = self._parseLine(lin)
                if (ms >= 0):
                    chan["meas"] = a
                    chan["ms"] = ms
                    # print(ms, a)
                    return True
                return False
            chan["curLine"] += ch
            if len(chan["curLine"]) > 250:
                chan["curLine"] = ""
        # if chan["idx"] == 0 and len(chan["curLine"]) > 0:
        #     print("INCOMPLETE LINE ", chan["curLine"])
        return False

    def _parseLine(self, lineToParse):
        # print(lineToParse)
        lineFields = lineToParse.split(" ")
        if len(lineFields) == 2:
            try:
                millis = int(lineFields[0])
                a1 = -float(lineFields[1])
                return millis, a1
            except Exception as excp:
                print(excp)
        return -1, 0

    def resetSteps(self):
        self.chan1["zeroPoint"] = self.chan1["meas"]
        self.chan2["zeroPoint"] = self.chan2["meas"]