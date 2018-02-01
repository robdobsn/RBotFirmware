import numpy as np
import matplotlib.pyplot as plt
import json
import glob

testRunIdx = 0

fileNameTemplate = "../TestOutputData/PipelinePlanner/steps_" + "{0:0>5}".format(testRunIdx) + "_*.txt"

testFiles = glob.glob(fileNameTemplate)

for testFile in testFiles:
    f = open(testFile)
    lines = f.readlines()

    # Find JSON
    jsonStr = ""
    lineIdx = 0
    while lineIdx < len(lines):
        if lines[lineIdx].strip() == "":
            break
        jsonStr += lines[lineIdx]
        lineIdx+=1

    configJson = json.loads(jsonStr)

    lines = lines[lineIdx+1:]

    stepDists = [configJson["axis0"]['unitsPerRotation']/configJson["axis0"]['stepsPerRotation'],
                 configJson["axis1"]['unitsPerRotation']/configJson["axis1"]['stepsPerRotation']]

    fieldCmd = 0
    fieldUs = 1
    fieldPin = 2
    fieldLevel = 3

    pinAxis0Step = ["st0"]
    pinAxis0Dirn = ["dr0"]
    pinAxis1Step = ["st1"]
    pinAxis1Dirn = ["dr1"]

    lastAxisUs = [0,0]
    axisDirn = [1, 1]

    axisDist = [[],[]]
    axisSpeed = [[],[]]
    axisTimes = [[],[]]
    axisXYLastUs = 0
    axisXY = [[],[]]
    startSet = False
    startUs = 0
    curDist = [0,0]
    for line in lines:
        fields = line.split()
        lineUs = int(fields[fieldUs])
        if not startSet or lineUs < startUs:
            startUs = lineUs
            lastUs = startUs
            startSet = True
        elapsedUs = lineUs - startUs

        lineCmd = fields[fieldCmd]
        linePin = fields[fieldPin]
        lineLevel = int(fields[fieldLevel])
        if lineCmd == "W":
            if linePin in pinAxis0Step or linePin in pinAxis1Step:
                if lineLevel == 0:
                    continue
                axisIdx = 0
                if linePin in pinAxis1Step:
                    axisIdx = 1
                intervalUs = elapsedUs - lastAxisUs[axisIdx]
                if intervalUs != 0:
                    speed = axisDirn[axisIdx] * stepDists[axisIdx] * 1e6 / intervalUs
                    # print(intervalUs)
                    lastAxisUs[axisIdx] = elapsedUs
                    axisSpeed[axisIdx].append(speed)
                    curDist[axisIdx] += axisDirn[axisIdx] * stepDists[axisIdx]
                    axisDist[axisIdx].append(curDist[axisIdx])
                    if axisXYLastUs != 0 and axisXYLastUs + 5 > elapsedUs:
                        axisXY[0][len(axisXY[0])-1] = curDist[0]
                        axisXY[1][len(axisXY[1]) - 1] = curDist[1]
                    else:
                        axisXY[0].append(curDist[0])
                        axisXY[1].append(curDist[1])
                        axisXYLastUs = elapsedUs
                    axisTimes[axisIdx].append(elapsedUs)
            if linePin in pinAxis0Dirn or linePin in pinAxis1Dirn:
                axisIdx = 0
                if linePin in pinAxis1Dirn:
                    axisIdx = 1
                axisDirn[axisIdx] = -1 if lineLevel else 1

    # for ax in axisSpeed:
    #    print(ax)

    fig = plt.figure()
    ax1 = fig.add_subplot(311)
    ax2 = fig.add_subplot(312)
    ax3 = fig.add_subplot(313)
    print("axisSpeed[0]", len(axisSpeed[0]), "\n", axisSpeed[0])
    print("axisTimes[0]", len(axisTimes[0]), "\n", axisTimes[0])
    # print("axisDist[0]", len(axisDist[0]), axisDist[0])
    # print("axisSpeed[1]", axisSpeed[1])
    # print("axisTimes[1]", axisTimes[1])
    # print("axisDist[1]", len(axisDist[1]), axisDist[1])
    ax1.scatter(axisTimes[0], axisDist[0], c="b", label="s vs t #1")
    ax1.scatter(axisTimes[1], axisDist[1], c="r", label="s vs t #2")

    ax2.scatter(axisTimes[0], axisSpeed[0], c="b", label="v vs t #1")
    ax2.scatter(axisTimes[1], axisSpeed[1], c="r", label="v vs t #2")

    ax3.scatter(axisXY[0], axisXY[1], c="g", label="XY")

    plt.show()

