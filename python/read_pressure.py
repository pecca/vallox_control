
import time
import socket
import time
import json
import numpy as np
from pprint import pprint
from BMP280 import *

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

 
UDP_IP = "127.0.0.1"
UDP_PORT = 8056

bmp280 = BMP280()

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
                     
while(True):
    pressure,temperature = ReadPressAndTemp.readBME280All()
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
