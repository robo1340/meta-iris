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

from rotation import xRot
from rotation import yRot
from rotation import zRot

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
			#cmd = cmd + 'S200 T%d' % (time_ms,)
			cmd = cmd + ' T%d' % (time_ms,)
		self.write(cmd)
		if (block == True):
			#log.debug('blocking')
			start = time.monotonic()
			while ((time.monotonic() - start) < block_timeout):
				self.write('Q')
				if (self.read() == b'.'):
					break

	def stop(self, chs : list):
		if (type(chs) is int):
			self.write('STOP %d' % (chs,))
		else: #assume chs is a list or iterable of ints
			for ch in chs:
				self.stop(ch)

	def read_analog(self):
		self.write('VA VB VC VD')
		ad = self.read()
		if (len(ad) == 4):
			ad = struct.unpack('<BBBB',ad)
			return {'VA':ad[0],'VB':ad[1],'VC':ad[2],'VD':ad[3]}
		return None

#def angle_to_pw(deg, umin, umax):
#	num = deg*(umax-umin)
#	return int( (num/180)+umin )

def limits(x, lower, upper):
	#log.debug('%f %f %f %f' % (x, lower, upper, max(lower, min(x, upper)))) 
	return max(lower, min(x, upper))
	
class Servo:
	def __init__(self, ch, angles : list, pws : list, calibration : dict):
		self.ch = ch
		self.angles = angles
		self.pws = pws
		self.lower_angle = angles[0]
		#self.center_angle = angles[1]
		self.upper_angle = angles[2]
		#self.lower_pw = pws[0]
		#self.center_pw = pws[1]
		#self.upper_pw = pws[2]
		
		self.my_limits = lambda angle : limits(angle, self.lower_angle, self.upper_angle)
		
		(self.m, self.b) = self.calibrate(calibration)
	
	def calibrate(self, calibration):
		low_angle = calibration['angles'][0]
		hi_angle  = calibration['angles'][1]
		
		low_pw = calibration['pws'][0]
		hi_pw  = calibration['pws'][1]
		m = (hi_pw - low_pw)/(hi_angle - low_angle)
		b = hi_pw - m*hi_angle	
		return (m, b)
	
	def get_pw(self, desired_angle):
		desired_angle = self.my_limits(desired_angle)
		return self.m * desired_angle + self.b
	
	@staticmethod
	def from_dict(d):
		return Servo(ch=d['ch'], angles=d['angles'], pws=d['pws'], calibration=d['calibration'])

def leg_ik1(x,y,z, sl, el):
	try:		
		#old 2D version
		elbow_angle = -acos(((x**2) + (z**2) - (sl**2) - (el**2))/(2*sl*el))
		shoulder_angle = atan2(z,x) - atan2(el*sin(elbow_angle), (sl + el*cos(elbow_angle)))
		pan_angle = degrees(atan2(x, y))
		pan_angle = -pan_angle #mirror the pan angle
		return (pan_angle, degrees(shoulder_angle), degrees(elbow_angle))
	except ValueError as ex:
		log.warning(ex)
		return None

#make it 3d
def leg_ik2(x,y,z, sl, el):
		try:
			num = (x**2) + (y**2) + (z**2) - (sl**2) - (el**2)
			elbow_angle = -acos(num / (2*sl*el) )
			
			num = -(el**2) + (sl**2) + (x**2) + (y**2) + (z**2)
			den = 2*sl*sqrt((x**2)+(y**2)+(z**2))
			shoulder_angle = acos(num/den) + atan2(z, sqrt((x**2)+(y**2)))
			
			pan_angle = degrees(atan2(x, y))
			pan_angle = -pan_angle #mirror the pan angle
			
			return (pan_angle, degrees(shoulder_angle), degrees(elbow_angle))
		except ValueError as ex:
			log.warning(ex)
			return None	

#account for coxa length
def ik3(bh, sl, el, x, y, z, wrist_offset_wrt_z, cuff_offset_wrt_x):
		try:
			r = sqrt( (z-bh)**2 + x**2 + y**2 )
			A2 = acos ( (r**2 + sl**2 - el**2) / (2*r*sl) )
			A1 = atan2(z-bh, y)
			shoulder_angle = A1 + A2
			
			B1 = acos ( (sl**2 + el**2 - r**2)/(2*sl*el) )
			elbow_angle = pi - B1
			
			pan_angle = atan2(y,x)
			
			#log.info('pan angle %2.1f' % (degrees(pan_angle),))
			A3 = pi/2 - shoulder_angle
			wrist_angle = radians(wrist_offset_wrt_z) - A3 - elbow_angle
			#wrist_angle = pi - A3 - elbow_angle
			wrist_angle = -wrist_angle
			#wrist_angle += pi
			#log.debug('%2.1f %2.1f %2.1f %2.1f' % (wrist_offset_wrt_z, degrees(A3), degrees(elbow_angle), degrees(wrist_angle)) )
			
			cuff_angle = radians(cuff_offset_wrt_x) - pan_angle
			
			return (degrees(pan_angle), degrees(shoulder_angle), degrees(elbow_angle), degrees(wrist_angle), degrees(cuff_angle))
		except ValueError as ex:
			log.warning(ex)
			return None	

class Arm:
	def __init__(self, ssc32, config, base_height_mm=73, shoulder_length_mm=144, elbow_length_mm=184, wrist_length_mm=57.2, gripper_length_mm=57.4, initial_coords=(0, 120, 210, 180, 90)):
		self.ssc32 = ssc32
		self.pan_servo = Servo.from_dict(config['pan_servo'])
		self.shoulder_servo = Servo.from_dict(config['shoulder_servo'])
		self.elbow_servo = Servo.from_dict(config['elbow_servo'])
		self.wrist_servo = Servo.from_dict(config['wrist_servo'])
		self.cuff_servo = Servo.from_dict(config['cuff_servo'])
		self.grip_servo = Servo.from_dict(config['grip_servo'])
		
		self.base_height_mm = base_height_mm
		self.shoulder_length_mm = shoulder_length_mm
		self.elbow_length_mm = elbow_length_mm
		self.wrist_length_mm = wrist_length_mm
		self.gripper_length_mm = gripper_length_mm
		#self.channels = [self.pan_servo.ch, self.shoulder_servo.ch, self.elbow_servo.ch, self.wrist_servo.ch, self.cuff_servo.ch, self.grip_servo.ch]
		self.channels = [self.pan_servo.ch, self.shoulder_servo.ch, self.elbow_servo.ch, self.wrist_servo.ch, self.cuff_servo.ch]
		
		self.inverse_kinematics = functools.partial(ik3, self.base_height_mm, self.shoulder_length_mm, self.elbow_length_mm)
		
		self.coords = list(initial_coords)
		self.set_coordinates_mm(self.coords, move=False)
		self.body_offset = None
	
	def __repr__(self):
		return self.__str__()
	
	def __str__(self):
		return 'Arm %s' % (self.channels,)
	
	def set_coordinates_mm(self, coords=None, move=True, time_ms=500):
		#if (len(coords) != 3):
		#	raise ValueError('tuple or list of length 3 is expected')
		if (coords is not None):
			x, y, z, w, c  = coords
			self.coords = list(coords)
		else:
			x, y, z, w, c = self.coords
		
		#log.debug("")
		log.debug("x : %d, y : %d, z : %d" % (x,y,z))
		
		#angles = leg_ik2(x,y,z, self.shoulder_length_mm, self.elbow_length_mm)
		angles = self.inverse_kinematics(x,y,z,w,c)
		if (angles is None):
			return False
		(pan_angle, shoulder_angle, elbow_angle, wrist_angle, cuff_angle) = angles
		#log.debug(angles)

		#perform forward kinematics
		#np.matmul(A, B)
		#numpy.linalg.multi_dot( (zRot(pan_angle), xRot(shoulder_angle), xRot(elbow_angle), 
		#zRot(pan_angle)

		#changes made to accomodate servo configurations
		#elbow_angle = -elbow_angle
		#pan_angle -= pi/2
		#log.debug(angles)

		pan_pw = self.pan_servo.get_pw(pan_angle)
		elbow_pw = self.elbow_servo.get_pw(elbow_angle)
		shoulder_pw = self.shoulder_servo.get_pw(shoulder_angle)
		wrist_pw = self.wrist_servo.get_pw(wrist_angle)
		cuff_pw = self.cuff_servo.get_pw(cuff_angle)
		
		#log.debug('pan pw %d shoulder pw %d elbow pw %d' % (pan_pw, shoulder_pw, elbow_pw))
		if (move == True):
			self.ssc32.multi_move(self.channels, pulses=[pan_pw, shoulder_pw, elbow_pw, wrist_pw, cuff_pw], time_ms=time_ms, block=False)
		else:
			return [self.channels, [pan_pw, shoulder_pw, elbow_pw, wrist_pw, cuff_pw]]
		return True
	
	def inc_x(self, amt, move=True):
		self.coords[0] += amt
		return self.set_coordinates_mm(move=move)
	
	def inc_y(self, amt, move=True):
		self.coords[1] += amt
		return self.set_coordinates_mm(move=move)	
	
	def inc_z(self, amt, move=True):
		self.coords[2] += amt
		return self.set_coordinates_mm(move=move)
	
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
	def __init__(self, config, ssc32, arm):
		self.zmq_ct = None
		self.push = None
		self.config = config
		self.stopped = False
		self.initialized = False
		
		self.ssc32 = ssc32
		self.arm = arm
		
		self.active_keys = {}
		
		self.ADVERTISMENT_RATE = lambda : self.config['advertisment_rate']
		
		self.last_key_press = time.monotonic()
		self.check_task = Task(2, self.check)
		self.advertisment_task = Task(self.ADVERTISMENT_RATE(), self.advertise_self)
		self.handle_key_press_task = Task(0.05, self.handle_key_presses)
		
		self.controller = Controller(self.ssc32, self.arm)

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
		self.initialized = False
		return False
		
	def init(self):
		log.info('state.init()')
		self.arm.set_coordinates_mm()
	
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

