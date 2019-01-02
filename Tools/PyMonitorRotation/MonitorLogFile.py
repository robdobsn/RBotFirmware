import os, sys
import configparser
import serial
import logging
from LogReader import LogReader
import math
import tkinter

logging.basicConfig(level=os.environ.get("LOGLEVEL", "INFO"))

config = configparser.ConfigParser()
config.read("config.ini")
serialPort1 = config["DEFAULT"]["SerialPort1"]
baudRate = config["DEFAULT"]["BaudRate"]

serial1 = serial.Serial(serialPort1, baudRate)
logReader = LogReader(serial1)

print("Monitoring log, press esc to stop")

stepsPerRotation = 9600

def calcXY(steps1, steps2):
    ang1 = steps1 * math.pi * 2 / stepsPerRotation
    ang2 = (stepsPerRotation/2 - steps2) * math.pi * 2 / stepsPerRotation
    print(steps1, steps2, ang1 * 180 / math.pi, ang2 * 180 / math.pi)
    l1 = 92.5
    l2 = 92.5
    elbowX = math.sin(ang1) * l1
    elbowY = math.cos(ang1) * l1
    curX = elbowX + math.sin(ang2) * l2
    curY = elbowY + math.cos(ang2) * l2
    return curX, curY

canvasX = 800
canvasY = 800
sizeX = 370
sizeY = 370
originX = 185
originY = 185

def scaleXY(x,y):
    return canvasX * (x + originX) / sizeX, canvasY - (canvasY * (y + originY) / sizeY)

curX = canvasX/2
curY = canvasY/2
def drawNext():
    global curX, curY, canvas
    millis, s1, s2 = logReader.getNewValue(100)
    if millis > 0:
        x,y = calcXY(s1, s2)
        print(x,y)
        x,y = scaleXY(x,y)
        line = canvas.create_line(curX,curY,x,y,fill="red")
            # canvas.create_line(0,0,100,100+y,fill="red")
            # print("H")
        curX = x
        curY = y
    canvas.after(1,drawNext)

def close(event):
    tk.withdraw()
    tk.destroy()

tk = tkinter.Tk()
canvas = tkinter.Canvas(tk, height=800, width=800)
canvas.pack()
tk.bind('<Escape>', close)
drawNext()
tk.mainloop()

