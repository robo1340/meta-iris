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

class Hexapod:
	def __init__(self, ssc32, config):
		self.ssc32 = ssc32
		#self.ssc32.write('LH1000 LM1400 LL1800 RH2000 RM1600 RL1200 VS3000')
		#self.ssc32.write('LF1700 LR1300 RF1300 RR1700 HT1000')
		
		self.config = config
		self.increment_amount = 20
		self.base_speed = config['base_speed']
		self.max_speed = config['max_speed']
		self.speed = self.base_speed
		self.panh_pos = 1500
		self.panv_pos = 1500
		
		self.initialized = False
		self.init()

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
			#log.warning(ex)
		self.initialized = False
		return False		

	def init(self):
		if (not self.check()):
			return
		
		log.info('Hexapod.init()')
		LH = self.config['LH']     
		LM = self.config['LM']     
		LL = self.config['LL']   
		RH = self.config['RH']      
		RM = self.config['RM']      
		RL = self.config['RL']    
		
		self.ssc32.write('LH%d LM%d LL%d RH%d RM%d RL%d VS3000' % (LH, LM, LL, RH, RM, RL))
		#self.ssc32.write('LH1400 LM1500 LL1800 RH2000 RM1600 RL1200 VS3000')
		self.ssc32.write('LF1700 LR1300 RF1300 RR1700 HT1000')
		
		for servo,pos in self.config['offsets'].items():
			servo = int(servo)
			self.ssc32.write('#%dPO%d' % (servo,pos))
		self.initialized = True
		self.stand()
		return True


	def panh(self, amt):
		self.panh_pos = max(575, min(self.panh_pos + amt, 2475))
		#log.debug('panh %d' % (self.panh_pos,))
		self.ssc32.move(25, self.panh_pos, time_ms=500)

	def panv(self, amt):
		self.panv_pos = max(1250, min(self.panv_pos + amt, 2425))
		#log.debug('panv %d' % (self.panv_pos,))
		self.ssc32.move(24, self.panv_pos, time_ms=500)

	def stand(self):
		log.info('stand')
		L = self.config['LL']
		R = self.config['RL']
		n = 1500
		#self.ssc32.write('#0 P%dS1000 #2 P%dS1000 #4 P%dS1000 #16 P%dS1000 #18 P%dS1000 #20 P%dS1000 #1 P%dS1000 #3 P%dS1000 #5 P%dS1000 #17 P%dS1000 #19 P%dS1000 #21 P%dS1000 T1500' % (R, R, R, L, L, L, n, n, n, n, n, n))
		self.ssc32.write('#0 P%d #2 P%d #4 P%d #16 P%d #18 P%d #20 P%d #1 P%d #3 P%d #5 P%d #17 P%d #19 P%d #21 P%d T1000' % (R, R, R, L, L, L, n, n, n, n, n, n))
		#for ch in [0,1,2,3,4,5,16,17,18,19,20,21,24,25]:
		for ch in [24,25]:
			self.ssc32.move(ch, 1500)

	def lower_legs(self):
		log.info('lower_legs')
		L = self.config['LL']
		R = self.config['RL']
		n = 1500
		self.ssc32.write('#0 P%d #2 P%d #4 P%d #16 P%d #18 P%d #20 P%d T500' % (R, R, R, L, L, L))


	def increment_speed(self):
		self.speed = min(self.speed+self.increment_amount, self.max_speed)
		log.debug('increment_speed %d' % (self.speed,))
		self.ssc32.write('XS %d' % (self.speed,))
	
	def decrement_speed(self):
		self.speed = max(self.speed-self.increment_amount, 0)
		log.debug('decrement_speed %d' % (self.speed,))
		self.ssc32.write('XS %d' % (self.speed,))
		
	def reset_speed(self):
		#log.debug('reset_speed')
		self.speed = self.base_speed
		self.ssc32.write('XS %d' % (self.speed,))
	
	def toggle_speed(self):
		self.speed = 100 if (self.speed == 200) else 200
	
	def move(self, left, right, speed=None):
		if (speed is None):
			speed = self.speed
		cmd = 'XL %d XR %d XS %d' % (left, right, speed)
		#log.debug('Hexapod.move(%s)' % (cmd,))
		self.ssc32.write(cmd)
	
	def stop_sequencer(self):
		self.ssc32.write('XSTOP')
		
	def stop(self):
		log.info('stop')
		self.stop_sequencer()
		self.speed = self.base_speed
		self.lower_legs()

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
	def __init__(self, hexapod, config, args):
		self.zmq_ct = None
		self.push = None
		self.hexapod = hexapod
		self.hex_stopped = False
		self.config = config
		self.args = args
		self.stopped = False
		
		self.last_key = None
		
		self.ADVERTISMENT_RATE = lambda : self.config['advertisment_rate']
		
		self.advertisment_task = Task(self.ADVERTISMENT_RATE(), self.advertise_self)
		self.handle_key_press_task = Task(0.25, self.handle_key_press)
		self.check_task = Task(2, self.hexapod.check)
		
		self.pan_amt = 75
		
		self.key_pressed_actions = {
			'w' 	: lambda : self.hexapod.move(100,100),
			'a' 	: lambda : self.hexapod.move(-100,100),
			's' 	: lambda : self.hexapod.move(-100,-100),
			'd' 	: lambda : self.hexapod.move(100,-100),
			'left'	: lambda : self.hexapod.panh(-self.pan_amt),
			'right'	: lambda : self.hexapod.panh(self.pan_amt),
			'down'	: lambda : self.hexapod.panv(-self.pan_amt),
			'up'	: lambda : self.hexapod.panv(self.pan_amt),
			'space' : lambda : self.hexapod.toggle_speed(),
			'enter' : lambda : self.hexapod.stand()
		}
		self.key_released_actions = {
			'w'		: lambda : self.hexapod.stop_sequencer(),
			's'		: lambda : self.hexapod.stop_sequencer(),
			'a'		: lambda : self.hexapod.stop_sequencer(),
			'd'		: lambda : self.hexapod.stop_sequencer(),
			'left'	: lambda : self.hexapod.panh(0),
			'right'	: lambda : self.hexapod.panh(0),
			'down'	: lambda : self.hexapod.panv(0),
			'up'	: lambda : self.hexapod.panv(0),
		}
		self.key_held_actions = {
			'w'		: lambda : self.hexapod.increment_speed(),
			's'		: lambda : self.hexapod.increment_speed(),	
			'a'		: lambda : self.hexapod.increment_speed(),
			'd'		: lambda : self.hexapod.increment_speed(),
			'left'	: lambda : self.hexapod.panh(-self.pan_amt),
			'right'	: lambda : self.hexapod.panh(self.pan_amt),
			'down'	: lambda : self.hexapod.panv(-self.pan_amt),
			'up'	: lambda : self.hexapod.panv(self.pan_amt),
		}
	
	def stop(self):
		self.stopped = True
	
	def handle_key_press(self):
		if (self.last_key is None):
			if (self.hex_stopped == False):
				self.hex_stopped = True
				self.hexapod.stop()
			return
		self.hex_stopped = False
		if (self.last_key.expired()) and (self.last_key.first_use == True):
			self.last_key.first_use = False
		elif (self.last_key.expired()):
			log.debug('expired %s' % (self.last_key,))
			self.last_key = None
			self.hexapod.stop()
			return
		
		if (self.last_key.pressed == True) and (self.last_key.key in self.key_pressed_actions):
			self.key_pressed_actions[self.last_key.key]()
		elif (self.last_key.held == True) and (self.last_key.key in self.key_held_actions):
			self.key_held_actions[self.last_key.key]()
		elif (self.last_key.released == True) and (self.last_key.key in self.key_released_actions):
			self.key_released_actions[self.last_key.key]()
	
	def handle_sub(self,cmd, msg):
		if (cmd == 'key_press'):
			log.debug('%s, %s' % (cmd, msg))
			self.last_key = KeyPress.from_dict(msg)
	
	def advertise_self(self):
		to_send = {
			'type' : 'hexapod',
			'dst' : 0
		}
		#log.debug('state.advertise_self %s' % (to_send,))
		self.push.send_json(to_send)
	
	def run(self):
		self.handle_key_press_task.run()
		self.advertisment_task.run()
		self.check_task.run()
		if (not self.hexapod.initialized):
			self.hexapod.init()

