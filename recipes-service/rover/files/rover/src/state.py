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
import struct
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
	
	def move(self, ch : int, pulse : int, time_ms : int = 250, block=False, block_timeout=3):
		cmd = '#%d P%d T%d' % (ch, pulse, time_ms) if (time_ms is not None) else '#%d P%d' % (ch, pulse)
		#log.debug('move "%s"' % (cmd,))
		self.write(cmd)
		if (block == True):
			start = time.monotonic()
			while ((time.monotonic() - start) < block_timeout):
				self.write('Q')
				if (self.read() == b'.'):
					break
	
	def multi_move(self, chs : list, pulses : list, time_ms : int = 250, block=False, block_timeout=3):
		assert(len(chs) == len(pulses))
		cmd = ''
		for ch, pulse in zip(chs, pulses):
			cmd = cmd + '#%d P%d ' % (ch, pulse)
		if (time_ms is not None):
			cmd = cmd + 'T%d' % (time_ms,)
		self.write(cmd)
		if (block == True):
			start = time.monotonic()
			while ((time.monotonic() - start) < block_timeout):
				self.write('Q')
				if (self.read() == b'.'):
					break

	def read_analog(self):
		self.write('VA VB VC VD')
		ad = self.read()
		if (len(ad) == 4):
			ad = struct.unpack('<BBBB',ad)
			return {'VA':ad[0],'VB':ad[1],'VC':ad[2],'VD':ad[3]}
		return None

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

class Rover:
	def __init__(self, ssc32, config):
		self.ssc32 = ssc32
		
		self.config = config

		self.fwd_ch 	= self.config['fwd_ch']
		self.turn_ch 	= self.config['turn_ch']
		self.fwd = 1500
		self.turn = 1500
		self.left = 1500 + self.config['fwd_offset']
		self.right = 1500 + self.config['turn_offset']

		self.max_limits 		= [1800,1825,2000]
		self.min_limits 		= [1200,1175,1000]
		self.increment_amounts 	= [  25,  50,  75]

		self.max_limit = lambda : self.max_limits[0]
		self.min_limit = lambda : self.min_limits[0]
		self.increment_amount = lambda : self.increment_amounts[0]


	def update_motors(self, fwd=None, turn=None):
		if (fwd is not None) and (turn is not None):
			self.fwd = fwd
			self.turn = turn
		f = self.fwd + self.config['fwd_offset']
		t = self.turn+ self.config['turn_offset']
		f= max(self.min_limit(), min(f, self.max_limit()))
		t = max(self.min_limit(), min(t, self.max_limit()))
		self.ssc32.write('#%d P%d #%d P%d' % (self.fwd_ch, f, self.turn_ch, t))	

	def stop(self):
		log.debug('Rover.stop')
		self.ssc32.write('#%d P0' % (self.fwd_ch,))
		self.ssc32.write('#%d P0' % (self.turn_ch,))
		#self.ssc32.write('#%d P1500 %c' % (self.left_ch,27))
		#self.ssc32.write('#%d P1500 %c' % (self.right_ch,27))
		self.fwd = 1500
		self.turn = 1500
		self.update_motors()

	def change_limits(self):
		prev_max = self.max_limits.pop(0)
		prev_min = self.min_limits.pop(0)
		prev_inc = self.increment_amounts.pop(0)
		self.max_limits.append(prev_max)
		self.min_limits.append(prev_min)
		self.increment_amounts.append(prev_inc)
		log.info('limits changed to [%d,%d,%d]' % (self.min_limit(), self.max_limit(), self.increment_amount()))	

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
	def __init__(self, config, ssc32):
		self.zmq_ct = None
		self.push = None
		self.config = config
		self.stopped = False
		self.initialized = False
		
		self.ssc32 = ssc32
		self.rover = Rover(ssc32, config)
		self.pantilt = PanTilt(ssc32, config)
		
		self.active_keys = {}
		
		self.ADVERTISMENT_RATE = lambda : self.config['advertisment_rate']
		
		self.last_key_press = time.monotonic()
		self.check_task = Task(2, self.check)
		self.advertisment_task = Task(self.ADVERTISMENT_RATE(), self.advertise_self)
		self.handle_key_press_task = Task(0.05, self.handle_key_presses)
		
		self.controller = Controller(self.ssc32, self.rover, self.pantilt)

		self.check()

	def check(self):
		self.ssc32.write('R4')
		val = self.ssc32.read()
		try:
			val = int(val.decode().rstrip('\r') + '0')
			#if (val == self.ssc32.baud):
			if (not self.initialized):
				self.initialized = True
				self.init()
			return True
			#else:
			#	log.warning('no ssc32')
			#	return False
		except BaseException as ex:
			log.error('no ssc32 %s' % (ex,))
		self.initialized = False
		return False
		
	def init(self):
		log.info('state.init()')
		self.rover.stop()
		self.pantilt.reset()
		return True
	
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

