from twisted.web.static import File
from twisted.internet.defer import succeed
from klein import run, route
import json

stSettings = {
    "name": "Sand Table",
    "patterns":
        {
            "pattern1":
                {
                    "setup": "angle=0;diam=10",
                    "loop": "x=diam*sin(angle*3);y=diam*cos(angle*3);diam=diam+0.5;angle=angle+0.0314;stop=angle>6.28"
                }
        },
    "sequences":
        {
            "testSequence": { "commands" : "G28; testPattern"},
            "testSequence2": { "commands" : "G28;G28"}
        },
    "startup": ""
}

@route('/', branch=False)
def static(request):
    return File("./res")

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
def exec(request, argToExec):
    print("Execute ", argToExec)
    return succeed(None)

run("localhost", 9027)
