import logging as log
import time
import os
import sys
import functools
import json
import zmq
import traceback
import random
from dataclasses import dataclass
from queue import Queue

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
	def cancel(self):
		self.last_update = time.monotonic()
	def run(self):
		if ((time.monotonic() - self.last_update) > self.period):
			self.last_update = time.monotonic()
			if (self.variance_ratio > 0):
				self.last_update += random.uniform(-1*self.period*self.variance_ratio, self.period*self.variance_ratio)
			self.callback()

@dataclass
class KeyPress:
	key: str
	time_pressed: float
	
	def held(self, check_keys_s):
		return (time.monotonic() > (self.time_pressed + check_keys_s))
	
	

class State:
	def __init__(self, config, args):
		self.zmq_ct = None
		self.push = None
		self.config = config
		self.args = args
		self.stopped = False
		self.key_queue = Queue()
		self.keys_pressed = {}
		
		self.CHECK_KEYS_S = lambda : self.config['check_keys_s']
		
		self.check_keys_task = Task(self.CHECK_KEYS_S(), self.check_keys)
		self.EXPIRES_MS = lambda : self.config['expires_ms']
		self.key_pressed_actions = {
			'w' 	: lambda : self.send('w',pressed=True),
			'a' 	: lambda : self.send('a',pressed=True),
			's' 	: lambda : self.send('s',pressed=True),
			'd' 	: lambda : self.send('d',pressed=True),
			'space' : lambda : self.send('space',pressed=True),
			'enter' : lambda : self.send('enter',pressed=True),
			'right' : lambda : self.send('right',pressed=True),
			'left' 	: lambda : self.send('left',pressed=True),
			'down' 	: lambda : self.send('down',pressed=True),
			'up' 	: lambda : self.send('up',pressed=True),
			'f'		: self.cycle_dst_addresses,
			'q'		: self.stop,
			'esc'	: self.stop
		}
		self.key_released_actions = {
			'w'		: lambda : self.send('w',released=True),
			's'		: lambda : self.send('s',released=True),
			'a'		: lambda : self.send('a',released=True),
			'd'		: lambda : self.send('d',released=True),
			'right' : lambda : self.send('right',released=True),
			'left' 	: lambda : self.send('left',released=True),
			'down' 	: lambda : self.send('down',released=True),
			'up' 	: lambda : self.send('up',released=True),
		}
		self.key_held_actions = {
			'w'		: lambda : self.send('w',held=True),
			's'		: lambda : self.send('s',held=True),
			'a'		: lambda : self.send('a',held=True),
			'd'		: lambda : self.send('d',held=True),	
			'right' : lambda : self.send('right',held=True),
			'left' 	: lambda : self.send('left',held=True),
			'down' 	: lambda : self.send('down',held=True),
			'up' 	: lambda : self.send('up',held=True),
		}
		self.dst_addresses = [0]
	
	def stop(self):
		self.stopped = True
	
	def cycle_dst_addresses(self):
		prev = self.dst_addresses.pop(0)
		self.dst_addresses.append(prev)
		log.info('     |')
		log.info('     V')
		log.info(['0x%04X' % x for x in self.dst_addresses])
		log.info('Address 0x%04x selected (prev 0x%04x)' % (self.dst_addresses[0], prev))
	
	def send(self, key, pressed=False, held=False, released=False):
		to_send = {
			'dst'		: self.dst_addresses[0],
			'type' 		: 'key_press',
			'expires' 	: self.EXPIRES_MS(),
			'pressed'	: pressed,
			'held'		: held,
			'released'	: released,
			'key'		: key
		}
		log.debug('state.send %s' % (to_send,))
		if (self.args['send_local'] == True):
			to_send['src'] = 0
			self.push.send_string('key_press', flags=zmq.SNDMORE)
			self.push.send_json(to_send)
		else:
			self.push.send_json(to_send)
	
	def check_keys(self):
		if (len(self.keys_pressed) > 0):
			for key in self.keys_pressed:
				if (key in self.key_held_actions) and (self.keys_pressed[key].held(self.CHECK_KEYS_S())):
					self.key_held_actions[key]()
			#log.info(self.keys_pressed)
	
	def read_key_presses(self):
		while (not self.key_queue.empty()):
			(action, key) = self.key_queue.get_nowait()
			now = time.monotonic()
			if (action == 'pressed'):
				self.keys_pressed[key] = KeyPress(key, now)
				if (key in self.key_pressed_actions):
					self.key_pressed_actions[key]()
					self.check_keys_task.cancel()
			elif (action == 'released'):
				if (key in self.keys_pressed):
					self.keys_pressed.pop(key)
				if (key in self.key_released_actions):
					self.key_released_actions[key]()
					self.check_keys_task.cancel()
			#log.info('%s %s' % (action, key))
	
	def handle_sub(self, cmd, msg):
		#log.debug('%s, %s' % (cmd, msg))
		if (cmd == 'hexapod') and (msg['src'] not in self.dst_addresses):
			log.info('New Hexapod found at address 0x%04X' % (msg['src'],))
			self.dst_addresses.append(msg['src'])
	
	def run(self):
		self.read_key_presses()
		self.check_keys_task.run()

