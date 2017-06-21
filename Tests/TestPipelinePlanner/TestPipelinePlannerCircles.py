
import math

blockStart=[0,0]
def addBlock(x,y):
    global blockStart
    blocks.append({"x1":blockStart[0], "y1":blockStart[1], "x2":x, "y2":y, "exitVelX":0, "exitVelY":0, "accX":0, "accY":0, "blkTime":0})
    blockStart = [x,y]

def bisector(p1, p2):
    x1, y1 = p1
    x2, y2 = p2
    if (y1 == y2):
        return 100000,
    a = (x2-x1)/(y1-y2)
    b = (y1+y2)/2-a*(x1+x2)/2
    return a, b

def intersect(f1, f2):
    a1, b1 = f1
    a2, b2 = f2
    x = (b1-b2)/(a2-a1)
    y = a1*x+b1
    return x, y

def distance(p1, p2):
    x1, y1 = p1
    x2, y2 = p2
    return ((x1-x2)**2+(y1-y2)**2)**0.5

def circle(p1, p2, p3):
    x1, y1 = p1
    x2, y2 = p2
    x3, y3 = p3

    if y1==y2:
        p3, p2 = p2, p3
    elif y2==y3:
        p1, p2 = p2, p1

    center = intersect(bisector(p1, p2), bisector(p2, p3))
    radius = distance(center, p1)
    cx, cy = center
    return '(x-{x0:.{w}f})^2 + (y-{y0:.{w}f})^2 = {r:.{w}f}^2'.format(w=2, x0=cx, y0=cy, r=radius)

def vecMag(v):
    return math.sqrt(v[0]*v[0]+v[1]*v[1])

def vec(p1,p2):
    x1, y1 = p1
    x2, y2 = p2
    p1p2 = (x2-x1,y2-y1)
    return p1p2

def vecUnit(p1,p2):
    x1, y1 = p1
    x2, y2 = p2
    p1p2 = (x2-x1,y2-y1)
    p1p2Mag = math.sqrt(p1p2[0]*p1p2[0]+p1p2[1]*p1p2[1])
    p1p2Unit = (p1p2[0]/p1p2Mag, p1p2[1]/p1p2Mag)
    return p1p2Unit

def vecMult(v, s):
    vRes = (v[0]*s, v[1]*s)
    return vRes

def vecAdd(v1, v2):
    return (v1[0]+v2[0],v1[1]+v2[1])

def lineSeg(p1,p2):
    return (p1[0],p1[1],p2[0],p2[1])

def lineSegMidPt(ls):
    return ((ls[0]+ls[2])/2,(ls[1]+ls[3])/2)

def circle2(p1, p2, p3):
    x1, y1 = p1
    x2, y2 = p2
    x3, y3 = p3

    # Vector p1p2
    p1p2 = vec(p1,p2)

    # Unit Vector p2p3
    p2p3Unit = vecUnit(p2,p3)

    # Make Vector p2p3 same magnitude as p1p2
    p2p3T = vecMult(p2p3Unit, vecMag(p1p2))
    p3T = vecAdd(p2, p2p3T)

    print("p1p2", p1p2, "p2p3Unit", p2p3Unit, "p2p3T", p2p3T)
    #
    # # Find midpoint of line from p1 to p3T
    # lineSegP1P3T = lineSeg(p1,p3T)
    # ptD = lineSegMidPt(lineSegP1P3T)

    # Length of line segment p1 to p3T
    p1p3T = vec(p1, p3T)
    lenP1P3T = vecMag(p1p3T)

    # Length of line segment p1 to p2
    lenP1P2 = vecMag(p1p2)

    print("lenP1P2", lenP1P2, "lenP1P3T", lenP1P3T)

    # Circle radius
    circleRadius = lenP1P2 * (0.5 * lenP1P3T) / (math.sqrt(lenP1P2*lenP1P2 - 0.25*lenP1P3T*lenP1P3T))

    # Perpendicular to p1p2Unit
    p1p2Unit = vecUnit(p1,p2)
    p1p2UnitPerp = (-p1p2Unit[1], p1p2Unit[0])

    # Centre of circle
    circleCentre = vecMult(p1p2UnitPerp, circleRadius)

    print("circleRadius", circleRadius, "circleCentre", circleCentre)

    return ""


blocks = []

addBlock(10,0)
addBlock(10,20)
# addBlock(0,10)
# addBlock(0,0)

print(blocks)

for loopIdx in range(len(blocks)-1):

    # Current block
    blkIdx = len(blocks)-1-loopIdx
    curBlock = blocks[blkIdx]

    # Prev block
    prevBlock = blocks[blkIdx-1]

    # X and Y distances in the block
    distX = curBlock["x2"] - curBlock['x1']
    distY = curBlock["y2"] - curBlock['y1']

    print("MoveFrom", curBlock['x1'], curBlock['y1'], "To", curBlock["x2"], curBlock["y2"], "DistX", distX, "DistY", distY)

    # Compute circle containing block start and end points
    print(prevBlock['x1'])
    print(circle2((prevBlock['x1'], prevBlock['y1']), (curBlock['x1'], curBlock['y1']), (curBlock['x2'], curBlock['y2'])))

