
from leg import recalculateLegAngles, startLegPos, legModel, getFeetPos, leg_position_relative
from body import bodyPos, bodyAngle
from move import walk
from ssc32 import Hexapod_SSC32, Servo
from math import hypot, atan2, degrees
import numpy as np
from typing import Any
from time import sleep
import time
import logging as log

ALL_MODES		= 'all_modes'
WALK_MODE 		= 'walk'
MOVE_LEGS_MODE 	= 'move_legs'
DANCE_MODE		= 'dance_mode'
YAW_MODE		= 'yaw_mode'
PASSIVE_MODE    = 'passive_mode'

class Controller:
	def __init__(self, ssc32, pantilt):
		self.modes = [WALK_MODE, MOVE_LEGS_MODE, DANCE_MODE, YAW_MODE]
		self.mode = self.cycle_modes()

		self.ssc32 = ssc32
		self.pantilt = pantilt
		self.body_model = bodyPos(pitch=0, roll=0, yaw=0, Tx=0, Ty=0, Tz=0, )
		self.neutral_leg_position = lambda rot=20 : startLegPos(self.body_model, start_x_offset=100, start_height=80, corner_leg_rotation_offset=rot)
		self.start_leg = self.neutral_leg_position()
		
		self.leg_model = legModel(self.start_leg, self.body_model)
		self.previous_walk_step = 0
		self.previous_walk_angle = 90
		self.previous_turn_angle = 0
		self.turn_feet_positions = getFeetPos(self.leg_model)
		self.last_position = self.turn_feet_positions
		self.right_foot = True

		self.max_walk_distance = 18
		self.max_turn_angle = 15
		self.max_body_shift = 20
		self.max_body_shift_z = 60
		self.max_body_turn = 20
		self.start_down = True
		
		self.move_positions = []
		self.step_finish_time = None
	
		self.walk_mode_inputs_idle = {
			'ws' : 0,
			'ad' : 0,
			'qe' : 0,
			'up_down' : 0,
			'left_right' : 0,
			'step_height' : 25,
			'step_depth' : 25,
			'shift_while_walking' : False,
		}
		self.walk_mode_inputs = self.walk_mode_inputs_idle.copy()
		
		self.move_legs_inputs_idle = {
			'x' : 80,
			'y' : 0,
			'z' : -30,
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
		
		self.key_pressed_actions = {
			ALL_MODES : {
				'enter' : self.cycle_modes,
				'1'		: lambda : self.select_mode(WALK_MODE),
				'2'		: lambda : self.select_mode(MOVE_LEGS_MODE),
				'3'		: lambda : self.select_mode(DANCE_MODE),
				'4'		: lambda : self.select_mode(YAW_MODE),
			},
			WALK_MODE : {
				'w' : lambda : inc_input(self.walk_mode_inputs, 'ws', 1, limits=True),
				's' : lambda : inc_input(self.walk_mode_inputs, 'ws', -1, limits=True),
				'a' : lambda : inc_input(self.walk_mode_inputs, 'ad', -1, limits=True),
				'd' : lambda : inc_input(self.walk_mode_inputs, 'ad', 1, limits=True),
				'q'	: lambda : inc_input(self.walk_mode_inputs, 'qe', -1, limits=True),
				'e'	: lambda : inc_input(self.walk_mode_inputs, 'qe', 1, limits=True),
				
				'r' : lambda : inc_input(self.walk_mode_inputs, 'step_height', 2, limits=True, min_val=10, max_val=40, report=True),
				'f' : lambda : inc_input(self.walk_mode_inputs, 'step_height', -2, limits=True, min_val=10, max_val=40, report=True),
				't' : lambda : inc_input(self.walk_mode_inputs, 'step_depth', 2, limits=True, min_val=0, max_val=60, report=True),
				'g' : lambda : inc_input(self.walk_mode_inputs, 'step_depth', -2, limits=True, min_val=0, max_val=60, report=True),
				
				'left'	: lambda : inc_input(self.walk_mode_inputs, 'left_right', -0.5, limits=True),
				'right'	: lambda : inc_input(self.walk_mode_inputs, 'left_right', 0.5, limits=True),
				'up'   	: lambda : inc_input(self.walk_mode_inputs, 'up_down', 0.2, limits=True, report=True),
				'down'	: lambda : inc_input(self.walk_mode_inputs, 'up_down', -0.2, limits=True, report=True),
				'space' : lambda : toggle_input(self.walk_mode_inputs, 'shift_while_walking', report=True),	
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
				'w' : lambda : inc_input(self.dance_mode_inputs, 'ws', 0.02, limits=True),
				's' : lambda : inc_input(self.dance_mode_inputs, 'ws', -0.02, limits=True),
				'a' : lambda : inc_input(self.dance_mode_inputs, 'ad', -0.2, limits=True),
				'd' : lambda : inc_input(self.dance_mode_inputs, 'ad', 0.2, limits=True),	
				'q' : lambda : inc_input(self.dance_mode_inputs, 'qe', -0.1, limits=True),
				'e' : lambda : inc_input(self.dance_mode_inputs, 'qe', 0.1, limits=True),
				'r' : lambda : inc_input(self.dance_mode_inputs, 'rf', 0.1, limits=True),
				'f' : lambda : inc_input(self.dance_mode_inputs, 'rf', -0.1, limits=True),				
			},
			YAW_MODE : {
				'w' : lambda : inc_input(self.yaw_mode_inputs, 'ws', 0.1, limits=True),
				's' : lambda : inc_input(self.yaw_mode_inputs, 'ws', -0.1, limits=True),
				'a' : lambda : inc_input(self.yaw_mode_inputs, 'ad', -0.05, limits=True),
				'd' : lambda : inc_input(self.yaw_mode_inputs, 'ad', 0.05, limits=True),				
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
			YAW_MODE : {
				'w' : self.key_pressed_actions[YAW_MODE]['w'],
				's' : self.key_pressed_actions[YAW_MODE]['s'],
				'a' : self.key_pressed_actions[YAW_MODE]['a'],
				'd' : self.key_pressed_actions[YAW_MODE]['d'],		
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
			YAW_MODE : {},
			PASSIVE_MODE : {},
		}

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
			prev_shift_while_walking = self.walk_mode_inputs['shift_while_walking']
			prev_step_height = self.walk_mode_inputs['step_height']
			prev_step_depth  = self.walk_mode_inputs['step_depth']
			up_down = self.walk_mode_inputs['up_down']
			self.walk_mode_inputs = self.walk_mode_inputs_idle.copy()
			self.walk_mode_inputs['step_height'] = prev_step_height
			self.walk_mode_inputs['step_depth']  = prev_step_depth
			self.walk_mode_inputs['shift_while_walking'] = prev_shift_while_walking
			self.walk_mode_inputs['up_down'] = up_down
			self.run_walk(**self.walk_mode_inputs, idle=True)
		elif (self.mode == DANCE_MODE):
			self.dance_mode_inputs = self.dance_mode_inputs_idle.copy()
			self.run_dance_mode(**self.dance_mode_inputs, time_ms=500)
		elif (self.mode == YAW_MODE):
			self.yaw_mode_inputs = self.yaw_mode_inputs_idle.copy()
			self.run_yaw_mode(**self.yaw_mode_inputs, time_ms=500)
	
	def handle_idle_long(self):
		log.info('controller.handle_idle_long()')
		self.select_mode(PASSIVE_MODE)
		self.sit()

	def stand(self): # setup the starting robot positions
		log.info('stand()')
		self.body_model = bodyPos(pitch=0, roll=0, yaw=0, Tx=0, Ty=0, Tz=0)
		self.start_leg = startLegPos(self.body_model, start_x_offset=100, start_height=30, corner_leg_rotation_offset=0)
		self.ssc32.move_legs(self.start_leg, time_ms=800, block=True)
		
		self.start_leg = self.neutral_leg_position()
		self.ssc32.move_legs(self.start_leg, time_ms=500, block=True)
		
		self.leg_model = legModel(self.start_leg, self.body_model)
		self.turn_feet_positions = getFeetPos(self.leg_model)
		
	def stand_short(self):
		self.start_leg = self.neutral_leg_position()
		self.step_finish_time = self.ssc32.move_legs(self.start_leg, time_ms=250)		

	def sit(self):
		log.info('sit()')
		self.body_model = bodyPos(pitch=0, roll=0, yaw=0, Tx=0, Ty=0, Tz=0)
		self.start_leg = startLegPos(self.body_model, start_x_offset=80, start_height=10, corner_leg_rotation_offset=0)
		self.ssc32.move_legs(self.start_leg , time_ms=1000, block=True) # get the serial message from the angles

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
		elif (self.mode == YAW_MODE):
			self.run_yaw_mode(**self.yaw_mode_inputs, time_ms=500)

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
			return
		
		if (self.mode == WALK_MODE):
			self.run_walk(**self.walk_mode_inputs)
		elif (self.mode == MOVE_LEGS_MODE):
			self.run_move_legs(**self.move_legs_inputs)
		elif (self.mode == DANCE_MODE):
			self.run_dance_mode(**self.dance_mode_inputs)
		elif (self.mode == YAW_MODE):
			self.run_yaw_mode(**self.yaw_mode_inputs)

	def run_move_legs(self, x, y, z, selected_leg, time_ms=500):
		new_position = leg_position_relative(x, y ,z, self.body_model)
		#log.info(new_position)
		self.start_leg[selected_leg-1] = new_position[selected_leg-1]
		
		self.step_finish_time = self.ssc32.move_legs(self.start_leg, time_ms=500)
		
	def run_walk(self, ws, ad, qe, step_height, step_depth,  shift_while_walking, up_down, left_right,idle=False, time_ms=100, speed=500):
		if (idle==True):
			if (len(self.move_positions) > 0):
				end = self.move_positions[-1].copy()
			else:
				end = self.last_position.copy()
			x = end[:,2].min() 
			end[:,2] = np.full(end[:,2].size, x) 
			self.move_positions += [end]
		
		if (len(self.move_positions) > 0):
			self.last_position = self.move_positions.pop(0)
			angles = recalculateLegAngles(self.last_position, self.body_model) # convert the feet positions to angles
			self.step_finish_time = self.ssc32.move_legs(angles, time_ms=time_ms, speed=speed) # get the serial message from the angles
			#self.leg_model = legModel(position, self.body_model)
			return

		if (ws==0) and (ad==0) and (qe == 0): 
			return
			
		pitch = roll = Tx = Ty = 0
		#self.body_model = bodyPos(pitch=pitch, roll=roll, yaw=0, Tx=0, Ty=0, Tz=0)

		Tz = self.max_body_shift * up_down
		self.body_model =bodyPos(pitch=0, roll=0, yaw=0, Tx=0,Ty=0, Tz=Tz)

		walk_distance = hypot(ws, qe) * self.max_walk_distance
		walk_angle = degrees(atan2(ws, qe))
		turn_angle = ad * self.max_turn_angle

		log.info('walk_distance %d, walk_angle %d, turn_angle %d' % (walk_distance, walk_angle, turn_angle))
		
		[self.turn_feet_positions, self.right_foot, self.previous_walk_step, self.previous_walk_angle, self.previous_turn_angle, move_positions] = walk(
								self.turn_feet_positions, self.right_foot,
								self.previous_walk_step,
								self.previous_walk_angle,
								self.previous_turn_angle,
								walk_distance, 
								walk_angle, 
								turn_angle,
								step_height=step_height,
								step_depth=step_depth,
								z_resolution=10
								)
		self.move_positions += move_positions

	def run_dance_mode(self, ws, ad, qe, rf, time_ms=100):# right_left_d, down_up_d):
		#log.info('run_dance_mode(%2.1f %2.1f %2.1f %2.1f %d)' % (ls_x, ls_y, rs_x, rs_y, time.monotonic()*1000.0))
		[pitch, roll] = bodyAngle(analog_x=qe, analog_y=ws, max_angle=self.max_body_turn)
		Tx = self.max_body_shift * ad
		Ty = self.max_body_shift * rf * 2
		self.body_model = bodyPos(pitch=pitch, roll=roll, yaw=0, Tx=Tx, Ty=Ty, Tz=0)
		self.start_leg = self.neutral_leg_position(0)
		self.step_finish_time = self.ssc32.move_legs(self.start_leg, time_ms=time_ms) # get the serial message from the angles

	def run_yaw_mode(self, ws, ad, time_ms=100):
		#log.info('run_yaw_mode(%2.2f %2.2f, %d)' % (ws, ad, time_ms))
		yaw = ad * self.max_body_turn
		Tz = ws * self.max_body_shift_z
		self.body_model = bodyPos(pitch=0, roll=0, yaw=yaw, Tx=0, Ty=0, Tz=Tz)
		self.start_leg = self.neutral_leg_position(0)
		self.step_finish_time = self.ssc32.move_legs(self.start_leg, time_ms=time_ms) # get the serial message from the angles
