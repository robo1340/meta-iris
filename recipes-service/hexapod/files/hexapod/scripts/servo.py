
#pyserial library is required for working
import serial
import sys
from serial.serialutil import SerialException
import json
import time
import os
import logging as log
from logging.handlers import RotatingFileHandler
from sshkeyboard import listen_keyboard, stop_listening
import functools


stopped = False

# Function to execute when a key is pressed
def on_key_press(ssc32, key):
	global stopped
	log.debug(f"Key '{key}' pressed")
	# You can add your desired logic here based on the key pressed
	
	if (key == 'w'):
		ssc32.increment()
	elif (key == 's'):
		ssc32.decrement()
	elif (key == 'd'):
		ssc32.next()
	elif (key == 'a'):
		ssc32.prev()
	elif key == "q" or key == "esc":
		stopped = True
		print("Exiting key detection.")
		stop_listening() # Stop listening for key presses

# Function to execute when a key is released (optional)
def on_key_release(ssc32, key):
	#log.debug(f"Key '{key}' released")
	pass

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

def delete(path):
	if (os.path.isfile(GPS_PATH)):
		os.remove(GPS_PATH)
	
def write(path, contents):
	try:
		with open(path,'w') as f:
			json.dump(contents, f)
	except BaseException as ex:
		print(ex)

log.basicConfig(level=log.DEBUG)

class SSC32:
	def __init__(self, path, baud, timeout):
		self.path = path
		self.baud = baud
		self.timeout = timeout
		log.info("%s %s %s" % (path, baud, timeout))
		self.ser = serial.Serial(self.path,self.baud,timeout=self.timeout)
		#self.ser = serial.Serial(port=self.path, baudrate=self.baud, timeout=self.timeout)
		
		self.write('#2PO80 8PO80')
		
		self.servo_list = [0,1,2,3,4,5,16,17,18,19,20,21,22,23]
		self.servos = {}
		for ch in self.servo_list:
			self.servos[ch] = 1500
		for servo, position in self.servos.items():
			self.move(servo, position)
		self.current_servo = self.servo_list[0]
		self.current_servo_ind = 0
		self.increment_amount = 25
	
	def write(self, b : str):
		cmd = (b + '\r').encode()
		self.ser.write(cmd)
	
	def increment(self):
		c = self.current_servo
		self.servos[c] = self.servos[c] + self.increment_amount
		self.move(c, self.servos[c])
		log.info('servo %d to %d' % (c, self.servos[c]))
	
	def decrement(self):
		c = self.current_servo
		self.servos[c] = self.servos[c] - self.increment_amount
		self.move(c, self.servos[c])
		log.info('servo %d to %d' % (c, self.servos[c]))
	
	def next(self):
		self.current_servo_ind += 1
		if (self.current_servo_ind == len(self.servo_list)):
			self.current_servo_ind = 0
		self.current_servo = self.servo_list[self.current_servo_ind]
		log.info('servo %d selected' % (self.current_servo,))
	
	def prev(self):
		self.current_servo_ind -= 1
		if (self.current_servo_ind < 0):
			self.current_servo_ind = len(self.servo_list)-1
		self.current_servo = self.servo_list[self.current_servo_ind]
		log.info('servo %d selected' % (self.current_servo,))

	def move(self, ch : int, pulse : int, time_ms : int = 250):
		cmd = '#%d P%d T%d' % (ch, pulse, time_ms)
		#log.debug('move "%s"' % (cmd,))
		self.write(cmd)

if __name__=="__main__":
	configure_logging_commandline(verbose=True)
	#if ((len(sys.argv) > 1) and (sys.argv[1] == '-F')): #log everything to stdout
	#	configure_logging_commandline(verbose=True)
	#else:
	#	configure_logging(verbose=False)
	
	mport = '/dev/ttySTM1'
	ssc32 = SSC32(mport, 115200, 2.0)

	time.sleep(1)

	print("Listening for key presses over SSH. Press 'q' or 'esc' to quit.")

	# Start listening for keyboard events
	listen_keyboard(
		on_press=functools.partial(on_key_press, ssc32),
		on_release=functools.partial(on_key_release, ssc32),
		delay_second_char=0.1,
		delay_other_chars=0.05,
		#sequential=True,
		debug=True
	)
	log.info(stopped)
	sys.exit(0)
	
	#hexapod.move(-100,100,150)
	#time.sleep(5)
	#hexapod.stop()
	
	while (stopped == False):
		try:
			while True:
				time.sleep(1)
		except KeyboardInterrupt:
			print("stopping")
			break
		except SerialException as ex:
			print(ex)
		#except BaseException as ex:
		#	print(ex)
	
	print("Exiting key detection.")
	stop_listening() # Stop listening for key presses
	time.sleep(1)
