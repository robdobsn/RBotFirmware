
import math

# for i in range(10):
#    blocks.append({"x":i*10, "y":i*10, "v":minVel})

def solveTimeEq(initialVel, dist, acc):
    if dist == 0:
        print("solveTimeEq: initialVel", initialVel, "dist", dist, "acc", acc, "time=", 0)
        return 0
    if acc == 0:
        timeA = abs(dist / initialVel)
        print("solveTimeEq: initialVel", initialVel, "dist", dist, "acc", acc, "time=", timeA)
        return timeA
    # Solving this using quadratic formula we get
    # timeInBlock = - Uend +/- sqrt(Uend^2 + (2*dist*acceleration)) / acceleration
    timeA = (- initialVel + math.sqrt((initialVel * initialVel) + (2 * dist * acc))) / acc
    timeB = (- initialVel - math.sqrt((initialVel * initialVel) + (2 * dist * acc))) / acc
    print("solveTimeEq: initialVel", initialVel, "dist", dist, "acc", acc, "time=", max(timeA, timeB))
    return max(timeA, timeB)


def withinBounds(val, minBound, maxBound):
    if val < minBound:
        return minBound
    if val > maxBound:
        return maxBound
    return val


# Assume the units are mm, mm/sec and mm/sec^2 which is commonly used in 3D printers
class AxisParams:
    def __init__(self, minVel = 1, maxVel = 100, maxAcc = 10, maxErr=0.1):
        self.minVel = minVel
        self.maxVel = maxVel
        self.maxAcc = maxAcc
        self.maxErr = maxErr

class AxisValue:
    def __init__(self, x=0, y=0, z=0):
        self.val = [x, y, z]
        self.valid = [1, 1, 1]

    def set(self, x, y, z):
        self.val = [x, y, z]
        self.valid = [1, 1, 1]

    def toString(self):
        return "x {:.2f} y {:.2f} z {:.2f}".format(self.val[0], self.val[1], self.val[2])

    def copy(self):
        return AxisValue(self.val[0], self.val[1], self.val[2])


class MotionBlock:
    def __init__(self, fromPoint, toPoint, entryVel = AxisValue(), exitVel = AxisValue(), acc = AxisValue(), blkTime = 0):
        self.frm = fromPoint.copy()
        self.to = toPoint.copy()
        self.entryVel = entryVel.copy()
        self.exitVel = exitVel.copy()
        self.acceleration = acc.copy()
        self.blkTime = blkTime

    def axisDist(self, axisIdx):
        return self.to.val[axisIdx] - self.frm.val[axisIdx]

    def toString(self):
        st = "From/to " + self.frm.toString() + ", " + self.to.toString()
        st += "  EntryV/exitV " + self.entryVel.toString() + ", " + self.exitVel.toString()
        st += "  Acc " + self.acceleration.toString()
        st += "  Time {:.2f}".format(self.blkTime)
        return st

def applyContraints(block, loopIdx, workForwards):

    print("..........................")

    minAxisTimes = [0 for i in range(MAX_AXES)]
    if not workForwards:
        blkIdx = len(blocks) - 1 - loopIdx
        curBlock = blocks[blkIdx]

        # Calculate overrun of block on each axis
        for axisIdx in range(MAX_AXES):
            minAxisTimes[axisIdx] = curBlock.blkTime
            axisDist = curBlock.axisDist(axisIdx)
            axisEntryVel = curBlock.entryVel.val[axisIdx]
            # Is there a change of direction in the current block
            if (axisEntryVel >= 0) != (axisDist >= 0):
                # Calculate distance travelled away from intended point
                timeTravellingWrongDirn = abs(axisEntryVel / axisParams[axisIdx].maxAcc)
                # Since the minBlockTime is based on all axes this could overshoot ...
                if timeTravellingWrongDirn > curBlock.blkTime:
                    print("Block", blkIdx, "Axis", axisIdx, "Time travelling in wrong direction > minBlockTime ...",
                          timeTravellingWrongDirn, ">", curBlock.blkTime)
                    timeTravellingWrongDirn = curBlock.blkTime
                distanceInWrongDirn = abs(axisEntryVel) * timeTravellingWrongDirn \
                                        - 0.5 * axisParams[axisIdx].maxAcc * timeTravellingWrongDirn ** 2
                print("Block", blkIdx, "Axis", axisIdx, "Overshoot! time", timeTravellingWrongDirn, "dist",
                      distanceInWrongDirn)

                if distanceInWrongDirn > axisParams[axisIdx].maxErr:
                    # Calculate max entry vel to fix this problem
                    maxEntryVel = math.sqrt(abs(2 * axisParams[axisIdx].maxAcc * axisDist))
                    print("New entry vel", maxEntryVel)
                    curBlock.entryVel.val[axisIdx] = maxEntryVel
                    changesMade = True
                    if blkIdx != 0:
                        blocks[blkIdx-1].exitVel.val[axisIdx] = maxEntryVel

        # Recalculate time for this axis based on any velocity changes
        for axisIdx in range(MAX_AXES):
            axisDist = curBlock.axisDist(axisIdx)
            minAxisTimes[axisIdx] = solveTimeEq(curBlock.entryVel.val[axisIdx], axisDist, curBlock.acceleration.val[axisIdx])

        # Find maximum of minTimes for each axis
        minBlockTime = 0
        for axTime in minAxisTimes:
            if minBlockTime < axTime:
                minBlockTime = axTime
        print("Minimum block time", minBlockTime)
        curBlock.blkTime = minBlockTime

        # Calculate the acceleration for the block based on any changes to entry velocity, exit velocity, distance and time
        for axisIdx in range(MAX_AXES):
            axisDist = curBlock.axisDist(axisIdx)
            if axisDist != 0:
                axisAcc = 2 * (axisDist - curBlock.entryVel.val[axisIdx] * curBlock.blkTime) / curBlock.blkTime**2
                # axisAcc = (curBlock.exitVel.val[axisIdx]**2 - curBlock.entryVel.val[axisIdx]**2) / 2 / axisDist
                curBlock.acceleration.val[axisIdx] = axisAcc


                    # timeTravellingWrongDirn = abs(maxEntryVel / axisParams[axisIdx].maxAcc)
                    # distanceInWrongDirn = abs(maxEntryVel) * timeTravellingWrongDirn \
                    #                       - 0.5 * axisParams[axisIdx].maxAcc * timeTravellingWrongDirn ** 2
                    # print("Block", blkIdx, "Axis", axisIdx, "Overshoot! time", timeTravellingWrongDirn, "dist",
                    #       distanceInWrongDirn)


MAX_AXES = 3
axisParams = [
    AxisParams(1, 100, 10, 0.1),
    AxisParams(1, 100, 10, 0.1),
    AxisParams(1, 100, 10, 0.1)
]


# Length of block (mm) and list of blocks
blkLen = 1
blocks = []
def addBlock(prevBlock, x, y, z):
    newBlock = (MotionBlock(prevBlock.to, AxisValue(x, y, z)))
    blocks.append(newBlock)
    return newBlock

startPos = MotionBlock(AxisValue(0,0,0), AxisValue(0,0,0))
prevBlock = addBlock(startPos, 1, 2, 0)
prevBlock = addBlock(prevBlock, 1, 1, 0)

TEST_AXES = 2

for loopIdx in range(len(blocks)):

    blkIdx = loopIdx
    curBlock = blocks[blkIdx]

    # Get prev block or initial block
    if blkIdx > 0:
        prevBlock = blocks[blkIdx-1]
    else:
        prevBlock = startPos

    print("..........................")

    print("Entry Vel", prevBlock.exitVel.toString())
    curBlock.entryVel = prevBlock.exitVel.copy()

    # Iterate each axis
    minAxisTimes = [0 for i in range(MAX_AXES)]
    for axisIdx in range(TEST_AXES):
        # distance in the block
        axisDist = curBlock.axisDist(axisIdx)
        axisEntryVel = curBlock.entryVel.val[axisIdx]

        print("Block", blkIdx, "AxisIdx", axisIdx, "From", curBlock.frm.val[axisIdx], "To", curBlock.to.val[axisIdx], "Dist", axisDist)

        # Go with max acceleration in the direction of travel
        # Also check if velocity is in same direction as travel and, if so, check if we're already at max velocity
        # and set acceleration to zero if so
        # This neglects the fact that we might accelerate beyond max in this block but hopefully the block is small so
        # this won't be a significant overrun
        testAcc = axisParams[axisIdx].maxAcc if axisDist >= 0 else -axisParams[axisIdx].maxAcc
        if (axisEntryVel >= 0) == (axisDist >= 0):
            if abs(axisEntryVel) >= axisParams[axisIdx].maxVel:
                testAcc = 0
        curBlock.acceleration.val[axisIdx] = testAcc

        # Solve the distance equation to get a minimum time for each direction
        minAxisTimes[axisIdx] = solveTimeEq(curBlock.entryVel.val[axisIdx], axisDist, testAcc)

        print("testAcc", testAcc, "minTime", minAxisTimes[axisIdx])

    # Find maximum of minTimes for each axis
    minBlockTime = 0
    for axTime in minAxisTimes:
        if minBlockTime < axTime:
            minBlockTime = axTime

    print("Minimum block time", minBlockTime)
    curBlock.blkTime = minBlockTime

    # Now that we know the minimum block time, re-calculate the acceleration and exit velocity
    for axisIdx in range(TEST_AXES):
        axisEntryVel = curBlock.entryVel.val[axisIdx]
        # With known entry velocity, block time and acceleration yield exit velocity
        exitVel = axisEntryVel + curBlock.acceleration.val[axisIdx] * minBlockTime
        curBlock.exitVel.val[axisIdx] = exitVel

    print("Exit Vel", curBlock.exitVel.toString())

# Now repeat backwards
print("-----------------")
print("In reverse")

# Enforce that the exit velocity is zero for the final block in the chain
finalBlock = blocks[len(blocks)-1]
finalBlock.exitVel = AxisValue()

for loopIdx in range(len(blocks)):
    applyContraints(blocks, loopIdx, False)





            # # Calculate overrun of block on each axis
    # for axisIdx in range(MAX_AXES):
    #     axisDist = curBlock.to.axisDist(axisIdx, prevBlock.to)
    #     axisEntryVel = curBlock.entryVel.val[axisIdx]
    #     if (axisEntryVel >= 0) != (axisDist >= 0):
    #         # Calculate distance travelled away from intended point
    #         timeTravellingWrongDirn = abs(axisEntryVel / axisParams[axisIdx].maxAcc)
    #         # Since the minBlockTime is based on all axes this could overshoot ...
    #         if timeTravellingWrongDirn > minBlockTime:
    #             print("Block", blkIdx, "Axis", axisIdx, "Time travelling in wrong direction > minBlockTime ...", timeTravellingWrongDirn, ">", minBlockTime)
    #             timeTravellingWrongDirn = minBlockTime
    #         distanceInWrongDirn = abs(axisEntryVel) * timeTravellingWrongDirn - 0.5 * axisParams[axisIdx].maxAcc * timeTravellingWrongDirn**2
    #         print("Block", blkIdx, "Axis", axisIdx, "Overshoot! time", timeTravellingWrongDirn, "dist", distanceInWrongDirn)
    #
    #         if distanceInWrongDirn > axisParams[axisIdx].maxErr:
    #             # Calculate max entry vel to fix this problem
    #             maxEntryVel = curBlock.entryVel
    #
    #

            # curBlock["accX"] = testAccX
    # curBlock["accY"] = testAccY
    # curBlock["blockTime"] = blockTime


        # blockTime = max(timeX, timeY)



            # # Lets say the time in the block a variable quantity
    # # and calculate what's the minimum time we can be in the block
    # # for each component of direction
    # # The distance travelled dist = Uend * timeInBlock - 0.5 * acceleration * timeInBlock^2
    # # This will always be a minimum when acceleration is a maximum + or - value
    # # Required sign of acceleration = - sign of distance
    # accMinTimeX = -maxAccX
    # if distX < 0:
    #     accMinTimeX = maxAccX
    # accMinTimeY = -maxAccY
    # if distY < 0:
    #     accMinTimeY = maxAccY
    #
    # # Solving this using quadratic formula we get
    # # timeInBlock = Uend +/- sqrt(Uend^2 - (2*dist*acceleration)) / acceleration
    # timeX = solveTimeEq(exitVelX, distX, accMinTimeX)
    # timeY = solveTimeEq(exitVelY, distY, accMinTimeY)
    # blockTime = max(timeX, timeY)
    #
    # print("Time=", blockTime)
    #
    # # Check for zero time or distance
    # if blockTime <= 0 or (distX == 0 and distY == 0):
    #     prevBlock["exitVelX"] = exitVelX
    #     prevBlock["exitVelY"] = exitVelX
    #     curBlock["accX"] = 0
    #     curBlock["accY"] = 0
    #     curBlock["blockTime"] = 0
    #     print("BlockTime or Dist==0")
    #     continue
    #
    # # Bound the entry velocity value (= exit velocity for previous block)
    # meanVelX = distX / blockTime
    # meanVelY = distY / blockTime
    # entryVelX = 2 * meanVelX - exitVelX
    # entryVelY = 2 * meanVelY - exitVelY
    # entryVelX = withinBounds(entryVelX, -maxVel, maxVel)
    # entryVelY = withinBounds(entryVelY, -maxVel, maxVel)
    #
    # # Recalculate the time for the block based on linear acceleration between two speeds (average speed)
    # timeX = 0
    # if exitVelX + entryVelX != 0:
    #     timeX = distX / ((exitVelX + entryVelX) / 2)
    # timeY = 0
    # if exitVelY + entryVelY != 0:
    #     timeY = distY / ((exitVelY + entryVelY) / 2)
    # blockTime = max(timeX, timeY)
    #
    # # Re-cast the acceleration based on realistic velocity bounds
    # accX = (exitVelX - entryVelX) / blockTime
    # accY = (exitVelY - entryVelY) / blockTime
    #
    # # Put the exit speed into the prevBlock
    # prevBlock["exitVelX"] = entryVelX
    # prevBlock["exitVelY"] = entryVelY
    # curBlock["accX"] = accX
    # curBlock["accY"] = accY
    # curBlock["blockTime"] = blockTime
    #
    # print("Block: entryVelX", entryVelX, "exitVelX", exitVelX, "accX", accX, "distX", distX, "checkX", entryVelX * blockTime + 0.5 * accX * blockTime * blockTime)
    # print("Block: entryVelY", entryVelY, "exitVelY", exitVelY, "accY", accY, "distY", distY, "checkY", entryVelY * blockTime + 0.5 * accY * blockTime * blockTime)
    #
    # # Ready for working backwards
    # exitVelX = entryVelX
    # exitVelY = entryVelY
    #
    # # Time in the block
    # timeInBlock = max(xDist/avgSpeedOverBlock, yDist/avgSpeedOverBlock)
    #
    # # Calculate max possible speed change over block in each direction
    # xMaxSpeedChangeOverBlock = xMaxAcc * timeInBlock
    # yMaxSpeedChangeOverBlock = yMaxAcc * timeInBlock
    #
    # # Calculate max vel entering block
    # maxEntryVelX = blkEndVelX + maxSpeedChangeOverBlock
    # maxEntryVelY = blkEndVelY + maxSpeedChangeOverBlock
    # block['entryVelX'] = maxVelX
    # block['entryVelY'] = maxVelY

    # Distance between cur and prev blocks
    #blockDist = math.sqrt((block["x"] - curX) * (block["x"] - curX) + (block["y"] - curY) * (block["y"] - curY))

    #
    # newDirection = math.atan2(block["y"] - curY, block["x"] - curX)
    # gamma = (newDirection + curDirection) / 2 + math.pi/2
    # curDirection = newDirection
    #
    # accXUnit = math.cos(gamma)
    # accyUnit = math.sin(gamma)

    # If last block in chain
    # if loopIdx == 0:
    #     # Assume velocity in each direction at block end is zero

    #


#    t = curV * math.sin(alpha / 2) / maxAcc

    #print(curDirection*180/math.pi, 'gamma', gamma*180/math.pi, accXUnit, accyUnit)

    # curX = curBlock["x"]
    # curY = curBlock["y"]


print("-----------------")
print("Result")
for block in blocks:
    print(block.toString())

exit(0)

posns = []

timeCount = 0
timeInc = 0.01
curVelX = exitVelX
curVelY = exitVelY
posX = 0
posY = 0
velX = []
velY = []
timeVals = []
for block in blocks:
    reqVelX = block['exitVelX']
    reqVelY = block['exitVelY']
    accX = block['accX']
    accY = block['accY']
    minBlockTime = block['blockTime']
    timeInBlock = 0
    while(timeInBlock < minBlockTime):
        timeCount += timeInc
        timeInBlock += timeInc
        posX += curVelX * timeInc
        curVelX += accX * timeInc
        posY += curVelY * timeInc
        curVelY += accY * timeInc
        posns.append([posX, posY])
        velX.append(curVelX)
        velY.append(curVelY)
        timeVals.append(timeCount)
    print("BlockEnd", curVelX, reqVelX, curVelY, reqVelY, "Pos", posX, posY)

import numpy as np
import matplotlib.pyplot as plt

xVals, yVals = zip(*posns)
plt.scatter(xVals, yVals)
plt.show()

plt.plot(timeVals, velX)
plt.show()

    # import numpy as np
# import matplotlib.pyplot as plt
# blkArry = []
# prevBlock = {"x":0, "y":0}
# for block in blocks:
#     newElem = [prevBlock['x'],prevBlock['y'],block['x']-prevBlock['x'],block['y']-prevBlock['y']]
#     blkArry.append(newElem)
#     prevBlock = block
# print(blkArry)
# soa = np.array(blkArry)
# X,Y,U,V = zip(*soa)
# plt.figure()
# ax = plt.gca()
# ax.quiver(X,Y,U,V,angles='xy',scale_units='xy',scale=1)
# ax.set_xlim([-1,20])
# ax.set_ylim([-1,20])
# plt.draw()
# plt.show()

# import numpy as np
# import matplotlib.pyplot as plt
# soa =np.array( [ [0,0,10,0], [10,0,10,10],[10,10,0,10]])
# X,Y,U,V = zip(*soa)
# plt.figure()
# ax = plt.gca()
# ax.quiver(X,Y,U,V,angles='xy',scale_units='xy',scale=1)
# ax.set_xlim([-1,20])
# ax.set_ylim([-1,20])
# plt.draw()
# plt.show()