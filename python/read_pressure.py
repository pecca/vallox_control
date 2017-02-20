
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
            return round(self.sum / (len(self.valueList) * 1.0), 2) + 1000000
        
filterTimeInSec = 20.0
measRateInHz = 1.25
sendIntervalInSec = 3.0

measIntervalInSec = 1.0/measRateInHz;

avgFilter = AvgFilter(int(filterTimeInSec / measIntervalInSec)) 

UDP_IP = "vallox.ddns.net"
UDP_PORT = 8056

#bmp280 = BMP280('oversampling_x1',
#                'oversampling_x1',
#                'forced',
#                '4000ms',
#                'off')



#    bmp280 = BMP280('oversampling_x16',
#                'oversampling_x2',
#                'normal',
#                '4000ms',
#                'coeff_16')


sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP

sendTimer = 0.0
                     
while(True):
    bmp280 = BMP280('oversampling_x16',
                'oversampling_x2',
                'normal',
                '4000ms',
                'coeff_16')


    pressure,temperature = bmp280.ReadPressAndTemp()
    avgFilter.Update(pressure)
    #print pressure, avgFilter.GetValue()
    if sendTimer > sendIntervalInSec:
        msg =  {'set' : { 'control_var' : { 'pressureIn': avgFilter.GetValue()}}}
        #print pressure,  avgFilter.GetValue()
        sock.sendto(json.dumps(msg), (UDP_IP, UDP_PORT))
        sendTimer = 0.0
    time.sleep(measIntervalInSec)
    sendTimer += measIntervalInSec;
