import numpy as np
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import math

fig, ax = plt.subplots()
# pos1 = ax.get_position()
# pos2 = [pos1.x0 + 0.1, pos1.y0 + 0.1, pos1.width, pos1.height]
# ax.set_position(pos2)

class Block:
    def __init__(self,x1,y1,x2,y2,vex=0, vey=0):
        self.fr = np.array([x1,y1])
        self.to = np.array([x2,y2])
        self.entryVel = np.array([vex,vey])
        self.exitVel = np.array([0,0])
        self.accVec = np.array([0,0])
        self.accPt = np.array([0,0])
        self.arcLen = 0
        self.arcRadius = 0

lineSegs = []
lineSegs.append(Block(0,0,1,0,1,0))
lineSegs.append(Block(1,0,1,1))
# lineSegs.append(Block(1,0,0.707,0.707))

curveSegs = []

# # Start segment - acceleration in direction of travel
# l0 = lineSegs[0]
# initSeg = Block(l0.fr[0], l0.fr[1], (l0.fr[0]+l0.to[0])/2, (l0.fr[1]+l0.to[1])/2)
# initSeg.accPt = np.array([l0.fr[0], l0.fr[1]])
# initSeg.accVec = np.array([l0.to[0], l0.to[1]])
# curveSegs.append(initSeg)


# Curve segments
for linSIdx in range(len(lineSegs)-1):
    l1 = lineSegs[linSIdx]
    l2 = lineSegs[linSIdx+1]
    crvS = Block((l1.fr[0]+l1.to[0])/2, (l1.fr[1]+l1.to[1])/2,(l2.fr[0]+l2.to[0])/2, (l2.fr[1]+l2.to[1])/2, l1.entryVel[0], l1.entryVel[1])
    #crvS = Block(l1.fr[0], l1.fr[1], l2.to[0], l2.to[1])
    # Calculate acceleration vector
    midPt = np.array([(crvS.fr[0]+crvS.to[0])/2, (crvS.fr[1]+crvS.to[1])/2])
    accVec = np.array([crvS.fr[1]-crvS.to[1], crvS.to[0]-crvS.fr[0]])
    crvS.accPt = midPt
    crvS.accVec = accVec
    # Arc length
    mL1 = math.atan2(l1.to[1]-l1.fr[1], l1.to[0]-l1.fr[0])
    mL2 = math.atan2(l2.to[1]-l2.fr[1], l2.to[0]-l2.fr[0])
    lenFromTo = math.sqrt((crvS.to[0] - crvS.fr[0]) ** 2 + (crvS.to[1] - crvS.fr[1]) ** 2)
    if abs(mL1) != abs(mL2):
        alpha = math.pi - abs(mL2 - mL1)
        print(mL1 * 180 / math.pi, mL2 * 180 / math.pi, alpha * 180 / math.pi)
        beta = alpha/2
        print(lenFromTo)
        rad = lenFromTo * math.sin(beta) / math.sin(math.pi-alpha)
        print(rad)
        arcLen = rad * (math.pi - alpha)
    elif mL1 == mL2:
        arcLen = lenFromTo
        radialAccFactor = 0
        rad = 1e20
    else:
        arcLen = 0
        rad = 0
    print("arcLen", arcLen, "ardRad", rad)
    crvS.arcLen = arcLen
    crvS.arcRadius = rad
    #

    # Add to list of segments
    curveSegs.append(crvS)

# # End segment - acceleration in opposite direction of travel
# endIdx = len(lineSegs)-1
# lN = lineSegs[endIdx]
# endSeg = Block(lN.fr[0], lN.fr[1], (lN.fr[0]+lN.to[0])/2, (lN.fr[1]+lN.to[1])/2)
# endSeg.accPt = np.array([lN.fr[0], lN.fr[1]])
# endSeg.accVec = np.array([-lN.to[0], -lN.to[1]])
# curveSegs.append(endSeg)








# Generate path
pathPoints = []
vel = lineSegs[0].entryVel
curPt = lineSegs[0].fr
distInCurve = 0
tInc = 0.01
crvSegIdx = 0
t = 0
while t < 10:
    crvS = curveSegs[crvSegIdx]
    dX = vel[0] * tInc
    dY = vel[1] * tInc
    newPt = np.array([curPt[0] + dX, curPt[1] + dY])
    distInCurve += math.sqrt(dX**2 + dY**2)
    vel = np.array([vel[0] + crvS.accVec[0]*tInc, vel[1] + crvS.accVec[1]*tInc])
    #print(newPt)
    pathPoints.append(newPt)
    curPt = newPt
    t+=tInc

    # Check if curve finished
    # doneX = curPt[0] >= crvS.to[0] if (crvS.to[0] > crvS.fr[0]) else curPt[0] <= crvS.to[0]
    # doneY = curPt[1] >= crvS.to[1] if (crvS.to[1] > crvS.fr[1]) else curPt[1] <= crvS.to[1]
    # if doneX and doneY:
    if distInCurve >= crvS.arcLen:
        crvSegIdx += 1
        distInCurve = 0
        if crvSegIdx >= len(curveSegs):
            break

# Plot
fig.set_size_inches(10,10)
for linS in lineSegs:
    (line_xs, line_ys) = zip(*[linS.fr, linS.to])
    ax.add_line(Line2D(line_xs, line_ys, linewidth=2,color="blue"))

for crvS in curveSegs:
    (line_xs, line_ys) = zip(*[crvS.fr, crvS.to])
    ax.add_line(Line2D(line_xs, line_ys, linewidth=2,color="green"))
    perpLine = np.array([crvS.accPt, [crvS.accPt[0]+crvS.accVec[0], crvS.accPt[1]+crvS.accVec[1]]])
    #print(perpLine)
    (line_xs, line_ys) = zip(*[perpLine[0], perpLine[1]])
    ax.add_line(Line2D(line_xs, line_ys, linewidth=2,color="red"))

(scat_xs, scat_ys) = zip(*pathPoints)
ax.scatter(scat_xs, scat_ys, 100, "orange")

ax.scatter([-0.1],[-0.1],s=0.01)

plt.show()
