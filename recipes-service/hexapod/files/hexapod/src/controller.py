from ssc32 import Hexapod_SSC32, Servo
from math import hypot, atan2, degrees
import numpy as np
from typing import Any
from time import sleep
import time
import logging as log
from body import Model
from control_pattern_generator import Legs
from control_pattern_generator import GAIT_CONFIGS
from util import safe_lookup

ALL_MODES		= 'all_modes'
WALK_MODE 		= 'walk'
MOVE_LEGS_MODE 	= 'move_legs'
DANCE_MODE		= 'dance_mode'
PASSIVE_MODE    = 'passive_mode'

def rescale(v, max_val, min_val):
	return (max_val-min_val)*v + min_val

class Controller:
	def __init__(self, ssc32, pantilt):
		self.modes = [WALK_MODE, MOVE_LEGS_MODE, DANCE_MODE]
		self.mode = DANCE_MODE

		self.ssc32 = ssc32
		self.pantilt = pantilt
		self.model = Model()
		self.cpg = Legs(selected_gait_name='tripod')
		
		self.step_finish_time = None
		
		self.ui_inputs = {}
	
		self.walk_mode_inputs_idle = {
			'ws' : 0,
			'ad' : 0,
			'qe' : 0,
			'up_down' : 0,
			'left_right' : 0,
			'step_height' : 25,
			'step_depth' : 25,
			'gait' : 'tripod',
		}
		self.walk_mode_inputs = self.walk_mode_inputs_idle.copy()
		
		self.move_legs_inputs_idle = {
			'x' : 105,
			'y' : 0,
			'z' : -106,
			'selected_leg' : 1,
		}
		self.move_legs_inputs = self.move_legs_inputs_idle.copy()
		
		self.dance_mode_inputs_idle = {
			'ws' : 0,
			'ad' : 0,
			'qe' : 0,
			'rf' : 0,
		}
		self.dance_mode_inputs = self.dance_mode_inputs_idle.copy()
		
		self.yaw_mode_inputs_idle = {
			'ws' : 0,
			'ad' : 0,
		}
		self.yaw_mode_inputs = self.yaw_mode_inputs_idle.copy()
		
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
				'2'		: lambda : self.select_mode(MOVE_LEGS_MODE),
				'3'		: lambda : self.select_mode(DANCE_MODE),
			},
			WALK_MODE : {
				'w' : lambda : inc_input(self.walk_mode_inputs, 'ws', 10, limits=True, min_val=0, max_val=75),
				's' : lambda : inc_input(self.walk_mode_inputs, 'ws', -10, limits=True, min_val=0, max_val=75),
				'a' : lambda : inc_input(self.walk_mode_inputs, 'ad', -1, limits=True),
				'd' : lambda : inc_input(self.walk_mode_inputs, 'ad', 1, limits=True),
				'q'	: lambda : inc_input(self.walk_mode_inputs, 'qe', -5, limits=True, min_val=-90, max_val=90),
				'e'	: lambda : inc_input(self.walk_mode_inputs, 'qe', 5, limits=True, min_val=-90, max_val=90),
				
				'r' : lambda : inc_input(self.walk_mode_inputs, 'step_height', 2, limits=True, min_val=10, max_val=40, report=True),
				'f' : lambda : inc_input(self.walk_mode_inputs, 'step_height', -2, limits=True, min_val=10, max_val=40, report=True),
				't' : lambda : inc_input(self.walk_mode_inputs, 'step_depth', 2, limits=True, min_val=0, max_val=60, report=True),
				'g' : lambda : inc_input(self.walk_mode_inputs, 'step_depth', -2, limits=True, min_val=0, max_val=60, report=True),
				
				'left'	: lambda : inc_input(self.walk_mode_inputs, 'left_right', -0.5, limits=True),
				'right'	: lambda : inc_input(self.walk_mode_inputs, 'left_right', 0.5, limits=True),
				'up'   	: lambda : inc_input(self.walk_mode_inputs, 'up_down', 0.2, limits=True, report=True),
				'down'	: lambda : inc_input(self.walk_mode_inputs, 'up_down', -0.2, limits=True, report=True),
				'space' : lambda : cycle_input(self.walk_mode_inputs, 'gait', choices=self.gaits),	
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
			DANCE_MODE : {
				'w' : lambda : inc_input(self.dance_mode_inputs, 'ws', 0.5, limits=True, min_val=-10, max_val=10),
				's' : lambda : inc_input(self.dance_mode_inputs, 'ws', -0.5, limits=True, min_val=-10, max_val=10),
				'a' : lambda : inc_input(self.dance_mode_inputs, 'ad', -0.5, limits=True, min_val=-10, max_val=10),
				'd' : lambda : inc_input(self.dance_mode_inputs, 'ad', 0.5, limits=True, min_val=-10, max_val=10),	
				'q' : lambda : inc_input(self.dance_mode_inputs, 'qe', -0.5, limits=True, min_val=-10, max_val=10),
				'e' : lambda : inc_input(self.dance_mode_inputs, 'qe', 0.5, limits=True, min_val=-10, max_val=10),
				'r' : lambda : inc_input(self.dance_mode_inputs, 'rf', 0.5, limits=True, min_val=-10, max_val=10),
				'f' : lambda : inc_input(self.dance_mode_inputs, 'rf', -0.5, limits=True, min_val=-10, max_val=10),				
			},
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
			DANCE_MODE : {
				'w' : self.key_pressed_actions[DANCE_MODE]['w'],
				's' : self.key_pressed_actions[DANCE_MODE]['s'],
				'a' : self.key_pressed_actions[DANCE_MODE]['a'],
				'd' : self.key_pressed_actions[DANCE_MODE]['d'],	
				'q' : self.key_pressed_actions[DANCE_MODE]['q'],
				'e' : self.key_pressed_actions[DANCE_MODE]['e'],
				'r' : self.key_pressed_actions[DANCE_MODE]['r'],
				'f' : self.key_pressed_actions[DANCE_MODE]['f'],					
			},
			PASSIVE_MODE : {},
		}
		
		self.key_released_actions = {
			ALL_MODES : {
				'j'	: lambda : self.pantilt.cancel_pan(),#self.pantilt.panh(0),
				'l'	: lambda : self.pantilt.cancel_pan(),#self.pantilt.panh(0),
				'k'	: lambda : self.pantilt.cancel_tilt(),#self.pantilt.panv(0),
				'i'	: lambda : self.pantilt.cancel_tilt(),#self.pantilt.panv(0),
			},
			WALK_MODE : {},
			MOVE_LEGS_MODE : {},
			DANCE_MODE : {},
			PASSIVE_MODE : {},
		}

	def update_ui_inputs(self, id, value):
		#log.info('%s=%f' % (id, value))
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
			prev_step_height = self.walk_mode_inputs['step_height']
			prev_step_depth  = self.walk_mode_inputs['step_depth']
			up_down = self.walk_mode_inputs['up_down']
			self.walk_mode_inputs = self.walk_mode_inputs_idle.copy()
			self.walk_mode_inputs['step_height'] = prev_step_height
			self.walk_mode_inputs['step_depth']  = prev_step_depth
			self.walk_mode_inputs['gait'] = prev_gait
			self.walk_mode_inputs['up_down'] = up_down
			self.run_walk(**self.walk_mode_inputs, idle=True)
		elif (self.mode == DANCE_MODE):
			self.dance_mode_inputs = self.dance_mode_inputs_idle.copy()
			self.run_dance_mode(**self.dance_mode_inputs, time_ms=500)
	
	def handle_idle_long(self):
		log.info('controller.handle_idle_long()')
		self.select_mode(PASSIVE_MODE)
		self.sit()

	def stand(self): # setup the starting robot positions
		log.info('stand()')
		#self.body_model = bodyPos(pitch=0, roll=0, yaw=0, Tx=0, Ty=0, Tz=0)
		#self.start_leg = startLegPos(self.body_model, start_x_offset=100, start_height=30, corner_leg_rotation_offset=0)
		#self.ssc32.move_legs(self.start_leg, time_ms=800, block=True)
		
		#self.start_leg = self.neutral_leg_position()
		#self.ssc32.move_legs(self.start_leg, time_ms=500, block=True)
		
		#self.leg_model = legModel(self.start_leg, self.body_model)
		#self.turn_feet_positions = getFeetPos(self.leg_model)
		
	def stand_short(self):
		self.start_leg = self.neutral_leg_position()
		self.step_finish_time = self.ssc32.move_legs(self.start_leg, time_ms=250)		

	def sit(self):
		log.info('sit()')
		#self.body_model = bodyPos(pitch=0, roll=0, yaw=0, Tx=0, Ty=0, Tz=0)
		#self.start_leg = startLegPos(self.body_model, start_x_offset=80, start_height=10, corner_leg_rotation_offset=0)
		#self.ssc32.move_legs(self.start_leg , time_ms=1000, block=True) # get the serial message from the angles

	def cycle_modes(self):
		self.select_mode(self.modes.pop(0))
		self.modes.append(self.mode)
		return self.mode
	
	def select_mode(self, new_mode : str):
		self.mode = new_mode
		log.info('"%s" mode selected' % (self.mode,))
		
		if (self.mode == MOVE_LEGS_MODE):
			self.start_leg = leg_position_relative(body_model=self.body_model, **self.move_legs_inputs)
		elif (self.mode == DANCE_MODE):
			self.run_dance_mode(**self.dance_mode_inputs, time_ms=500)

	def run(self, *args, **kwargs):
		def run_move_positions():
			if (self.step_finish_time is None):
				return True
			elif (time.monotonic() > self.step_finish_time):
				self.step_finish_time = None
				return True
			else:
				return False

		if (run_move_positions() != True):
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
		angles = self.model.set_leg_positions(positions, corner_leg_rotation_offset=0)
		self.step_finish_time = self.ssc32.move_legs(angles, time_ms=500)
		
	def run_walk(self, ws, ad, qe, step_height, step_depth,  gait, up_down, left_right,idle=False, time_ms=100, speed=500):
		log.info('run_walk(ws=%d,qe=%d)' % (ws,qe))
		self.cpg.set_A(ws)
		#self.cpg.set_yaw(qe)
		self.cpg.set_yaw(180)
		self.cpg.new_gait(gait)
		
		leg_coords_inc = self.cpg.update()
		#log.info(leg_coords_inc[0:2,0:3])
		angles = self.model.set_leg_positions(self.model.leg_coords_local + leg_coords_inc)
		#log.info(angles)
		self.step_finish_time = self.ssc32.move_legs(angles, time_ms=time_ms, speed=speed) # get the serial message from the angles


	def run_dance_mode(self, ws, ad, qe, rf, time_ms=100):# right_left_d, down_up_d):
		
		yaw 	= rescale(safe_lookup(self.ui_inputs, 'yaw', default=0.5), 10, -10)
		pitch 	= rescale(safe_lookup(self.ui_inputs, 'pitch', default=0.5), 10, -10)
		roll 	= rescale(safe_lookup(self.ui_inputs, 'roll', default=0.5), 10, -10)
		tx 		= rescale(safe_lookup(self.ui_inputs, 'tx', default=0.5), 20, -20)
		ty 		= rescale(safe_lookup(self.ui_inputs, 'ty', default=0.5), 20, -20)
		tz 		= rescale(safe_lookup(self.ui_inputs, 'tz', default=0.5), 20, -20)

		self.model.set_body(yaw=yaw, pitch=pitch, roll=roll, tx=tx, ty=ty, tz=tz)

		leg_coords = np.array([ [70, 0, -80, 1],
								[70, 0, -80, 1],
								[70, 0, -80, 1],
								[70, 0, -80, 1],
								[70, 0, -80, 1],
								[70, 0, -80, 1]
								])
		angles = self.model.set_leg_positions(leg_coords)
		
		self.step_finish_time = self.ssc32.move_legs(angles, time_ms=time_ms) # get the serial message from the angles
