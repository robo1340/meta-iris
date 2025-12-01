
#pyserial library is required for working
import serial
import sys
from serial.serialutil import SerialException
import pynmea2
import json
import time
import os
import logging as log
from logging.handlers import RotatingFileHandler

mport = '/dev/ttySTM1'                     #choose your com port on which you connected your neo 6m GPS
GPS_PATH = '/tmp/gps.json'
DELETION_TIME_S = 30

#a simple task scheduler class because I don't want to use Timer to schedule tasks
class Task:
	def __init__(self, period, callback):
		self.period = period
		self.callback = callback
		self.last_update = time.monotonic()
	def run(self):
		if ((time.monotonic() - self.last_update) > self.period):
			self.last_update = time.monotonic()
			self.callback()

def configure_logging(verbose=True):
	lvl = log.DEBUG if (verbose) else log.INFO
	handler = RotatingFileHandler('/var/log/gps.log', maxBytes=10*10000, backupCount=5)
	log.basicConfig(handlers=(handler,), level=lvl)

def configure_logging_commandline(verbose=True):
	lvl = log.DEBUG if (verbose) else log.INFO
	log.basicConfig(level=lvl)

def parseGPS(s):
	s = s.decode()
	#print(s)
	#return
	if 'GGA' in s:
	#if 'GLL' in s:
		msg = pynmea2.parse(s)
		log.debug("Timestamp: %s -- Lat: %s %s %s -- Lon: %s %s %s -- Altitude: %s %s" % (msg.timestamp,msg.latitude,msg.lat,msg.lat_dir,msg.longitude,msg.lon,msg.lon_dir,msg.altitude,msg.altitude_units))
		return (msg.timestamp, msg.latitude, msg.longitude, msg.altitude, msg.altitude_units, msg.gps_qual, msg.num_sats)

def safe_parseGPS(s):
	try:
		return parseGPS(s)
	except BaseException as ex:
		print(ex)
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

def delete_known_hosts():
	k = '/home/root/.ssh/known_hosts'
	if (os.path.isfile(k)):
		os.remove(k)

if __name__=="__main__":
	if ((len(sys.argv) > 1) and (sys.argv[1] == '-F')): #log everything to stdout
		configure_logging_commandline(verbose=True)
	else:
		configure_logging(verbose=False)
	
	log.info('GPS Service Starting')
	delete(GPS_PATH)
	last_update = -1
	delete_known_hosts = Task(30, delete_known_hosts)
	
	while True:
		try:
			while True:
				rc = safe_parseGPS(ser.readline())
				if (rc is not None):
					(timestamp,lat,lon,alt,alt_units,gps_qual,num_sats) = rc
					if (gps_qual != 0):
						if (last_update < 0):
							log.info('Lock: %s' % (rc,))
						last_update = time.monotonic()
						write(GPS_PATH, [lat,lon])
				elif (last_update > 0) and ((time.monotonic() - last_update) > DELETION_TIME_S):
					delete(GPS_PATH)
					last_update = -1
					log.info('Lock Lost')
				#delete_known_hosts.run()
				time.sleep(1)
		except KeyboardInterrupt:
			print("stopping")
			break
		except SerialException as ex:
			print(ex)
		#except BaseException as ex:
		#	print(ex)
