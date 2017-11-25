from twisted.web.static import File
from twisted.internet.defer import succeed
from klein import run, route
import json
import queue
import threading
import re

gCodeMaxQueueLen = 5
gCodeQueue = queue.Queue(gCodeMaxQueueLen)
machineStatus = {"pos": {"X": 0, "Y": 0, "Z": 0}}
moveRate = 10
moving = False
moveTo = {}
moveStep = 0
axes = ["X", "Y", "Z"]

stSettings = {
    "maxCfgLen": 2000,
    "name": "Mug Bot",
    "patterns":
        {
        },
    "sequences":
        {
        },
    "startup": ""
}

@route('/', branch=True)
def static(request):
    return File("../../WebUI/CNCUI")

@route('/getsettings', branch=False)
def getsettings(request):
    return json.dumps(stSettings)

@route('/postsettings', methods=['POST'])
def postsettings(request):
    global stSettings
    postData = json.loads(request.content.read().decode('utf-8'))
    print(postData)
    stSettings = postData
    return succeed(None)

@route('/exec/<string:argToExec>', branch=False)
def execute(request, argToExec):
    if gCodeQueue.full():
        request.setResponseCode(218) # 218 I'm a teapot - probably would be better 503 Service Unavailable!
        return succeed(None)
    print("Adding ", argToExec)
    gCodeQueue.put(argToExec)
    return succeed(None)

@route('/stop', branch=False)
def getsettings(request):
    print("Stopping")
    while not gCodeQueue.empty():
        gCodeQueue.get()
    return succeed(None)

@route('/status', branch=False)
def getstatus(request):
    global machineStatus
    return json.dumps(machineStatus)

def doMoveHome(moveToPos):
    global moving, moveStep, moveTo
    for ax in axes:
        if ax in moveToPos and moveToPos[ax] is not None:
            moveTo[ax] = 0
    moving = True
    moveStep = 0
    moveTimer = threading.Timer(0.1, moveElem)
    moveTimer.start()

def doMoveTo(moveToPos):
    global moving, moveStep, moveTo
    for ax in axes:
        if ax in moveToPos:
            try:
                moveTo[ax] = float(moveToPos[ax])
            except:
                moveTo[ax] = machineStatus["pos"][ax]
        else:
            moveTo[ax] = machineStatus["pos"][ax]
    moving = True
    moveStep = 0
    moveTimer = threading.Timer(0.1, moveElem)
    moveTimer.start()

def stepAxis(axis):
    global machineStatus, moveTo
    mv = moveRate if abs(moveTo[axis]-machineStatus["pos"][axis]) > moveRate else abs(moveTo[axis]-machineStatus["pos"][axis])
    if machineStatus["pos"][axis] > moveTo[axis]:
        mv = -mv
    if abs(machineStatus["pos"][axis]-moveTo[axis]) > 0.001:
        machineStatus["pos"][axis] += mv
        return True
    return False

def moveElem():
    global moving
    moved = False
    for ax in axes:
        moved |= stepAxis(ax)
    if not moved:
        print("Finished moving")
        moving = False
    else:
        print(machineStatus, moveTo)
        moveTimer = threading.Timer(0.1, moveElem)
        moveTimer.start()

def removeElem():
    global elementTimer
    if not gCodeQueue.empty():
        el = gCodeQueue.get()
        print("Removing", el)
        if gCodeQueue.empty():
            print("Queue now empty")
        # Find G0..G3 commands
        reGcode = re.compile(
            '(?i)^G[0-3][^0-9](?:\s*x(?P<x>-?[0-9.]{1,15})|\s*y(?P<y>-?[0-9.]{1,15})|\s*z(?P<z>-?[0-9.]{1,15})|\s*f(?P<f>-?[0-9.]{1,15})|\s*e(?P<e>-?[0-9.]{1,15}))*')
        mtch = reGcode.match(el)
        # print(mtch)
        if mtch:
            doMoveTo(mtch.groupdict())
        else:
            # Find G28 commands
            reGcode = re.compile(
                '(?i)^G28(?:\s+?(?P<x>x)|\s+?(?P<y>y)|\s+?(?P<z>z))*')
            mtch = reGcode.match(el)
            if mtch:
                doMoveHome(mtch.groupdict())
    elementTimer = threading.Timer(0.4, removeElem)
    elementTimer.start()

elementTimer = threading.Timer(0.4, removeElem)
elementTimer.start()

run("localhost", 9027)
