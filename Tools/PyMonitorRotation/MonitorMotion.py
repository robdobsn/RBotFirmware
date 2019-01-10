import os, sys
import configparser
import serial
import logging
from LogReader import LogReader
from EncoderReader import EncoderReader
import math
import tkinter
import collections
import pandas
from matplotlib import pyplot
import numpy as np
import scipy.signal

logging.basicConfig(level=os.environ.get("LOGLEVEL", "INFO"))

config = configparser.ConfigParser()
config.read("config.ini")
serialPort1 = config["DEFAULT"]["SerialPort1"]
serialPort2 = config["DEFAULT"]["SerialPort2"]
baudRate = int(config["DEFAULT"]["BaudRate"])
monitorFromLog = config["DEFAULT"]["MonitorFromLog"] == "True"
robotName = config["DEFAULT"]["RobotName"]

stepsPerRot1 = float(config[robotName]["StepsPerRot1"])
stepsPerRot2 = float(config[robotName]["StepsPerRot2"])
unitsPerRot1 = float(config[robotName]["UnitsPerRot1"])
unitsPerRot2 = float(config[robotName]["UnitsPerRot2"])
Gearing1 = int(config[robotName]["Gearing1"])
Gearing2 = int(config[robotName]["Gearing2"])
sizeX = int(config[robotName]["SizeX"])
sizeY = int(config[robotName]["SizeY"])
originX = int(config[robotName]["OriginX"])
originY = int(config[robotName]["OriginY"])
robotGeom = config[robotName]["RobotGeom"]

dataReader = None
serial1 = serial.Serial(serialPort1, baudRate)
if monitorFromLog:
    dataReader = LogReader(serial1, stepsPerRot1, stepsPerRot2)
else:
    serial2 = serial.Serial(serialPort2, baudRate)
    dataReader = EncoderReader(serial1, serial2)

print("Monitoring press esc to stop, c to clear, r to reset")

def calcSingleArmScara(degs1, degs2):
    ang1 = degs1 * math.pi / 180
    ang2 = (180 - degs2) * math.pi / 180
    l1 = sizeX / 4
    l2 = sizeX / 4
    elbowX = math.sin(ang1) * l1
    elbowY = math.cos(ang1) * l1
    curX = elbowX + math.sin(ang2) * l2
    curY = elbowY + math.cos(ang2) * l2
    # print(f"{degs1:4.2f}, {degs2:4.2f}, {ang1 * 180 / math.pi:4.2f}, {ang2 * 180 / math.pi:4.3f}, {curX:4.2f}, {curY:4.2f}")
    return curX, curY

def calcXYBot(ang1, ang2):
    x = ang1 * unitsPerRot1 / 360
    if x > sizeX - originX:
        x = sizeX - originX
    elif x < -originX:
        x = -originX
    y = ang2 * unitsPerRot2 / 360
    if y > sizeY - originY:
        y = sizeY - originY
    elif y < -originY:
        y = -originY
    # print(ang1, ang2, x, y)
    return x, y

def scaleXY(x,y):
    return canvasXNoBorder * (x + originX) / sizeX + border, (canvasY - border) - ((canvasYNoBorder * (y + originY) / sizeY) + border)

# Heatmap rgb colors in mapping order (ascending).
# From https://stackoverflow.com/questions/29267532/heat-map-from-data-points
palette = (0, 0, 1), (0, .5, 0), (0, 1, 0), (1, .5, 0), (1, 0, 0)
# palette = (0, 0, 1), (0, 0, 1), (1, 0, 0)

def pseudocolor(value, minval, maxval, palette):
    """ Maps given value to a linearly interpolated palette color. """
    max_index = len(palette)-1
    # Convert value in range minval...maxval to the range 0..max_index.
    v = (float(value-minval) / (maxval-minval)) * max_index
    if v >= max_index:
        v = max_index
    i = int(v); f = v-i  # Split into integer and fractional portions.
    c0r, c0g, c0b = palette[i]
    c1r, c1g, c1b = palette[min(i+1, max_index)]
    dr, dg, db = c1r-c0r, c1g-c0g, c1b-c0b
    return c0r+(f*dr), c0g+(f*dg), c0b+(f*db)  # Linear interpolation.

def colorize(value, minval, maxval, palette):
    """ Convert value to heatmap color and convert it to tkinter color. """
    color = (int(c*255) for c in pseudocolor(value, minval, maxval, palette))
    colrStr = '#{:02x}{:02x}{:02x}'.format(*color)  # Convert to hex string.
    # print(colrStr)
    return colrStr

def drawNext():
    global curMs, curX, curY, curPlotX, curPlotY
    global canvas, firstPoint, lastSpeedMs, speedMMps
    global firstPointMs, waitingForMotionEnd
    ms, ang1, ang2 = dataReader.getNewValue(1000)
    # print(ms, ang1, ang2)
    if ms > 0 and ms != curMs:
        ang1 = ang1 / Gearing1
        ang2 = ang2 / Gearing2
        if robotGeom == "XYBot":
            x,y = calcXYBot(ang1, ang2)
        elif robotGeom == "SingleArmScara":
            x,y = calcSingleArmScara(ang1, ang2)
        else:
            print("Unknown RobotGeom", robotGeom)
        # print(x,y)
        plotX,plotY = scaleXY(x,y)
        # Calc speed
        msDiff = ms - lastSpeedMs
        if msDiff > 10:
            dist = math.sqrt((x - curX) ** 2 + (y - curY) ** 2)
            # print(x,y,dist)
            if dist > 0.25 or (msDiff > 10 and waitingForMotionEnd):
                speedMMps = 1000 * dist / msDiff
                waitingForMotionEnd = dist > 1 or speedMMps > 0.5
                # if speedMMps != 0 or (not speedZero):
                #     print(x,y,msDiff,dist,speedMMps)
                if firstPointMs == 0:
                    firstPointMs = ms
                motionInfo.append((ms-firstPointMs, speedMMps))
                lastSpeedMs = ms
                curX = x
                curY = y
        if x != curX or y != curY:
            colr = colorize(speedMMps, 0, 120, palette)
            canvas.create_line(curPlotX,curPlotY,plotX,plotY,fill=colr,width=2)
            # print(speedMMps, colr)
            curPlotX = plotX
            curPlotY = plotY
        curMs = ms
    canvas.after(1,drawNext)

def close(event):
    tk.withdraw()
    tk.destroy()

def clear(event):
    canvas.delete('all')

def resetSteps(event):
    dataReader.resetSteps()
    motionInfo.clear()

def plotMotionInfo(event):
    dataSeries = list(zip(*motionInfo))

    # smoothedData = scipy.signal.savgol_filter(dataSeries[1], 51, 3)

    # sos = scipy.signal.cheby2(12, 20, 17, 'hp', fs=1000, output='sos')
    # smoothedData = scipy.signal.sosfilt(sos, dataSeries[1])

    b,a = scipy.signal.butter(3, 0.1)
    smoothedData = scipy.signal.filtfilt(b, a, dataSeries[1])

    pyplot.plot(dataSeries[0], smoothedData)
    pyplot.show()

canvasX = 800
canvasY = 800
border = 5
curX = 0
curY = 0
curMs = 0
lastSpeedMs = 0
speedMMps = 0
firstPointMs = 0
waitingForMotionEnd = False
motionInfo = collections.deque()
canvasXNoBorder = canvasX - border * 2
canvasYNoBorder = canvasY - border * 2
curPlotX, curPlotY = scaleXY(curX, curY)
tk = tkinter.Tk()
canvas = tkinter.Canvas(tk, height=800, width=800)
canvas.pack()
tk.bind('<Escape>', close)
tk.bind('c', clear)
tk.bind('r', resetSteps)
tk.bind('p', plotMotionInfo)
drawNext()
tk.mainloop()

