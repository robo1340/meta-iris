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
import itertools

from math import acos
from math import sin
from math import cos
from math import radians
from math import degrees
from math import atan2
from math import sqrt
from math import pi
from math import atan

from dataclasses import dataclass, field

from controller import Controller

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
	def now(self):
		self.last_update = 0
		self.run()
	def run(self):
		if ((time.monotonic() - self.last_update) > self.period):
			self.last_update = time.monotonic()
			if (self.variance_ratio > 0):
				self.last_update += random.uniform(-1*self.period*self.variance_ratio, self.period*self.variance_ratio)
			self.callback()	

class PanTilt:
	def __init__(self, ssc32, config):
		self.ssc32 = ssc32
		self.config = config
		self.pan_ch = config['pan_ch']
		self.tilt_ch = config['tilt_ch']
		self.pan_offset = config['pan_offset']
		self.tilt_offset = config['tilt_offset']
		self.panh_pos = 1500
		self.panv_pos = 1500
		self.pan_limits = [575, 2075]
		self.tilt_limits = [1050, 2425]

	def set_position(self, pan, tilt, time_ms=None, block=False, block_timeout=3):
		self.panh_pos = max(self.pan_limits[0], min(pan+self.pan_offset, self.pan_limits[1]))
		self.panv_pos = max(self.tilt_limits[0], min(tilt+self.tilt_offset, self.tilt_limits[1]))
		self.ssc32.multi_move([self.pan_ch,self.tilt_ch], [self.panh_pos, self.panv_pos], time_ms=time_ms, block=block, block_timeout=block_timeout)	
	
	def reset(self):
		self.set_position(1500, 1500)

	def cancel_pan(self):
		self.ssc32.write('#%d P1500 %c' % (self.pan_ch,27))
		
	def cancel_tilt(self):
		self.ssc32.write('#%d P1500 %c' % (self.tilt_ch,27))

	def panh(self, amt, time_ms=250, block=False, block_timeout=3):
		self.panh_pos = max(self.pan_limits[0], min(self.panh_pos + amt, self.pan_limits[1]))
		self.ssc32.move(self.pan_ch, self.panh_pos, time_ms=time_ms, block=block, block_timeout=block_timeout)

	def panv(self, amt, time_ms=250, block=False, block_timeout=3):
		self.panv_pos = max(self.tilt_limits[0], min(self.panv_pos + amt, self.tilt_limits[1]))
		self.ssc32.move(self.tilt_ch, self.panv_pos, time_ms=time_ms, block=block, block_timeout=block_timeout)

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
	action_taken : bool = False

	def expired(self):
		return time.monotonic() > self.expires

	def used(self):
		if (self.action_taken == False):
			self.action_taken = True
			return False
		return True

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
	def __init__(self, config, args, ssc32):
		self.zmq_ct = None
		self.push = None
		self.config = config
		self.args = args
		self.stopped = False
		self.initialized = False
		
		self.ssc32 = ssc32
		
		self.active_keys = {}
		
		self.ADVERTISMENT_RATE = lambda : self.config['advertisment_rate']
		
		self.last_key_press = time.monotonic()
		self.check_task = Task(2, self.check)
		self.advertisment_task = Task(self.ADVERTISMENT_RATE(), self.advertise_self)
		self.handle_key_press_task = Task(0.05, self.handle_key_presses)
		
		self.pantilt = PanTilt(self.ssc32, config['pantilt'])
		
		self.controller = Controller(self.ssc32, self.pantilt)

		self.check()

	def check(self):
		self.ssc32.write('R4')
		val = self.ssc32.read()
		try:
			val = int(val.decode().rstrip('\r') + '0')
			if (val == self.ssc32.baud):
				if (not self.initialized):
					self.initialized = True
					self.init()
				return True
			else:
				log.warning('no ssc32')
				return False
		except BaseException as ex:
			log.error('no ssc32')
			pass
			#log.warning(ex)
		self.initialized = False
		return False
		
	def init(self):
		log.info('state.init()')
		self.controller.sit()
		self.controller.stand()
		self.pantilt.reset()
	
	def stop(self):
		self.stopped = True
	
	def handle_key_presses(self):
		if (len(self.active_keys) == 0) and (self.last_key_press is not None) and (time.monotonic() > (self.last_key_press+10)):
			self.last_key_press = None
			self.controller.handle_idle_long()
		for key in self.active_keys.values():
			if (key.expired()) and (key.first_use == True):
				key.first_use = False
			if(key.expired()):#elif (key.expired()):
				#log.debug('%s expired' % (key.key,))
				self.active_keys.pop(key.key)
				if (len(self.active_keys) == 0):
					self.controller.handle_idle()
				return
			
			self.last_key_press = time.monotonic()
			if (key.pressed == True):
				#log.debug('%s pressed' % (key.key,))
				self.controller.handle_key_pressed(key.key)
			elif (key.held == True):
				#log.debug('%s held' % (key.key,))
				self.controller.handle_key_held(key.key)
			elif (key.released == True):
				#log.debug('%s released' % (key.key,))
				self.controller.handle_key_released(key.key)
	
	def handle_sub(self,cmd, msg):
		if (cmd == 'key_press'):
			#log.debug('%s, %s' % (cmd, msg))
			new = KeyPress.from_dict(msg)
			self.active_keys[msg['key']] = new
			if (new.pressed == True)or(new.released==True):
				self.handle_key_press_task.now()
	
	def advertise_self(self):
		to_send = {
			'type' : 'hexapod',
			'dst' : 0
		}
		#log.debug('state.advertise_self %s' % (to_send,))
		self.push.send_json(to_send)
	
	def run(self):
		self.handle_key_press_task.run()
		self.controller.run()
		self.check_task.run()
		#self.controller.run_alt()
		
		#self.advertisment_task.run()
		#self.check_task.run()
		#if (not self.hexapod.initialized):
		#	self.hexapod.init()

