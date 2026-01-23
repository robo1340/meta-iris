from ssc32 import Hexapod_SSC32, Servo
from math import hypot, atan2, degrees, pi
import numpy as np
from typing import Any
from time import sleep
import time
import logging as log
from body import Model
from control_pattern_generator import Legs
from control_pattern_generator import GAIT_CONFIGS
from util import safe_lookup

from body import COXA_LENGTH
from body import FEMUR_LENGTH
from body import TIBIA_LENGTH

ALL_MODES		= 'all_modes'
WALK_MODE 		= 'walk'
MOVE_LEGS_MODE 	= 'move_legs'
DANCE_MODE		= 'dance_mode'
PASSIVE_MODE    = 'passive_mode'

def rescale(v, max_val, min_val):
	return (max_val-min_val)*v + min_val

class Controller:
	def __init__(self, ssc32, pantilt, servo_dicts):
		self.modes = [WALK_MODE, DANCE_MODE, MOVE_LEGS_MODE, PASSIVE_MODE]
		self.mode = WALK_MODE

		self.ssc32 = ssc32
		self.pantilt = pantilt
		self.model = Model(servo_dicts)
		self.cpg = Legs(selected_gait_name='tripod')
		
		self.step_finish_time = None
		
		self.ui_inputs = {}
		self.corner_rot = lambda : rescale(safe_lookup(self.ui_inputs, 'corner_rotation', default=0.5), 40, 0)
	
		self.walk_mode_inputs_idle = {
			'w' : 0,
			's' : 0,
			'a' : 0,
			'd' : 0,
			'q' : 0,
			'e' : 0,
			'gait' : 'tripod',
		}
		self.walk_mode_inputs = self.walk_mode_inputs_idle.copy()
		
		self.move_legs_inputs_idle = {
			'x' : COXA_LENGTH+FEMUR_LENGTH,
			'y' : 0,
			'z' : -TIBIA_LENGTH,
			'selected_leg' : 1,
		}
		self.move_legs_inputs = self.move_legs_inputs_idle.copy()
		
		self.dance_mode_inputs_idle = {}
		self.dance_mode_inputs = self.dance_mode_inputs_idle.copy()
		
		def set_input(d,key,val):
			d[key] = val
		
		def inc_input(d,key,inc,limits=False,min_val=-1,max_val=1, report=False):
			if (limits == False):
				d[key] += inc
			elif (limits == True):
				d[key] = min(max(d[key]+inc, min_val), max_val)
			if (report==True):
				log.info('%s=%s' % (key, d[key]))
		
		def toggle_input(d,key, report=False):
			d[key] = not d[key]
			if (report == True):
				log.info('%s=%s' % (key, d[key]))
		
		self.gaits = list(GAIT_CONFIGS.keys())
		def cycle_input(d, key, choices, report=True):
			
			d[key] = choices.pop(0)
			choices.append(d[key])
			if (report == True):
				log.info('%s=%s' % (key, d[key]))
		
		self.key_pressed_actions = {
			ALL_MODES : {
				'enter' : self.cycle_modes,
				'1'		: lambda : self.select_mode(WALK_MODE),
				'2'		: lambda : self.select_mode(DANCE_MODE),
				'3'		: lambda : self.select_mode(MOVE_LEGS_MODE),
				'4'		: lambda : self.select_mode(PASSIVE_MODE),
			},
			WALK_MODE : {
				'w' : lambda : set_input(self.walk_mode_inputs, 'w', 1),
				's' : lambda : set_input(self.walk_mode_inputs, 's', 1),
				'a' : lambda : set_input(self.walk_mode_inputs, 'a', 1),
				'd' : lambda : set_input(self.walk_mode_inputs, 'd', 1),
				'q'	: lambda : set_input(self.walk_mode_inputs, 'q', 1),
				'e'	: lambda : set_input(self.walk_mode_inputs, 'e', 1),
				'space' : lambda : cycle_input(self.walk_mode_inputs, 'gait', choices=self.gaits, report=False),	
			},
			MOVE_LEGS_MODE : {
				'w' : lambda : inc_input(self.move_legs_inputs, 'y', 5, report=True),
				's' : lambda : inc_input(self.move_legs_inputs, 'y', -5, report=True),
				'a' : lambda : inc_input(self.move_legs_inputs, 'x', -5, report=True),
				'd' : lambda : inc_input(self.move_legs_inputs, 'x', 5, report=True),
				
				'up'  : lambda : inc_input(self.move_legs_inputs, 'selected_leg', 1, limits=True, min_val=1, max_val=6, report=True),
				'down' : lambda : inc_input(self.move_legs_inputs, 'selected_leg',-1, limits=True, min_val=1, max_val=6, report=True),
				
				'r' : lambda : inc_input(self.move_legs_inputs, 'z', 5, report=True),
				'f' : lambda : inc_input(self.move_legs_inputs, 'z', -5, report=True),
			},
			DANCE_MODE : {},
			PASSIVE_MODE : {},
		}
		
		pan_amt = 40
		self.key_held_actions = {
			ALL_MODES : {
				'j'	: lambda : self.pantilt.panh(-pan_amt),
				'l'	: lambda : self.pantilt.panh(pan_amt),
				'k'	: lambda : self.pantilt.panv(-pan_amt),
				'i'	: lambda : self.pantilt.panv(pan_amt),
			},
			WALK_MODE : {
				'w' : self.key_pressed_actions[WALK_MODE]['w'],
				's' : self.key_pressed_actions[WALK_MODE]['s'],
				'a' : self.key_pressed_actions[WALK_MODE]['a'],
				'd' : self.key_pressed_actions[WALK_MODE]['d'],
				'q' : self.key_pressed_actions[WALK_MODE]['q'],
				'e' : self.key_pressed_actions[WALK_MODE]['e'],
			},
			MOVE_LEGS_MODE : {},
			DANCE_MODE : {},
			PASSIVE_MODE : {},
		}
		
		self.key_released_actions = {
			ALL_MODES : {
				'j'	: lambda : self.pantilt.cancel_pan(),#self.pantilt.panh(0),
				'l'	: lambda : self.pantilt.cancel_pan(),#self.pantilt.panh(0),
				'k'	: lambda : self.pantilt.cancel_tilt(),#self.pantilt.panv(0),
				'i'	: lambda : self.pantilt.cancel_tilt(),#self.pantilt.panv(0),
			},
			WALK_MODE : {
				'w' : lambda : set_input(self.walk_mode_inputs, 'w', 0),
				's' : lambda : set_input(self.walk_mode_inputs, 's', 0),
				'a' : lambda : set_input(self.walk_mode_inputs, 'a', 0),
				'd' : lambda : set_input(self.walk_mode_inputs, 'd', 0),
			},
			MOVE_LEGS_MODE : {},
			DANCE_MODE : {},
			PASSIVE_MODE : {},
		}

	def update_ui_inputs(self, id, value):
		self.ui_inputs[id] = value

	def handle_key_pressed(self, key : str):
		if (key in self.key_pressed_actions[ALL_MODES]):
			self.key_pressed_actions[ALL_MODES][key]()
		elif (key in self.key_pressed_actions[self.mode]):
			self.key_pressed_actions[self.mode][key]()

	def handle_key_held(self, key : str):
		if (key in self.key_held_actions[ALL_MODES]):
			self.key_held_actions[ALL_MODES][key]()
		elif (key in self.key_held_actions[self.mode]):
			self.key_held_actions[self.mode][key]()

	def handle_key_released(self, key : str):
		if (key in self.key_released_actions[ALL_MODES]):
			self.key_released_actions[ALL_MODES][key]()
		elif (key in self.key_released_actions[self.mode]):
			self.key_released_actions[self.mode][key]()		

	def handle_idle(self):
		#log.info('controller.handle_idle()')
		if (self.mode == WALK_MODE):
			prev_gait = self.walk_mode_inputs['gait']
			self.walk_mode_inputs = self.walk_mode_inputs_idle.copy()
			self.walk_mode_inputs['gait'] = prev_gait
	
	def handle_idle_long(self):
		log.info('controller.handle_idle_long()')
		self.select_mode(PASSIVE_MODE)

	def stand(self): # setup the starting robot positions
		log.info('stand()')
		angles = self.model.init_leg_positions(corner_leg_rotation_offset=self.corner_rot())	
		self.step_finish_time = self.ssc32.move_legs(angles, time_ms=500, block=True)		

	def sit(self):
		log.info('sit()')
		#self.model.set_body(yaw=0, pitch=0, roll=0, tx=0, ty=0, tz=0)
		angles = self.model.init_leg_positions(start_x_offset=90, start_height=25, corner_leg_rotation_offset=0)
		self.step_finish_time = self.ssc32.move_legs(angles, time_ms=500, block=True)

	def cycle_modes(self):
		self.select_mode(self.modes.pop(0))
		self.modes.append(self.mode)
		return self.mode
	
	def select_mode(self, new_mode : str):
		self.mode = new_mode
		log.info('"%s" mode selected' % (self.mode,))
		
		if (self.mode == WALK_MODE):
			self.model.init_leg_positions(corner_leg_rotation_offset=self.corner_rot())
		elif (self.mode == MOVE_LEGS_MODE):
			self.run_move_legs(**self.move_legs_inputs)
		elif (self.mode == DANCE_MODE):
			self.run_dance_mode(**self.dance_mode_inputs, time_ms=500)
		elif (self.mode == PASSIVE_MODE):
			self.sit()

	def run(self, *args, **kwargs):
		def run_move_positions():
			if (self.step_finish_time is None):
				return True
			elif (time.monotonic() > self.step_finish_time):
				self.step_finish_time = None
				return True
			else:
				return False

		if (run_move_positions() != True): #always run the control pattern generator
			self.cpg.update()
			return
		
		if (self.mode == WALK_MODE):
			self.run_walk(**self.walk_mode_inputs)
		elif (self.mode == MOVE_LEGS_MODE):
			self.run_move_legs(**self.move_legs_inputs)
		elif (self.mode == DANCE_MODE):
			self.run_dance_mode(**self.dance_mode_inputs)

	def run_move_legs(self, x, y, z, selected_leg, time_ms=500):
		# L1|----|L2
		#   |    |
		# L3|    |L4
		#   |    |
		# L5|----|L6
		positions = np.array([[x,-y,z,1],
							  [x,y,z,1],
							  [x,-y,z,1],
							  [x,y,z,1],
							  [x,-y,z,1],
							  [x,y,z,1]])
		#log.info(positions[5])
		angles = self.model.set_leg_positions(positions, corner_leg_rotation_offset=0)
		self.step_finish_time = self.ssc32.move_legs(angles, time_ms=500)
	
	def set_body_orientation(self):
		yaw 	= rescale(safe_lookup(self.ui_inputs, 'yaw', default=0.5), 12, -12)
		pitch 	= rescale(safe_lookup(self.ui_inputs, 'pitch', default=0.5), 12, -12)
		roll 	= rescale(safe_lookup(self.ui_inputs, 'roll', default=0.5), 12, -12)
		tx 		= rescale(safe_lookup(self.ui_inputs, 'tx', default=0.5), 30, -30)
		ty 		= rescale(safe_lookup(self.ui_inputs, 'ty', default=0.5), 30, -30)
		tz 		= rescale(safe_lookup(self.ui_inputs, 'tz', default=0.5), 30, -30)
		self.model.set_body(yaw=yaw, pitch=pitch, roll=roll, tx=tx, ty=ty, tz=tz)
	
	def run_walk(self, w, s, a, d, q, e, gait, time_ms=100, speed=None):
		#log.info('run_walk(ws=%d,qe=%d)' % (ws,qe))
		self.set_body_orientation()
		
		wasd_vector = np.array([d-a,w-s])
		rotate_walk = e-q
		mag = np.linalg.norm(wasd_vector)
		angle = degrees(atan2(wasd_vector[1], wasd_vector[0])) - 90
		angle = (angle + 360) % 360
		#log.info('%f %f' % (mag, angle))
		
		length = rescale(safe_lookup(self.ui_inputs, 'stride_length', default=0.5), 70, 0)
		height_ratio = safe_lookup(self.ui_inputs, 'step_height', default=0.5)
		walk_speed = rescale(safe_lookup(self.ui_inputs, 'speed', default=0.5), 1.2, 0.2)
		
		if (mag==0) and (rotate_walk==0):
			length = 0
			height_ratio = 0
		elif (mag==0) and (rotate_walk!=0):
			mag = 1
			angle = 0
		
		self.cpg.set_step_parameters(length, height_ratio, walk_speed)
		
		self.cpg.set_yaw(angle)
		self.cpg.new_gait(gait)
		
		if(rotate_walk!=0):
			self.cpg.set_turn('right' if (rotate_walk>0) else 'left')
		else:
			self.cpg.set_turn(None)
		
		leg_coords_inc = self.cpg.update()
		
		#log.info('leg_coords_increment')
		#log.info(leg_coords_inc[1])
		
		angles = self.model.set_leg_positions(self.model.leg_coords_local, gait_offset=leg_coords_inc, corner_leg_rotation_offset=self.corner_rot())
		#log.info(angles)
		self.step_finish_time = self.ssc32.move_legs(angles, time_ms=time_ms, speed=speed) # get the serial message from the angles


	def run_dance_mode(self, time_ms=100):
		self.set_body_orientation()
		angles = self.model.init_leg_positions(start_x_offset=70, start_height=80, corner_leg_rotation_offset=self.corner_rot())
		self.step_finish_time = self.ssc32.move_legs(angles, time_ms=time_ms) # get the serial message from the angles
