import numpy as np
#import matplotlib.pyplot as plt
from math import sin
from math import cos
from math import atan
from math import pi
from math import asin
from math import tau
import copy
import functools
import time
import logging as log
from math import radians

def equation_15(a, v, ua, A):
	dadt = v
	dvdt = (ua**2)*(A-a) - 1.5*ua*v
	return [dadt, dvdt]

def equation_19(c, v, uc, C):
	dcdt = v
	dvdt = (uc**2)*(C-c) - 1.5*uc*v
	return [dcdt, dvdt]

def normalize_angle_negative_pi_to_pi(angle):
	"""
	Normalizes an angle in radians to the range [-pi, pi].
	"""
	# Use the modulo operator to bring the angle into the range [0, 2*pi)
	# The '+ math.pi' offset helps handle negative input angles correctly with Python's % operator
	angle = (angle + pi) % tau
	
	# Subtract math.pi to shift the range from [0, 2*pi) to [-pi, pi)
	# Note: This implementation technically results in the range [-pi, pi), 
	# meaning -pi is included, but pi is the upper bound that is not included.
	angle -= pi
	return angle

def clamp(val, max_val, min_val):
	return max(min(val, max_val), min_val)

phi_wave = np.array([[0, -2, -4, -1, -3, -5], 
						  [2,  0, -2,  1, -1, -3],
						  [4,  2,  0,  3,  1, -1],
						  [1, -1, -3,  0, -2, -4],
						  [3,  1, -1,  2,  0, -2],
						  [5,  3,  1,  4,  2,  0]
						 ]) * (pi/3)
phi_tetra =np.array([[0, -3, -2,  0, -1, -2], 
						  [3,  0,  1,  3,  2,  1],
						  [2, -1,  0,  2,  1,  0],
						  [0, -3, -2,  0, -1, -2],
						  [1, -2, -1,  1,  0, -1],
						  [2, -1,  0,  2,  1,  0]
						 ]) * (pi/2)
phi_rice = np.array([[0, -2, -1,  0, -2, -1], 
						  [2,  0,  1,  2,  0,  1], 
						  [1, -1,  0,  1, -1,  0], 
						  [0, -2, -1,  0, -2, -1], 
						  [2,  0,  1,  2,  0,  1], 
						  [1, -1,  0,  1, -1,  0]
						 ]) * (2*pi/3)
phi_tripod=np.array([[0,  1,  0,  1,  0,  1], 
						  [1,  0,  1,  0,  1,  0], 
						  [0,  1,  0,  1,  0,  1],  
						  [1,  0,  1,  0,  1,  0], 
						  [0,  1,  0,  1,  0,  1], 
						  [1,  0,  1,  0,  1,  0]
						 ]) * pi

###################################################################################################################### 
class Leg:
	def __init__(self, legs, ind, ua=10, A=50, uc=10, C=0, v=5):
		self.legs = legs
		self.ind = ind
		self.name = 'leg%d' % (ind,)
		self.ua = ua
		self.A = A
		self.uc = uc
		self.C = C
		self.H = 90
		self.a = 0
		self.av = 0
		self.c = 0
		self.cv = 0
		self.theta = 0
		self.v = v
		self.d = 0
		self.r = 0
		self.prev_r = self.r
	   
	def eq15(self, dt):
		[dadt, dvdt] = equation_15(self.a, self.av, self.ua, self.A)
		self.a += dt*dadt
		self.av += dt*dvdt
		return [self.a, self.av]
		
	def eq19(self, dt):
		[dcdt, d2cdt] = equation_19(self.c, self.cv, self.uc, self.C)
		self.c += dt*dcdt
		self.cv += dt*d2cdt
		return [self.c, self.cv]
	
	def eq17(self):
		self.d = 0

		i = self.legs.legs.index(self)

		for j in range(0,i):
			leg = self.legs.legs[j]
			phase_offset = self.legs.phi[i, j]
			phase = normalize_angle_negative_pi_to_pi(leg.theta - self.theta - phase_offset)
			self.d += (1/self.v)*sin(phase/2)
		self.d *= self.legs.alpha
		return self.d           

	def eq18(self, dt):
		dtheta = 2*self.v*(pi+atan(self.d))
		self.theta = self.theta + dt*dtheta
		return self.theta

	def eq16(self):
		self.prev_r = self.r
		self.r = self.a*sin(self.theta) + self.c
		return self.r

	def eq20(self):
		num = self.r - self.c - self.a*sin(self.legs.selected_gait)
		den = self.a*(1- sin(self.legs.selected_gait))
		return num/den

	def eq21(self):
		'''step height'''
		if (self.r > (self.c + self.a*sin(self.legs.selected_gait))): #swing phase
			#H = 90 #maximum height of the leg that is physically achievable
			h = ((self.a+self.c)/(2*self.a))*self.H
			return (self.eq20()**2)*h
		else: #stance phase
			return 0
	
	def eq22_1(self):
		if (self.a == 0):
			self.a = 0.0000000001
		num = asin((self.r-self.c)/self.a) - self.legs.selected_gait
		den = (pi/2) - self.legs.selected_gait
		return -1 + clamp(num/den, 1, 0)

	def eq22_2(self):
		if (self.a == 0):
			self.a = 0.0000000001
		num = self.legs.selected_gait - asin((self.r-self.c)/self.a)
		den = (pi/2) + self.legs.selected_gait
		return -1 + clamp(num/den, 1, 0)

	def eq23(self):
		'''horizontal displacement'''
		dr = self.r - self.prev_r
		sign_dr = 1 if (dr >= 0) else -1
		#return sign_dr*(self.a/2)*self.eq22_2()
		if (self.r > (self.c + self.a*sin(self.legs.selected_gait))): #swing phase
			return sign_dr*(self.a/2)*self.eq22_1()
		else: #elif (self.r < self.c + self.a*sin(self.legs.selected_gait)): #stance phase
			return sign_dr*(self.a/2)*self.eq22_2()

GAIT_CONFIGS = {
		'wave': {
			'phase_shift_matrix' : phi_wave,
			'gait_phi' : pi/3
		},
		'tetrapod': {
			'phase_shift_matrix' : phi_tetra,
			'gait_phi' : pi/4
		},
		'rice': {
			'phase_shift_matrix' : phi_rice,
			'gait_phi' : pi/6
		},
		'tripod': {
			'phase_shift_matrix' : phi_tripod,
			'gait_phi' : 0
		}
	}

class Legs:
	def __init__(self, alpha=1, selected_gait_name='wave'):
		self.alpha = alpha
		self.v=0.25 #frequency of the cpg oscillators
		self.last_update = time.monotonic()
		self.yaw = 0
		self.leg1 = Leg(self,1, v=self.v)
		self.leg2 = Leg(self,2, v=self.v)
		self.leg3 = Leg(self,3, v=self.v)
		self.leg4 = Leg(self,4, v=self.v)
		self.leg5 = Leg(self,5, v=self.v)
		self.leg6 = Leg(self,6, v=self.v)

		# L1|----|L2
		#   |    |
		# L3|    |L4
		#   |    |
		# L5|----|L6
		
		#self.legs = [self.leg1, self.leg2, self.leg3, self.leg4, self.leg5, self.leg6]
		self.legs = [self.leg2, self.leg4, self.leg6, self.leg5, self.leg3, self.leg1]


		self.selected_gait_name = selected_gait_name
		self.phi = GAIT_CONFIGS[self.selected_gait_name]['phase_shift_matrix']
		self.selected_gait = GAIT_CONFIGS[self.selected_gait_name]['gait_phi']
		self.prev_phi = None
		self.prev_selected_gait = None
		self.new_phi = None
		self.new_selected_gait = None
		self.gait_transition_time = None #time to transition between gaits
		self.gait_transition_time_remaining = None

		print(self)

	def set_A(self, a):
		for leg in self.legs:
			leg.A = a if (a > 5) else 0
			leg.H = 90 if (a > 0) else 0
	
	def set_yaw(self, yaw_degrees):
		self.yaw = radians(yaw_degrees)

	def __str__(self):
		return 'Legs(%s)' % (self.selected_gait_name,)

	def new_gait(self, selected_gait_name):
		if (selected_gait_name == self.selected_gait_name):
			return
		#print('transition from gait %s to %s' % (self.selected_gait_name, selected_gait_name))
		self.selected_gait_name = selected_gait_name
		self.prev_phi = self.phi
		self.prev_selected_gait = self.selected_gait
		self.new_phi = GAIT_CONFIGS[self.selected_gait_name]['phase_shift_matrix']
		self.new_selected_gait = GAIT_CONFIGS[self.selected_gait_name]['gait_phi']
		self.gait_transition_time = 1/self.v #set transition time to the period of the oscillators
		self.gait_transition_time_remaining = self.gait_transition_time
		
	
	def update(self, t=None, history=None):
		if (t is None):
			t = time.monotonic()
		dt = t - self.last_update
		#log.debug(dt)
		self.last_update = time.monotonic()
		if (self.gait_transition_time_remaining is not None):
			self.phi = self.prev_phi + (self.new_phi - self.prev_phi) * (dt/self.gait_transition_time)
			self.selected_gait = self.prev_selected_gait + (self.new_selected_gait - self.prev_selected_gait) * (dt/self.gait_transition_time)
			self.gait_transition_time_remaining -= dt
			if (self.gait_transition_time_remaining <= 0): #gait transition is complete
				self.gait_transition_time_remaining = None
				self.phi = self.new_phi
				self.selected_gait = self.new_selected_gait
		
		to_return = np.empty([6, 4])
		for i,leg in zip((2,4,6,5,3,1),self.legs): #[self.leg2, self.leg4, self.leg6, self.leg5, self.leg3, self.leg1]
			(a,av) = leg.eq15(dt)
			(c,cv) = leg.eq19(dt)
			d = leg.eq17()
			theta = leg.eq18(dt)
			r = leg.eq16()
	
			z = leg.eq21()
	
			B1 = leg.eq22_1()
			B2 = leg.eq22_2()
			
			# L1|----|L2
			#   |    |
			# L3|    |L4
			#   |    |
			# L5|----|L6			
			u = leg.eq23()
			if (i==1) or (i==3) or (i==5):
				u = -u
	
			x = sin(self.yaw) * u
			y = cos(self.yaw) * u

			to_return[i-1,0] = x 
			to_return[i-1,1] = y 
			to_return[i-1,2] = z
			to_return[i-1,3] = 0

			if (type(history) is not dict):
				continue
			h = history[leg.name]
			h['a'].append(a)
			h['av'].append(av)
			h['c'].append(c)
			h['cv'].append(cv)
			h['d'].append(d)
			h['theta'].append(theta)
			h['r'].append(r)
			h['z'].append(z)
			h['B1'].append(B1)
			h['B2'].append(B2)
			h['u'].append(u)
			h['x'].append(x)
			h['y'].append(y)
			
		return to_return