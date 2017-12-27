import numpy as np
import matplotlib.pyplot as plt

f = open("../TestPipelinePlannerCLRCPP/TestPipelinePlannerCLRCPP/testOut/testOut_00048.txt")
lines = f.readlines()

stepDist = 60 / 3200

fieldCmd = 0
fieldUs = 1
fieldPin = 2
fieldLevel = 3

pinAxis0Step = 2
pinAxis0Dirn = 3
pinAxis1Step = 4
pinAxis1Dirn = 5

lastAxisUs = [0,0]
axisDirn = [1, 1]

axisDist = [[],[]]
axisSpeed = [[],[]]
axisTimes = [[],[]]
startSet = False
startUs = 0
curDist = 0
for line in lines:
    fields = line.split("\t")
    lineUs = int(fields[fieldUs])
    if not startSet:
        startUs = lineUs
        lastUs = startUs
        startSet = True
    elapsedUs = lineUs - startUs

    lineCmd = fields[fieldCmd]
    linePin = int(fields[fieldPin])
    lineLevel = int(fields[fieldLevel])
    if lineCmd == "W":
        if linePin == pinAxis0Step or linePin == pinAxis1Step:
            if lineLevel == 0:
                continue
            axisIdx = 0
            if linePin == pinAxis1Step:
                axisIdx = 1
            intervalUs = elapsedUs - lastAxisUs[axisIdx]
            if intervalUs != 0:
                speed = axisDirn[axisIdx] * stepDist * 1e6 / intervalUs
                print(intervalUs)
                lastAxisUs[axisIdx] = elapsedUs
                axisSpeed[axisIdx].append(speed)
                curDist += stepDist
                axisDist[axisIdx].append(curDist)
                axisTimes[axisIdx].append(elapsedUs)
        if linePin == pinAxis0Dirn or linePin == pinAxis1Dirn:
            axisIdx = 0
            if linePin == pinAxis1Dirn:
                axisIdx = 1
            axisDirn[axisIdx] = 1 if lineLevel else -1

#for ax in axisSpeed:
#    print(ax)

fig = plt.figure()
ax1 = fig.add_subplot(211)
ax2 = fig.add_subplot(212)
print(axisSpeed[0])
print(axisTimes[0])
ax1.scatter(axisTimes[0], axisDist[0], c="b", label="Distance vs time")
ax2.scatter(axisTimes[0], axisSpeed[0], c="r", label="Speed vs time")
plt.show()

