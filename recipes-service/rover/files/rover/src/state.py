import logging as log
import time
import os
import sys
import functools
import json
import zmq
import traceback
import random
import serial
from dataclasses import dataclass, field

#a simple task scheduler class because I don't want to use Timer to schedule tasks
class Task:
	def __init__(self, period, callback, random_precharge=False, variance_ratio=0):
		self.period = period
		self.callback = callback
		self.last_update = time.monotonic()
		self.variance_ratio = variance_ratio
		if (random_precharge == True):
			self.last_update = self.last_update - random.uniform(0, self.period)
	def reset(self):
		self.last_update = 0
	def run(self):
		if ((time.monotonic() - self.last_update) > self.period):
			self.last_update = time.monotonic()
			if (self.variance_ratio > 0):
				self.last_update += random.uniform(-1*self.period*self.variance_ratio, self.period*self.variance_ratio)
			self.callback()

class SSC32:
	def __init__(self, path, baud, timeout):
		self.path = path
		self.baud = baud
		self.timeout = timeout
		log.info("%s %s %s" % (path, baud, timeout))
		self.ser = serial.Serial(self.path,self.baud,timeout=self.timeout)
	
	def write(self, b : str):
		cmd = (b + '\r').encode()
		self.ser.write(cmd)
	
	def read(self):
		return self.ser.readline()
	
	def move(self, ch : int, pulse : int, time_ms : int = 250):
		cmd = '#%d P%d T%d' % (ch, pulse, time_ms)
		#log.debug('move "%s"' % (cmd,))
		self.write(cmd)

class PanTilt:
	def __init__(self, ssc32, config):
		self.ssc32 = ssc32
		self.config = config
		self.panh_pos = 1500
		self.panv_pos = 1500
		self.pan_ch = config['pan_ch']
		self.tilt_ch = config['tilt_ch']

	def reset(self):
		self.ssc32.move(self.pan_ch, 1500)
		self.ssc32.move(self.tilt_ch, 1500)

	def panh(self, amt):
		self.panh_pos = max(575, min(self.panh_pos + amt, 2475))
		#log.debug('panh %d' % (self.panh_pos,))
		self.ssc32.move(self.pan_ch, self.panh_pos, time_ms=500)

	def panv(self, amt):
		self.panv_pos = max(1250, min(self.panv_pos + amt, 2425))
		#log.debug('panv %d' % (self.panv_pos,))
		self.ssc32.move(self.tilt_ch, self.panv_pos, time_ms=500)

class Rover:
	def __init__(self, ssc32, config):
		self.ssc32 = ssc32
		
		self.config = config
		self.increment_amount = 20

		self.left_ch = self.config['left_ch']
		self.right_ch = self.config['right_ch']
		self.left = 1500 + self.config['left_offset_us']
		self.right = 1500 + self.config['right_offset_us']

		self.max_limits = [1700,1825,2000]
		self.min_limits = [1300,1175,1000]

		self.max_limit = lambda : self.max_limits[0]
		self.min_limit = lambda : self.min_limits[0]

		self.initialized = False
		self.init()

	def set_motors(self,left=1500,right=1500):
		l = left + self.config['left_offset_us']
		r= right+ self.config['right_offset_us']
		l = max(self.min_limit(), min(l, self.max_limit()))
		r = max(self.min_limit(), min(r, self.max_limit()))
		self.ssc32.write('#%d P%d #%d P%d' % (self.left_ch, l, self.right_ch, r))

	def check(self):
		self.ssc32.write('R4')
		val = self.ssc32.read()
		try:
			val = int(val.decode().rstrip('\r') + '0')
			if (val == self.ssc32.baud):
				#log.debug(val)
				return True
			else:
				return False
		except BaseException as ex:
			pass
		self.initialized = False
		return False		

	def init(self):
		if (not self.check()):
			return
		
		self.stop()
		self.initialized = True
		return True

	def stop(self):
		log.debug('Rover.stop')
		self.left = 1500
		self.right = 1500
		self.set_motors(1500, 1500)

	def turn_left(self):
		log.debug('Rover.turn_left')
		self.left += self.increment_amount
		self.right -= self.increment_amount
		self.set_motors(self.left, self.right)

	def turn_right(self):
		log.debug('Rover.turn_right')
		self.left -= self.increment_amount
		self.right += self.increment_amount
		self.set_motors(self.left, self.right)

	def forward(self):
		log.debug('Rover.forward')
		self.left -= self.increment_amount
		self.right -= self.increment_amount
		self.set_motors(self.left, self.right)

	def change_limits(self):
		prev_max = self.max_limits.pop(0)
		prev_min = self.min_limits.pop(0)
		self.max_limits.append(prev_max)
		self.min_limits.append(prev_min)
		log.info('limits changed to [%d,%d]' % (self.min_limit(), self.max_limit()))	

	def backward(self):
		log.debug('Rover.backward')
		self.left += self.increment_amount
		self.right += self.increment_amount
		self.set_motors(self.left, self.right)

@dataclass
class KeyPress:
	key : str
	pressed : bool
	held : bool
	released : bool
	src : int
	dst : int
	expires : float
	first_use : bool = True

	def expired(self):
		return time.monotonic() > self.expires

	def hold(self):
		self.pressed = False
		self.released = False
		self.held = True

	@staticmethod
	def from_dict(d):
		return KeyPress(key = d['key'],
			pressed = d['pressed'],
			held = d['held'],
			released = d['released'],
			src = d['src'],
			dst = d['dst'],
			expires = d['expires']/1000 + time.monotonic()
		)

class State:
	def __init__(self, rover, pantilt, config, args):
		self.zmq_ct = None
		self.push = None
		self.rover = rover
		self.pantilt = pantilt
		self.machine_stopped = False
		self.config = config
		self.args = args
		self.stopped = False
		
		self.last_key = None
		
		self.ADVERTISMENT_RATE = lambda : self.config['advertisment_rate']
		
		self.advertisment_task = Task(self.ADVERTISMENT_RATE(), self.advertise_self)
		self.handle_key_press_task = Task(0.25, self.handle_key_press)
		self.check_task = Task(2, self.rover.check)
		
		self.pan_amt = 75
		
		def stand():
			self.hexapod.stand()
			self.pantilt.reset()
			
		
		self.key_pressed_actions = {
			'w' 	: lambda : self.rover.forward(),
			'a' 	: lambda : self.rover.turn_left(),
			's' 	: lambda : self.rover.backward(),
			'd' 	: lambda : self.rover.turn_right(),
			'left'	: lambda : self.pantilt.panh(-self.pan_amt),
			'right'	: lambda : self.pantilt.panh(self.pan_amt),
			'down'	: lambda : self.pantilt.panv(-self.pan_amt),
			'up'	: lambda : self.pantilt.panv(self.pan_amt),
			'space' : lambda : self.rover.change_limits(),
			'enter' : lambda : self.stand()
		}
		self.key_released_actions = {
			'w'		: lambda : self.rover.stop(),
			's'		: lambda : self.rover.stop(),
			'a'		: lambda : self.rover.stop(),
			'd'		: lambda : self.rover.stop(),
			'left'	: lambda : self.pantilt.panh(0),
			'right'	: lambda : self.pantilt.panh(0),
			'down'	: lambda : self.pantilt.panv(0),
			'up'	: lambda : self.pantilt.panv(0),
		}
		self.key_held_actions = {
			'w' 	: lambda : self.rover.forward(),
			'a' 	: lambda : self.rover.turn_left(),
			's' 	: lambda : self.rover.backward(),
			'd' 	: lambda : self.rover.turn_right(),
			'left'	: lambda : self.pantilt.panh(-self.pan_amt),
			'right'	: lambda : self.pantilt.panh(self.pan_amt),
			'down'	: lambda : self.pantilt.panv(-self.pan_amt),
			'up'	: lambda : self.pantilt.panv(self.pan_amt),
		}
	
	def stop(self):
		self.stopped = True
	
	def handle_key_press(self):
		if (self.last_key is None):
			if (self.machine_stopped == False):
				self.machine_stopped = True
				self.rover.stop()
			return
		self.machine_stopped = False
		if (self.last_key.expired()) and (self.last_key.first_use == True):
			self.last_key.first_use = False
		elif (self.last_key.expired()):
			log.debug('expired %s' % (self.last_key,))
			self.last_key = None
			self.rover.stop()
			return
		
		if (self.last_key.pressed == True) and (self.last_key.key in self.key_pressed_actions):
			self.key_pressed_actions[self.last_key.key]()
			self.last_key.hold()
		elif (self.last_key.held == True) and (self.last_key.key in self.key_held_actions):
			self.key_held_actions[self.last_key.key]()
		elif (self.last_key.released == True) and (self.last_key.key in self.key_released_actions):
			self.key_released_actions[self.last_key.key]()
			self.last_key.hold()
	
	def handle_sub(self,cmd, msg):
		if (cmd == 'key_press'):
			log.debug('%s, %s' % (cmd, msg))
			self.last_key = KeyPress.from_dict(msg)
	
	def advertise_self(self):
		to_send = {
			'type' : 'rover',
			'dst' : 0
		}
		#log.debug('state.advertise_self %s' % (to_send,))
		self.push.send_json(to_send)
	
	def run(self):
		self.handle_key_press_task.run()
		self.advertisment_task.run()
		self.check_task.run()
		if (not self.rover.initialized):
			self.rover.init()

