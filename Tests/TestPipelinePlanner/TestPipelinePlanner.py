
import math

# Assume the units are mm/sec which is commonly used in 3D printers
blkLen = 1
minVel = 1
maxVel = 10
maxAccX = 10
maxAccY = 10
blocks = []

#for i in range(10):
#    blocks.append({"x":i*10, "y":i*10, "v":minVel})
def addBlock(x,y):
    blocks.append({"x":x, "y":y, "exitVelX":0, "exitVelY":0, "accX":0, "accY":0, "blkTime":0})

def solveTimeEq(blkEndVel, dist, acc):
    # Solving this using quadratic formula we get
    # timeInBlock = Uend +/- sqrt(Uend^2 - (2*dist*acceleration)) / acceleration
    timeA = blkEndVel + math.sqrt((blkEndVel * blkEndVel) - (2 * dist * acc)) / acc
    timeB = blkEndVel - math.sqrt((blkEndVel * blkEndVel) - (2 * dist * acc)) / acc
#    print("endVel", exitVelX, "dist", dist, "acc", acc, "time=", max(timeA, timeB))
    return max(timeA, timeB)

def withinBounds(val, minBound, maxBound):
    if val < minBound:
        return minBound
    if val > maxBound:
        return maxBound
    return val

addBlock(10,0)
addBlock(10,10)
# addBlock(0,10)
# addBlock(0,0)

print(blocks)

# Calculate max acceleration over direction component distances
# Assume that over each block we are travelling at an "average speed"
avgSpeedOverBlock = (maxVel + minVel) / 2
avgTimeInBlock = blkLen / avgSpeedOverBlock
maxSpeedChangeOverBlock = maxAccX * avgTimeInBlock
print("minVel", minVel, "maxVel", maxVel, "xMaxAcc", maxAccX, "avgSpeedOverBlock", avgSpeedOverBlock, "avgTimeInBlock", avgTimeInBlock, "maxSpeedChangeOverBlock", maxSpeedChangeOverBlock)

curX = 0
curY = 0
curVelX = 0
curVelY = 0
exitVelX = 0
exitVelY = 0
for loopIdx in range(len(blocks)):
    # Start with last block
    blkIdx = len(blocks)-1-loopIdx
    curBlock = blocks[blkIdx]

    # Prev block
    if blkIdx > 0:
        prevBlock = blocks[blkIdx-1]
    else:
        prevBlock = {"x":curX, "y":curY, "exitVelX":curVelX, "exitVelY": curVelY}

    # X and Y distances in the block
    distX = curBlock["x"] - prevBlock['x']
    distY = curBlock["y"] - prevBlock['y']

    print("MoveFrom", prevBlock['x'], prevBlock['y'], "To", curBlock["x"], curBlock["y"], "DistX", distX, "DistY", distY)

    # Lets say the time in the block a variable quantity
    # and calculate what's the minimum time we can be in the block
    # for each component of direction
    # The distance travelled dist = Uend * timeInBlock - 0.5 * acceleration * timeInBlock^2
    # This will always be a minimum when acceleration is a maximum + or - value
    # Required sign of acceleration = - sign of distance
    accMinTimeX = -maxAccX
    if distX < 0:
        accMinTimeX = maxAccX
    accMinTimeY = -maxAccY
    if distY < 0:
        accMinTimeY = maxAccY

    # Solving this using quadratic formula we get
    # timeInBlock = Uend +/- sqrt(Uend^2 - (2*dist*acceleration)) / acceleration
    timeX = solveTimeEq(exitVelX, distX, accMinTimeX)
    timeY = solveTimeEq(exitVelY, distY, accMinTimeY)
    blockTime = max(timeX, timeY)

    print("Time=", blockTime)

    # Check for zero time or distance
    if blockTime <= 0 or (distX == 0 and distY == 0):
        prevBlock["exitVelX"] = exitVelX
        prevBlock["exitVelY"] = exitVelX
        curBlock["accX"] = 0
        curBlock["accY"] = 0
        curBlock["blockTime"] = 0
        print("BlockTime or Dist==0")
        continue

    # Bound the entry velocity value (= exit velocity for previous block)
    meanVelX = distX / blockTime
    meanVelY = distY / blockTime
    entryVelX = 2 * meanVelX - exitVelX
    entryVelY = 2 * meanVelY - exitVelY
    entryVelX = withinBounds(entryVelX, -maxVel, maxVel)
    entryVelY = withinBounds(entryVelY, -maxVel, maxVel)

    # Recalculate the time for the block based on linear acceleration between two speeds (average speed)
    timeX = 0
    if exitVelX + entryVelX != 0:
        timeX = distX / ((exitVelX + entryVelX) / 2)
    timeY = 0
    if exitVelY + entryVelY != 0:
        timeY = distY / ((exitVelY + entryVelY) / 2)
    blockTime = max(timeX, timeY)

    # Re-cast the acceleration based on realistic velocity bounds
    accX = (exitVelX - entryVelX) / blockTime
    accY = (exitVelY - entryVelY) / blockTime

    # Put the exit speed into the prevBlock
    prevBlock["exitVelX"] = entryVelX
    prevBlock["exitVelY"] = entryVelY
    curBlock["accX"] = accX
    curBlock["accY"] = accY
    curBlock["blockTime"] = blockTime

    print("Block: entryVelX", entryVelX, "exitVelX", exitVelX, "accX", accX, "distX", distX, "checkX", entryVelX * blockTime + 0.5 * accX * blockTime * blockTime)
    print("Block: entryVelY", entryVelY, "exitVelY", exitVelY, "accY", accY, "distY", distY, "checkY", entryVelY * blockTime + 0.5 * accY * blockTime * blockTime)

    # Ready for working backwards
    exitVelX = entryVelX
    exitVelY = entryVelY

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


print("Output")
print("x","y","exitVX","exitVY")
for block in blocks:
    print(block['x'], block['y'], block['exitVelX'], block['exitVelY'], block['accX'], block['accY'], block['blockTime'])

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
    blockTime = block['blockTime']
    timeInBlock = 0
    while(timeInBlock < blockTime):
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