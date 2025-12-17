"""
Driver functions to communicate with the Lynxmotion SSC-32U.

Functions to communicate with the Lynxmotion SSC-32U to drive the 18 servos of
the hexapod. These functions format commands based on the user manual for the
controller board.

Functions
---------
angleToPW:
	Convert an angle to the pulse width to command that angle.
anglesToSerial:
	Takes the hexapods servo angles and converts them to a serial command.
connect:
	Connect to the input COM port.
disconnect:
	Disconnects from the serial port.
sendData:
	Sends the commands for the servos to the Lynxmotion SSC-32U.

Notes
-----
As of right now, this product may have been discontinued.
"""
import serial
from serial import SerialException
import numpy as np
from typing import Any, Optional
import logging as log
import time

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
		self.current_angle = angles[1]

		def limits(x, lower, upper):
			return max(lower, min(x, upper))
	
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
	
	def limits(self):
		return (self.lower_angle, self.upper_angle)
	
	def get_pw(self, desired_angle):
		desired_angle = self.my_limits(desired_angle)
		return int(self.m * desired_angle + self.b)
	
	@staticmethod
	def from_dict(d):
		return Servo(ch=d['ch'], angles=d['angles'], pws=d['pws'], calibration=d['calibration'])

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
	
	def move(self, ch : int, pulse : int, time_ms : int = None, block=False, block_timeout=3):
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

	def block(self, block_timeout=1):
		start = time.monotonic()
		while ((time.monotonic() - start) < block_timeout):
			self.write('Q')
			if (self.read() == b'.'):
				break
	
	def prev_move_complete(self):
		self.write('Q')
		if (self.read() == b'.'):
			return True
		return False

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

class Hexapod_SSC32(SSC32):
	def __init__(self, path, baud, timeout, leg_servos):
		super().__init__(path, baud, timeout)
		self.leg_servos = leg_servos

	def move_legs(self, angles: np.ndarray, speed: Optional[int] = None, time_ms: Optional[int] = None, block=False):
		"""
		Takes the hexapods servo angles and converts them to a serial command.

		Converts an array of 18 servo angles to the formatted serial command for
		the Lynxmotion SSC-32U. The servo angles need to be in the format made in
		the leg module.

		Parameters
		----------
		angles: np.ndarray
			6x3 numpy array of the 18 servo angles of the hexapod's legs.
		speed: int, optional
			How fast the servos move in microseconds/second (the default is None).
			A speed of 1000 takes 1 second to go 90 degrees.
		time_ms: int, optional
			The maximum amount of time in milliseconds to move (the default is
			None).

		Returns
		-------
		serial_string: bytes
			The formatted message for all 18 servos together.

		Raises
		------
		Exception
			If the input numpy array is not a 6x3 array.

		See Also
		--------
		angleToPW:
			Convert an angle to the pulse width to command that angle.

		Notes
		-----
		The message is converted to bytes in the return statement.
		"""
		if angles.shape == (6, 3):
			#log.info(angles)
			temp_angles = np.copy(angles)
			
			temp_angles[0, 0] = (temp_angles[0, 0] + 360) % 360.0
			temp_angles[2, 0] = (temp_angles[2, 0] + 360) % 360.0
			temp_angles[4, 0] = (temp_angles[4, 0] + 360) % 360.0
			#log.info(temp_angles)
			
			#log.info('----')
			#log.info(temp_angles)
			#log.info(temp_angles[:,0])
			
			#log.info(temp_angles[3:6,0])
			#temp_angles[0:3, 2] = - temp_angles[0:3, 2]
			#log.info(temp_angles)
			#temp_angles[3:6, 0] = (temp_angles[3:6, 0] + 360) % 360.0
			#temp_angles[3:6, 1] = - temp_angles[3:6, 1]
			#log.info(temp_angles)
			adjustment = np.array([[-90-180, 0, -90],
								   [-90,     0, -90],
								   [-90-180, 0, -90],
								   [-90,     0, -90],
								   [-90-180, 0, -90],
								   [-90,     0, -90]])
			temp_angles = temp_angles + adjustment
			#temp_angles[:,0] = temp_angles[:,0] % 360
			#log.info(temp_angles)
			angles_processed = temp_angles.flatten()
		else:
			raise Exception('Input angles were the wrong format. Should be a 6x3 numpy array')
		# speed is in microseconds/second and time is in milliseconds. A speed of
		# 1000us takes 1 second to go 90 degrees
		serial_string = ''
		for i, angle in enumerate(angles_processed):
			servo = self.leg_servos[i]
			serial_string += '#' + str(servo.ch) + 'P' + str(servo.get_pw(angle))
			#log.info(str(servo.get_pw(angle)))
			if speed is not None:
				serial_string += 'S' + str(speed)

		if time_ms is not None:
			serial_string += 'T' + str(time_ms)
		#log.debug(serial_string)
		self.write(serial_string)
		if (block == True):
			self.block()
		if (time_ms is not None):
			return time.monotonic() + time_ms/1000.0