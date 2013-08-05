import os
import glob
import time
     
os.system('sudo modprobe w1-gpio')
os.system('sudo modprobe w1-therm')
     
base_dir = '/sys/bus/w1/devices/'
device_folders = glob.glob(base_dir + '28*')
print device_folders
device1_file = device_folders[0] + '/w1_slave'
device2_file = device_folders[1] + '/w1_slave'

     
def read_temp_raw(device_file):
	f = open(device_file, 'r')
	lines = f.readlines()
	f.close()
	return lines
     
def read_temp(name, device_file):
	lines = read_temp_raw(device_file)
	print "file = " + device_file + ":"
	print lines
	if lines[0].strip()[-3:] != 'YES':
		print "temp = INVALID"
	else:
		#time.sleep(0.2)
		#lines = read_temp_raw(device_file)
		equals_pos = lines[1].find('t=')
		if equals_pos != -1:
			temp_string = lines[1][equals_pos+2:]
			temp_c = float(temp_string) / 1000.0
			temp_f = temp_c * 9.0 / 5.0 + 32.0
			print name + " temp = " + str(temp_c) + " *C"

while True:
	read_temp("sensor1", device1_file)
	read_temp("sensor2", device2_file)
#	os.system("sudo ./../dht_driver/Adafruit_DHT 2302 18")
	print "sleeping for 5 secs..."
	print
	
	time.sleep(5)
