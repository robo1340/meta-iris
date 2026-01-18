from typing import Tuple
import numpy as np
from numpy.linalg import inv
from rotation import xRot, yRot, zRot, dh_transform
from math import sin, tan, atan2, pi, radians, degrees, acos, pi, atan2, sqrt
import logging as log
from typing import List
from rotation import ypr

COXA_LENGTH = 29
FEMUR_LENGTH = 76
TIBIA_LENGTH = 101

dh_model = [
	{ #coxa
		'joint_angle' 	: 0,
		'joint_mult' 	: 1,
		'theta_offset' 	: -90,
		'alpha' : 90,
		'd' : 0,
		'a' : COXA_LENGTH
	},
	{ #femur
		'joint_angle' 	: 0,
		'joint_mult' 	: 1,
		'theta_offset' 	: 0,
		'alpha' : 0,
		'd' : 0,
		'a' : FEMUR_LENGTH
	},
	{ #tibia
		'joint_angle' 	: 0,
		'joint_mult' 	: 1,
		'theta_offset' 	: -90,
		'alpha' : 0,
		'd' : 0,
		'a' : TIBIA_LENGTH
	}
]

def float_equal(a,b,delta=0.001):
	return (a-b) < delta


def ik3(coords,  servo_dict : dict, coxa=COXA_LENGTH, femur=FEMUR_LENGTH, tibia=TIBIA_LENGTH) -> List[float]:
	#directions of the local coordinate frames for each leg
	#    Y            Y
	#    ^            ^
	#    |            |
	# X<-. L1|----|L2 .->X
	#    Z   |    |   Z
	#      L3|    |L4
	#        |    |
	#      L5|----|L6
	
	try:
		x,y,z = coords
		coxa_angle = degrees(atan2(y, x))
		coax_rot = zRot(-coxa_angle)
		#log.debug('%2.2f %2.2f %2.2f' % (x,y,z))
		#log.debug(coxa_angle)
		leg_rotated = np.matmul(inv(coax_rot), np.array([[x, y, z]]).T)
		femur_angle = degrees(acos((tibia ** 2 - femur ** 2 - leg_rotated[2] ** 2
									- (leg_rotated[0] - coxa) ** 2) /
								   (-2 * femur * (sqrt(leg_rotated[2] ** 2 +
													   (leg_rotated[0] - coxa)**2))
									)))\
			- degrees(atan2(-leg_rotated[2],
							(leg_rotated[0] - coxa)))
		tibia_angle = degrees(acos((leg_rotated[2] ** 2 + (leg_rotated[0] - coxa)
									** 2 - femur ** 2 - tibia ** 2) /
								   (-2 * femur * tibia))) - 90

		if abs(coxa_angle) <= 1e-10:
			coxa_angle = 0

		if abs(femur_angle) <= 1e-10:
			femur_angle = 0

		if abs(tibia_angle) <= 1e-10:
			tibia_angle = 0

		dh_model[0]['joint_angle'] = servo_dict['coxa'].my_limits(coxa_angle-90)+90
		dh_model[1]['joint_angle'] = servo_dict['femur'].my_limits(femur_angle)
		dh_model[2]['joint_angle'] = servo_dict['tibia'].my_limits(tibia_angle-90)+90
		
		#dh_model[0]['joint_angle'] = coxa_angle
		#dh_model[1]['joint_angle'] = femur_angle
		#dh_model[2]['joint_angle'] = tibia_angle
		
		result = np.linalg.multi_dot( (dh_transform(**dh_model[0]), dh_transform(**dh_model[1]), dh_transform(**dh_model[2])) )
		
		xf = -result[1,3]
		yf = result[0,3]
		zf = result[2,3]
		
		if (not float_equal(x, xf)) or (not float_equal(y, yf)) or (not float_equal(z, zf)):
			log.debug('%s not reachable' % (coords,))
			#log.debug('%s not reachable | actual %s' % (coords,[xf,yf,zf]))
			return None
		return [coxa_angle, femur_angle, tibia_angle]
	except ValueError as ex:
		log.error(ex)
		return None

class Model:
	def __init__(self, servo_dicts):
		self.servo_dicts = servo_dicts
		self.body_ypr = ypr(rotx=-5, roty=0, rotz=0, x=0, y=0, z=0)
		self.leg_coords_local = None
		self.init_leg_positions()

	def set_body(self, yaw, pitch, roll, tx, ty, tz):
		self.body_ypr = ypr(rotx=pitch, roty=roll, rotz=yaw, x=tx, y=ty, z=tz)

	def init_leg_positions(self, start_x_offset=70, start_height=80, corner_leg_rotation_offset=30):
		
		#the starting position of all the legs relative to their own local coordinate frame
		leg_coords = np.array([ [start_x_offset, 0, -start_height, 1],
								[start_x_offset, 0, -start_height, 1],
								[start_x_offset, 0, -start_height, 1],
								[start_x_offset, 0, -start_height, 1],
								[start_x_offset, 0, -start_height, 1],
								[start_x_offset, 0, -start_height, 1]
								])
		return self.set_leg_positions(leg_coords, corner_leg_rotation_offset=corner_leg_rotation_offset)

	def calculate_leg_angles(self, feet_positions: np.ndarray) -> np.ndarray:
		leg_angles = np.empty([6, 3])
		for i in range(6):
			angles = ik3(feet_positions[i, 0:3], servo_dict=self.servo_dicts[i])
			if (angles is None):
				return None
			leg_angles[i, :] = angles
		#log.info(leg_angles)
		return leg_angles

	def set_leg_positions(self, neutral_leg_coords: np.ndarray, gait_offset: np.ndarray=None, width_mm=107, length_mm=214, corner_leg_rotation_offset=30) -> np.ndarray:
		if (self.leg_coords_local is None):
			self.leg_coords_local = neutral_leg_coords.copy()
		leg_coords = neutral_leg_coords.copy()
		# L1|----|L2
		#   |    |
		# L3|    |L4
		#   |    |
		# L5|----|L6
		
		#apply corner leg rotations if they are being used
		rotate = lambda c : ypr(0,0,c,0,0,0)
		c=corner_leg_rotation_offset
		leg_coords[0] = np.matmul(rotate(-c), leg_coords[0])
		leg_coords[1] = np.matmul(rotate(c), leg_coords[1])
		leg_coords[4] = np.matmul(rotate(c), leg_coords[4])
		leg_coords[5] = np.matmul(rotate(-c), leg_coords[5])
		
		leg1_tf = ypr(0,0,180, -width_mm/2, length_mm/2, 0) 
		leg2_tf = ypr(0,0,0,    width_mm/2, length_mm/2, 0)
		leg3_tf = ypr(0,0,180, -width_mm/2, 0, 0)
		leg4_tf = ypr(0,0,0,    width_mm/2, 0, 0)
		leg5_tf = ypr(0,0,180, -width_mm/2, -length_mm/2, 0)
		leg6_tf = ypr(0,0,0,    width_mm/2, -length_mm/2, 0)
		tfs = [leg1_tf, leg2_tf, leg3_tf, leg4_tf, leg5_tf, leg6_tf]
		
		if (gait_offset is not None):#add the gait offsets
			leg_coords = leg_coords + gait_offset
		#log.info('wrt local')
		#log.info(leg_coords)
		
		#get the leg positions with respect to the body
		for i in range(6):
			leg_coords[i] = np.matmul(tfs[i], leg_coords[i])
		#log.info('wrt body')
		#log.info(leg_coords)
		
		#apply the body rotation
		for i in range(6):
			leg_coords[i] = np.matmul(self.body_ypr, leg_coords[i])
		#log.info('wrt world frame')
		#log.info(leg_coords)
		
		#get the leg position with respect to the local coordinate frames again
		for i in range(6):
			leg_coords[i] = np.matmul(inv(tfs[i]), leg_coords[i])
			if (i%2 == 0): #leg on the left side, reverse the y-axis
				leg_coords[i][1] *= -1
		#log.info('wrt local')
		#log.info(leg_coords)
		
		return self.calculate_leg_angles(leg_coords)		
	
