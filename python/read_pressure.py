
import time
import socket
import time
import json
import numpy as np
from pprint import pprint
from BMP280 import *



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
        
filterTimeInSec = 2.0
measRateInHz = 25
sendIntervalInSec = 2.0

measIntervalInSec = 1.0/measRateInHz;

avgFilter = AvgFilter(int(filterTimeInSec / measIntervalInSec)) 

UDP_IP = "vallox.ddns.net"
UDP_PORT = 8056

bmp280 = BMP280()

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP

sendTimer = 0.0
                     
while(True):
    pressure,temperature = bmp280.ReadPressAndTemp()
    avgFilter.Update(pressure)
    if sendTimer > sendIntervalInSec:
        msg =  {'set' : { 'control_var' : { 'pressureOut': avgFilter.GetValue()}}}
        print round(pressure, 1),  avgFilter.GetValue()
        sock.sendto(json.dumps(msg), (UDP_IP, UDP_PORT))
        sendTimer = 0.0
    time.sleep(measIntervalInSec)
    sendTimer += measIntervalInSec;
