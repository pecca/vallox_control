#!/usr/bin/python
# Copyright (c) 2014 Adafruit Industries
# Author: Tony DiCola
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Can enable debug output by uncommenting:
#import logging
#logging.basicConfig(level=logging.DEBUG)

import Adafruit_BMP.BMP085 as BMP085
import time
import socket
import time
import json
import numpy as np
from pprint import pprint
from bme280 import readBME280ID, readBME280All

def runningMeanFast(x, N):
    return np.convolve(x, np.ones((N,))/N)[(N-1):]

class AvgFilter:
    def __init__(self, size):
        self.size = size
        self.sum = 0
        self.count = 0
        self.valueList = []
    def GetCount(self):
        return self.count
    def Update(self, value):
        value -= 1000000
        if self.count < self.size:
            self.valueList.append(value)
            self.sum += value  
        else:
            self.sum -= self.valueList.pop(0)
            self.sum += value
            self.valueList.append(value)      
        self.count += 1    
    def GetValue(self):
        if self.count == 0:
            return 0
        else:
            return round(self.sum / (len(self.valueList) * 1.0), 1) + 1000000
    
    
time.sleep(1.0)

avgFilter = AvgFilter(2000) 

# Default constructor will pick a default I2C bus.
#
# For the Raspberry Pi this means you should hook up to the only exposed I2C bus
# from the main GPIO header and the library will figure out the bus number based
# on the Pi's revision.
#
# For the Beaglebone Black the library will assume bus 1 by default, which is
# exposed with SCL = P9_19 and SDA = P9_20.
#sensor = BMP085.BMP085()

# Optionally you can override the bus number:
#sensor = BMP085.BMP085(busnum=2)

# You can also optionally change the BMP085 mode to one of BMP085_ULTRALOWPOWER,
# BMP085_STANDARD, BMP085_HIGHRES, or BMP085_ULTRAHIGHRES.  See the BMP085
# datasheet for more details on the meanings of each mode (accuracy and power
# consumption are primarily the differences).  The default mode is STANDARD.
#sensor = BMP085.BMP085(mode=BMP085.BMP085_ULTRAHIGHRES)

 
UDP_IP = "127.0.0.1"
UDP_PORT = 8056

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP

(chip_id, chip_version) = readBME280ID()
print "Chip ID     :", chip_id
print "Version     :", chip_version

temperature,pressure,humidity = readBME280All()

print "Temperature : ", temperature, "C"
print "Pressure : ", pressure, "hPa"
print "Humidity : ", humidity, "%"
                     
while(True):
    #pressure = sensor.read_pressure()
    temperature,pressure,humidity = readBME280All()
    pressure = pressure * 100
    avgFilter.Update(pressure)
    if avgFilter.GetCount() % 50 == 0:
        #print('Temp = {0:0.2f} *C'.format(sensor.read_temperature()))
        #print('Pressure = {0:0.2f} Pa'.format(sensor.read_pressure()))
    #print('Altitude = {0:0.2f} m'.format(sensor.read_altitude()))
    #print('Sealevel Pressure = {0:0.2f} Pa'.format(sensor.read_sealevel_pressure()))
        msg =  {'set' : { 'control_var' : { 'pressureOut': avgFilter.GetValue()}}}
        #print round(pressure, 1),  avgFilter.GetValue()
        #print avgFilter.valueList, avgFilter.sum 
        sock.sendto(json.dumps(msg), (UDP_IP, UDP_PORT))
    time.sleep(0.04) # 0.04
