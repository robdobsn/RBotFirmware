
import os, sys, time
import configparser
import serial
import logging
from MotorRotation import MotorRotation
from LogReader import LogReader
import keyboard
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from  matplotlib.colors import LinearSegmentedColormap
import math

logging.basicConfig(level=os.environ.get("LOGLEVEL", "INFO"))

config = configparser.ConfigParser()
config.read("config.ini")
serialPort1 = config["DEFAULT"]["SerialPort1"]
serialPort2 = config["DEFAULT"]["SerialPort2"]
baudRate = config["DEFAULT"]["BaudRate"]
monitorFromLog = config["DEFAULT"]["MonitorFromLog"]

serial1 = serial.Serial(serialPort1, baudRate)
if monitorFromLog:
    logReader = LogReader(serial1)
    motor1 = MotorRotation(False, logReader, 0, 3, 9600)
    motor2 = MotorRotation(False, logReader, 1, 3, 9600)
else:
    serial2 = serial.Serial(serialPort2, baudRate)
    motor1 = MotorRotation(True, serial1, 0, 3, 9600)
    motor2 = MotorRotation(True, serial2, 1, 3, 9600)
motor1.startReader()
motor2.startReader()

def key_handler(event):
    global motor1, motor2
    # global colorindex
    sys.stdout.flush()
    print('Key pressed:', event.key)
    if event.key == "escape":
        print("Stopping")
        motor1.stopReader()
        motor2.stopReader()
        ani.event_source.stop()
        sys.exit(0)
    elif event.key == "C" or event.key == "c":
        motor1.clear()
        motor2.clear()

  # sys.stdout.flush()
  # if event.key == 'x':
  #   print "Exiting..."
  #   sys.exit(0)
  # elif event.key == 'c':
  #   print "changing color"
  #   colorindex += 1
  #   if colorindex >= len(colors):
  #     colorindex = 0
  #   plotfig()

print("Monitoring motors, press esc to stop")

# keyboard.wait('esc')
#
# print(f"Motor1 count {len(motor1.measurements)}")
# print(f"Motor2 count {len(motor2.measurements)}")
#
# motor1.stopReader()
# motor2.stopReader()

fig = plt.figure()
figSub1 = fig.add_subplot(1,1,1)

# plt.subplot(1, 2, 1)
# plt.plot(times1, angles1, 'k.-')
# plt.title('Angle vs time')
# plt.xlabel('time (ms)')
# plt.ylabel('Motor 1')
#
# plt.subplot(1, 2, 2)
# plt.plot(times2, angles2, 'r.-')
# plt.xlabel('time (ms)')
# plt.ylabel('Motor 2')

def calcPlotVectorsScara(angles1, angles2):
    # Assume 0,0 is the home position
    vec1 = []
    vec2 = []
    vec3 = []
    handX = 0
    handY = 0
    ang1 = 0
    ang2 = 180
    for i in range(len(angles1)):
        ang1 = angles1[i] * math.pi / 180
        ang2 = (180 - angles2[i]) * math.pi / 180
        l1 = 92.5
        l2 = 92.5
        elbowX = math.sin(ang1) * l1
        elbowY = math.cos(ang1) * l1
        curX = elbowX + math.sin(ang2) * l2
        curY = elbowY + math.cos(ang2) * l2
        dist = math.sqrt((curX-handX)**2 + (curY-handY)**2)
        speed = dist / 0.01
        vec1.append(curX)
        vec2.append(curY)
        vec3.append(speed)
        handX = curX
        handY = curY
        # print("Angles", ang1*180/math.pi, ang2*180/math.pi, elbowX, elbowY, handX, handY, speed)
    return (vec1, vec2, vec3, ang1 * 180 / math.pi, ang2 * 180 / math.pi, handX, handY)

def animate(i):
    meas1 = motor1.getMeasurements()
    meas2 = motor2.getMeasurements()
    angles1 = [k[1] for k in meas1]
    angles2 = [k[1] for k in meas2]
    if len(angles1) > len(angles2):
        angles1 = angles1[:len(angles2)]
    elif len(angles2) > len(angles1):
        angles2 = angles2[:len(angles1)]
    # print(len(angles1),len(angles2))

    vec1, vec2, vec3, curA1, curA2, curX, curY = calcPlotVectorsScara(angles1, angles2)

    # angles1 = np.array([k[1] for k in motor1.measurements])
    # angles2 = np.array([k[1] for k in motor2.measurements])
    # if len(angles1) == len(angles2):
    # vec1 = np.array(vec1)
    # vec2 = np.array(vec2)

    figSub1.clear()
    figSub1.scatter(vec1, vec2, c=vec3, cmap=cmap)
    figSub1.title.set_text('Shoulder ' + str(round(curA1)) + ", elbow " + str(round(curA2)) + " degrees, xy " + str(round(curX)) + ", " + str(round(curY)))

cmap = LinearSegmentedColormap.from_list('rg', ["r", "w", "g"], N=256)
ani = animation.FuncAnimation(fig, animate, interval=100)

fig.canvas.mpl_connect('key_press_event', key_handler)
plt.show()

# keyboard.wait('esc')

motor1.stopReader()
motor2.stopReader()

