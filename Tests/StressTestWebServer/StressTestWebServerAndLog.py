import time
import serial
import threading
import requests
from datetime import datetime

testDevice = "RedBearDuoRBotFirmwareSandTable"

if testDevice == "RedBearDuoRBotFirmwareSandTable":
    weburlandport = 'http://192.168.0.35/getsettings'
    webTestContent = "{"
    # weburlandport = 'http://192.168.0.35/q'
    # webTestContent = "rslt"
    # weburlandport = 'http://192.168.0.35:7123'
    # webTestContent = "Test"

#comport = "/dev/ttyS1"
comport = "COM14"

failureCounts = {
    "OK": 0,
    "reqOthrExcp": 0,
    "reqTimO": 0,
    "reqConnErr": 0,
    "serTimO": 0,
    "serExcp": 0,
    "reopenedSerial": 0,
    "non200Resp": 0,
    "testPageTxt": 0,
    "errWrite-18": 0,
    "errWrite-1": 0,
    "errWriteOther": 0,
    "openSerialExcp" : 0,
}

indentPrefix = "......."

def writeLog(lin, showTime=True, printToConsole=True):
    logFile = open("StressLog1.txt", "a")
    timeStr = datetime.now().strftime("%Y%m%d %H%M%S")
    if showTime:
        logFile.write(timeStr + " " + lin + "\n")
        if printToConsole:
            print(timeStr + " " + lin)
    else:
        logFile.write(lin + "\n")
        if printToConsole:
            print(lin)
    logFile.close()

# Read data from serial port and echo
def serialRead():
    global serialIsClosing, serPort
    serExcept = False
    serialLine = ""
    lastChWasEOL = False
    lastSerialLine = ""
    serOpenExcept = False
    while True:
        # Handle closing down
        if serialIsClosing:
            break
        # Get a char if there is one
        serTimeoutExcept = None
        try:
            if serPort.isOpen():
                val = serPort.read(1)
                if len(val) == 0:
                    continue
                serCh = val.decode("utf-8")
                if serCh == "\r" or serCh == "\n":
                    if not lastChWasEOL:
                        lastChWasEOL = True
                        lastSerialLine = serialLine
                        serialLine = ""
                        # print(lastSerialLine)
                        if "error writing -18" in lastSerialLine:
                            failureCounts["errWrite-18"] += 1
                        elif "error writing -1" in lastSerialLine:
                            failureCounts["errWrite-1"] += 1
                        elif "error writing" in lastSerialLine:
                            failureCounts["errWrite-other"] += 1
                        writeLog(lastSerialLine, True, True)  # True if ("Avg" in lastSerialLine) else False)
                    else:
                        lastChWasEOL = False
                else:
                    serialLine += serCh
            else:
                try:
                    serPort = serial.Serial(port=comport, baudrate=115200, timeout=1)
                    if serPort.isOpen():
                        writeLog("Reopened Serial Port")
                        failureCounts["reopenedSerial"] += 1
                        serOpenExcept = False
                except Exception as excp:
                    if not serOpenExcept:
                        writeLog("Serial Port Open Exception " + str(excp))
                        failureCounts["openSerialExcp"] += 1
                        serOpenExcept = True
            serExcept = False
            serTimeoutExcept = False

        except serial.SerialTimeoutException:
            if not serTimeoutExcept:
                writeLog("Serial timeout exception")
                serTimeoutExcept = True
                serExcept = False
                serPort.close()
                failureCounts["serTimO"] += 1
        except serial.SerialException:
            if not serExcept:
                writeLog("Serial exception")
                serExcept = True
                serTimeoutExcept = False
                serPort.close()
                failureCounts["serExcp"] += 1

writeLog("\n\n", False)
writeLog("Stress Test Starting ...")

# Serial connection
serialIsClosing = False
serPort = serial.Serial()
#serPort = serial.Serial(port=comport, baudrate=115200, timeout=1)

# Thread for reading from port
thread = threading.Thread(target=serialRead, args=())
thread.start()

def showFailCounts():
    infoStr = indentPrefix
    for key, value in failureCounts.items():
        infoStr += " " + key + " " + str(value)
    writeLog(infoStr)

writeLog("\n\n", False)
writeLog("Stress Test Starting ...")

# State machine for testing
testState = "reqPage"
testNumPages = 100
testPageCount = 0
testStage = 0
testNumStages = 4
testNumCycles = 0
testNumReqs = 0
TEST_CYCLES_REQUIRED = 1

def handleTestState():
    global testState, testPageCount, testStage, serPort, testNumCycles, testNumReqs, testNumPages
    if testState == "reqPage":
        rsltCh = "."
        try:
            r = None
            r = requests.get(weburlandport, timeout=3.0)
            # print(r.status_code)
            if not webTestContent in r.text:
                failureCounts["testPageTxt"] += 1
                rsltCh = "X"
            if r.status_code != 200:
                failureCounts["non200Resp"] += 1
                rsltCh = "2"
            else:
                failureCounts["OK"] += 1
                rsltCh = "."
        except requests.exceptions.ConnectTimeout:
            failureCounts["reqTimO"] += 1
            rsltCh = "T"
        except requests.exceptions.ConnectionError:
            failureCounts["reqConnErr"] += 1
            rsltCh = "C"
        except requests.exceptions.RequestException as excp:
            failureCounts["reqOthrExcp"] += 1
            writeLog("Other exception " + str(excp))
            rsltCh = "O"
        print(rsltCh, end="", flush=True)
        testNumReqs += 1
        testPageCount += 1
        if testPageCount >= testNumPages:
            testPageCount = 0
            testNumCycles += 1
            writeLog(indentPrefix + " Cycles " + str(testNumCycles) + " TotalRequests " + str(testNumReqs))
            testState = "reqPage"

# Test web using request
while(testNumCycles < TEST_CYCLES_REQUIRED):
    handleTestState()
    time.sleep(0.1)

showFailCounts()

