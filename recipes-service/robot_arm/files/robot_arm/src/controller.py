import numpy as np
import time
import logging as log

ALL_MODES		= 'all_modes'
NORMAL_MODE     = 'normal_mode'

class Controller:
	def __init__(self, ssc32, arm):
		self.modes = [NORMAL_MODE]
		self.mode = self.cycle_modes()

		self.ssc32 = ssc32
		self.arm = arm
		
		self.step_finish_time = None
	
		self.normal_mode_inputs_idle = {
			'ws' : self.arm.coords[1],
			'ad' : self.arm.coords[0],
			'rf' : self.arm.coords[2],
			'qe' : self.arm.coords[3],
			'lr' : self.arm.coords[4],
		}
		self.normal_mode_inputs = self.normal_mode_inputs_idle.copy()
		
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
			NORMAL_MODE : {
				'w' : lambda : inc_input(self.normal_mode_inputs, 'ws', 2, limits=False, report=True),
				's' : lambda : inc_input(self.normal_mode_inputs, 'ws', -2, limits=False, report=True),
				'a' : lambda : inc_input(self.normal_mode_inputs, 'ad', -2, limits=False, report=True),
				'd' : lambda : inc_input(self.normal_mode_inputs, 'ad', 2, limits=False, report=True),
				'r'	: lambda : inc_input(self.normal_mode_inputs, 'rf', 2, limits=False, report=True),
				'f'	: lambda : inc_input(self.normal_mode_inputs, 'rf', -2, limits=False, report=True),
				'q'	: lambda : inc_input(self.normal_mode_inputs, 'qe', 2, limits=False, report=True),
				'e'	: lambda : inc_input(self.normal_mode_inputs, 'qe', -2, limits=False, report=True),
				'left'	: lambda : inc_input(self.normal_mode_inputs, 'lr', 2, limits=False, report=True),
				'right'	: lambda : inc_input(self.normal_mode_inputs, 'lr', -2, limits=False, report=True)
			}
		}
		
		pan_amt = 40
		self.key_held_actions = {
			ALL_MODES : {
				'j'	: lambda : self.pantilt.panh(-pan_amt),
				'l'	: lambda : self.pantilt.panh(pan_amt),
				'k'	: lambda : self.pantilt.panv(-pan_amt),
				'i'	: lambda : self.pantilt.panv(pan_amt),
			},
			NORMAL_MODE : {
				'w' : self.key_pressed_actions[NORMAL_MODE]['w'],
				's' : self.key_pressed_actions[NORMAL_MODE]['s'],
				'a' : self.key_pressed_actions[NORMAL_MODE]['a'],
				'd' : self.key_pressed_actions[NORMAL_MODE]['d'],
				'r' : self.key_pressed_actions[NORMAL_MODE]['r'],
				'f' : self.key_pressed_actions[NORMAL_MODE]['f'],
				'q' : self.key_pressed_actions[NORMAL_MODE]['q'],
				'e' : self.key_pressed_actions[NORMAL_MODE]['e'],
				'left' : self.key_pressed_actions[NORMAL_MODE]['left'],
				'right' : self.key_pressed_actions[NORMAL_MODE]['right'],
			}
		}
		
		self.key_released_actions = {
			ALL_MODES : {
				'j'	: lambda : self.pantilt.cancel_pan(),#self.pantilt.panh(0),
				'l'	: lambda : self.pantilt.cancel_pan(),#self.pantilt.panh(0),
				'k'	: lambda : self.pantilt.cancel_tilt(),#self.pantilt.panv(0),
				'i'	: lambda : self.pantilt.cancel_tilt(),#self.pantilt.panv(0),
			},
			NORMAL_MODE : {}
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
		pass
		#log.info('controller.handle_idle()')
		#if (self.mode == NORMAL_MODE):
		#	self.normal_mode_inputs = self.normal_mode_inputs_idle.copy()
	
	def handle_idle_long(self):
		log.info('controller.handle_idle_long()')
		#self.select_mode(PASSIVE_MODE)
		#self.sit()

	def cycle_modes(self):
		self.select_mode(self.modes.pop(0))
		self.modes.append(self.mode)
		return self.mode
	
	def select_mode(self, new_mode : str):
		self.mode = new_mode
		log.info('"%s" mode selected' % (self.mode,))
		
		#if (self.mode == MOVE_LEGS_MODE):
		#	self.start_leg = leg_position_relative(body_model=self.body_model, **self.move_legs_inputs)
		#elif (self.mode == DANCE_MODE):
		#	self.run_dance_mode(**self.dance_mode_inputs, time_ms=500)
		#elif (self.mode == YAW_MODE):
		#	self.run_yaw_mode(**self.yaw_mode_inputs, time_ms=500)

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
		
		if (self.mode == NORMAL_MODE):
			self.run_normal_mode(**self.normal_mode_inputs)

	def run_normal_mode(self, ws, ad, rf, qe, lr, time_ms=500):
		x = ad
		y = ws
		z = rf
		w = qe
		c = lr
		
		self.arm.set_coordinates_mm((x,y,z,w,lr))
		
		self.step_finish_time = time.monotonic() + (time_ms/1000.0)
