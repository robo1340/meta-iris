
#pyserial library is required for working
import serial
import json
import time
import os

mport = '/dev/ttySTM1'                     #choose your com port on which you connected your neo 6m GPS
GPS_PATH = '/tmp/gps.json'
DELETION_TIME_S = 30

def decode(coord):
	l = list(coord)
	for i in range(0,len(l)-1):
			if l[i] == "." :
					break
	base = l[0:i-2]
	degi = l[i-2:i]
	degd = l[i+1:]
	#print(base,"   ",degi,"   ",degd)
	baseint = int("".join(base))
	degiint = int("".join(degi))
	degdint = float("".join(degd))
	degdint = degdint / (10**len(degd))
	degs = degiint + degdint
	full = float(baseint) + (degs/60)
	#print(full)
	return full

def parseGPS(data):
	#print(data)
	try:
		data = data.decode()
	except BaseException as ex:
		print(ex)
		return None
	
	if data.startswith("$GPGGA"):
		s = data.split(",")
		if s[7] == '0' or s[7]=='00':
			#print ("no satellite data available")
			return None
		time = s[1][0:2] + ":" + s[1][2:4] + ":" + s[1][4:6]
		#print("-----------")
		lat = decode(s[2])
		lon = decode(s[4])
		return  [lat,lon]
	else:
		return None

ser = serial.Serial(mport,9600,timeout = 2)

def delete(path):
	if (os.path.isfile(GPS_PATH)):
		os.remove(GPS_PATH)
	
def write(path, contents):
	try:
		with open(path,'w') as f:
			json.dump(contents, f)
	except BaseException as ex:
		print(ex)

if __name__=="__main__":
	delete(GPS_PATH)
	last_update = -1
	try:
		while True:
			coords = parseGPS(ser.readline())
			if (coords is not None):
				last_update = time.monotonic()
				write(GPS_PATH, coords)
			elif ((time.monotonic() - last_update) > DELETION_TIME_S):
				delete(GPS_PATH)
	except KeyboardInterrupt:
		print("stopping")
