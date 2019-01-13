from twisted.web.static import File
from twisted.internet.defer import succeed
from klein import run, route
import json
import os
import cgi
from os.path import isfile, join
import re

stSettings = {
    "maxCfgLen": 2000,
    "name": "Sand Table",
    "startup": ""
}

stFileInfo = {
    "rslt":"ok",
    "fsName":"sd",
    "fsBase":"/sd",
    "diskSize":1374476,
    "diskUsed":1004,
    "folder":"/sd/",
    "files":
        [
            # {"name": "/test1.gcode", "size": 79},
            # {"name": "/pattern.param", "size": 200},
            # {"name": "/robot.json", "size": 47}
        ]

        }

stStatus = {}

stEvents = {}

stRobotTypes = ["SandTableScaraPiHat2","SandTableScaraPiHat3.6","SandTableScaraPiHat4","SandTableScaraMatt","XYBot"]

# stFile1 = {
#         "setup": "angle=0;diam=10",
#         "loop": "x=diam*sin(angle*3);y=diam*cos(angle*3);diam=diam+0.5;angle=angle+0.0314;stop=angle>6.28"
#         }
#
# stFile2 = "This is test text\nand another line of it\nand more ..."
#
# stFile3 = {"robotConfig":{"robotType":"SandTableScara","cmdsAtStart":"","homingSeq":"A-10000n;B10000;#;A+10000N;B-10000;#;A+500;B-500;#;B+10000n;#;B-10000N;#;B-1050;#;A=h;B=h;$","maxHomingSecs":120,"stepEnablePin":"4","stepEnLev":1,"stepDisableSecs":10,"blockDistanceMM":1,"allowOutOfBounds":1,"axis0":{"stepPin":"14","dirnPin":"13","maxSpeed":75,"maxAcc":50,"stepsPerRot":9600,"unitsPerRot":628.318,"maxVal":185,"endStop0":{"sensePin":"36","actLvl":0,"inputType":"INPUT_PULLUP"}},"axis1":{"stepPin":"15","dirnPin":"21","maxSpeed":75,"maxAcc":50,"stepsPerRot":9600,"unitsPerRot":628.318,"maxVal":185,"endStop0":{"sensePin":"39","actLvl":0,"inputType":"INPUT_PULLUP"}}},"patterns":{},"sequences":{},"name":""}

@route('/files/sd', branch=True)
def staticSd(request):
    return(File("./testfiles/sd"))

# @route('/files', branch=True)
# def staticFiles(request):
#     return File("./testfiles")

@route('/getsettings', branch=False)
def getsettings(request):
    return json.dumps(stSettings)

@route('/getRobotTypes', branch=False)
def getRobotTypes(request):
    return json.dumps(stRobotTypes)

@route('/status', branch=False)
def getstatus(request):
    return json.dumps(stStatus)

@route('/events', branch=False)
def getevents(request):
    return json.dumps(stEvents)

@route('/filelist/', branch=False)
def filelist(request):
    stFileInfo["files"] = [{"name":f,"size":1234} for f in os.listdir("./testfiles/sd/") if isfile(join("./testfiles/sd/", f))]
    return json.dumps(stFileInfo)

# @route('/fileread/<string:fileName>', branch=False)
# def fileread(request, fileName):
#     print("fileread ", fileName)
#     if (fileName == "test1.gcode"):
#         return stFile2
#     elif (fileName == "pattern.param"):
#         return json.dumps(stFile1)
#     elif (fileName == "robot.json"):
#         return json.dumps(stFile3)
#     return json.dumps({"rslt":"fail"})

@route('/uploadtofileman', methods=['POST'])
def uploadtofileman(request):
    method = request.method.decode('utf-8').upper()
    content_type = request.getHeader('content-type')
    content = request.content.read().decode("utf-8")
    fileInfo = request.args
    print(fileInfo)
    fileContent=fileInfo[b'file'][0].decode('utf-8')
    # fileContent = cgi.parse_multipart(content)
    # fileInfo = cgi.FieldStorage(
    #     fp = request.content,
    #     headers = request.getAllHeaders(),
    #     environ = {'REQUEST_METHOD': method, 'CONTENT_TYPE': content_type})
    # name = "./testfiles/" + fileInfo[b'datafile'].filename
    # print (f"Received file {name} content {request.args[b'datafile'][0]}")
    # print (f"Received file content {content}")
    filename = re.search(r'name="file"; filename="(.*)"', content).group(1)
    with open("./testfiles/sd/" + filename, 'w') as fileOutput:
        # fileOutput.write(img['datafile'].value)
        fileOutput.write(fileContent)
    return succeed(None)



    return json.dumps(stFileInfo)

@route('/postsettings', methods=['POST'])
def postsettings(request):
    global stSettings
    postData = json.loads(request.content.read().decode('utf-8'))
    print(postData)
    stSettings = postData
    return succeed(None)

@route('/playFile/<string:argToExec>', branch=False)
def playFile(request, argToExec):
    print("playFile ", argToExec)
    return succeed(None)

@route('/exec/<string:argToExec>', branch=False)
def execute(request, argToExec):
    print("Execute ", argToExec)
    return succeed(None)

@route('/deleteFile/<string:argToExec>', branch=False)
def deleteFile(request, argToExec):
    print("deleteFile ", argToExec)
    return succeed(None)

@route('/', branch=False)
def staticRoot(request):
    fileName = os.path.abspath(os.getcwd() + "../../../WebUI/ComboUI/sandUI.html")
    print(fileName, os.path.exists(fileName))
    with open(fileName, "r") as f:
        return f.read()

@route('/<string:pathfile>', branch=False)
def static(request, pathfile):
    fileName = os.path.abspath(os.getcwd() + "../../../WebUI/ComboUI/" + pathfile)
    print(fileName, os.path.exists(fileName))
    with open(fileName, "r") as f:
        return f.read()

run("localhost", 9027)
