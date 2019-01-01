
import logging
import time
from threading import Thread
from collections import deque
import threading

logger = logging.getLogger(__name__)

class MotorRotation:
    def __init__(self, sourceIsSerial, source, axisIdx, gearingRatio, stepsPerRotation):
        self.sourceIsSerial = sourceIsSerial
        self.axisIdx = axisIdx
        self.gearingRatio = gearingRatio
        self.stepsPerRotation = stepsPerRotation
        if sourceIsSerial:
            self.serial = source
        else:
            self.logReader = source
        self.running = False
        self.millisStr = ""
        self.angleStr = ""
        self.addToMillis = True
        self.measurements = deque()
        self.measLock = threading.Lock()
        self.maxMeasurementsLen = 3000

    def _readChars(self, maxChars):
        if not self.running:
            return
        while maxChars > 0:
            chRead = chr(self.serial.read(1)[0])
            maxChars -= 1
            if chRead == '\n':
                if (not self.addToMillis) and (len(self.millisStr) > 0) and \
                                (len(self.angleStr) > 0):
                    try:
                        mVal = int(self.millisStr)
                        aVal = float(self.angleStr)
                        with self.measLock:
                            if self.axisIdx == 0:
                                self.measurements.append((mVal,-aVal/self.gearingRatio))
                            else:
                                self.measurements.append((mVal,aVal/self.gearingRatio))
                            if len(self.measurements) > self.maxMeasurementsLen:
                                self.measurements.popleft()
                        self.addToMillis = True
                    except Exception as excp:
                        pass
                    # print(self.millisStr, self.angleStr)
                self.millisStr = ""
                self.angleStr = ""
            elif chRead == " ":
                if self.addToMillis:
                    self.addToMillis = False
            elif chRead.isdigit():
                if self.addToMillis:
                    self.millisStr += chRead
                else:
                    self.angleStr += chRead
            elif chRead == "." or chRead == "-":
                if not self.addToMillis:
                    self.angleStr += chRead

    def _receiveLoop(self):
        while self.running:
            if self.sourceIsSerial:
                numReady = self.serial.in_waiting
                if numReady < 1:
                    time.sleep(0.001)
                    continue
                self._readChars(numReady)
            else:
                newValValid, newMillis, newValue = self.logReader.getNewValue(self.axisIdx)
                if newValValid:
                    with self.measLock:
                        self.measurements.append((newMillis, newValue * 360 / self.stepsPerRotation))
                        if len(self.measurements) > self.maxMeasurementsLen:
                            self.measurements.popleft()

    def clear(self):
        with self.measLock:
            self.measurements = deque()

    def getMeasurements(self):
        with self.measLock:
            measList = list(self.measurements)
        return measList

    def startReader(self):
        if self.running:
            raise RuntimeError("reader already running")
        self.reader = Thread(target=self._receiveLoop)
        self.reader.setDaemon(True)
        self.running = True
        self.reader.start()
        print(f"Motor {self.axisIdx} reader started")

    def stopReader(self):
        self.running = False
        # self.reader.join()
        # self.reader = None
        print(f"Motor {self.axisIdx} reader stopped")
