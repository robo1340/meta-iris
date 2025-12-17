"""
Functions to generate, change, and use the model of the hexapod's legs.

These functions take the leg dimensions and servo angles to find their position
or can do the opposite and find the angles from the dimensions. Together,
these functions define the leg model and allow the user to update and pull
relevant information about the legs.

Functions
---------
getFeetPos:
	Output the x, y, z position of the feet of the hexapod.
legAngle:
	Finds the angles for the coax, femur, and tibia leg segments.
legModel:
	Generates the model of the legs based on the servo angles of the legs.
legPos:
	Finds the positions for the leg segments.
recalculateLegAngles:
	Finds the coax, femur, and tibia angles of each leg.
startLegPos:
	Find the neutral position of the hexapod.
"""
import numpy as np
from numpy.linalg import inv
from math import degrees, sin, cos, acos, atan2, sqrt, pi
from rotation import yRot, zRot
from typing import List
import logging as log


np.set_printoptions(precision=1, suppress=True, floatmode='fixed')

def getFeetPos(leg_model: np.ndarray) -> np.ndarray:
	"""
	Output the x, y, z position of the feet of the hexapod.

	Return the current positions of the ends of the legs or where the feet
	of the hexapod currently are.

	Parameters
	----------
	leg_model: np.ndarray
		the 4x3x6 numpy array that holds the locations of the coax, femur,
		and tibia servos as well as the feet end positions.

	Returns
	-------
	feet_positions: np.ndarray
		The 6x3 numpy array of where each leg of the hexapod ends as an
		x, y, z point.
	"""
	feet_positions = np.empty([6, 3])
	for i in range(6):
		feet_positions[i, :] = leg_model[3, :, i]
	return feet_positions


def legAngle(x: float, y: float, z: float, coax: float = 29, femur: float = 76, tibia: float = 97) -> List[float]:
	"""
	Finds the angles for the coax, femur, and tibia leg segments.

	Takes the foot position, or end point of the leg, and calculates what angle
	each segment of the legs should have to achieve that end position.

	Parameters
	----------
	x: float
		The x location of the foot.
	y: float
		The y location of the foot.
	z: float
		The z location of the foot.
	coax: float, default=26.34
		The length of the coax segment in millimeters or the segment from the
		body attachment point to the beginning of the femur.
	femur: float, default=76.2
		The length of the femur segment in millimeters or the segment from the
		beginning of the femur to the start of the tibia.
	tibia: float, default=88.32
		The length of the tibia segment in millimeters or the segment from the
		start of the tibia to the foot of the leg.

	Returns
	-------
	[coax_angle, femur_angle, tibia_angle]: List[float]
		A list of the three servo angles for the leg segments.

	See Also
	--------
	recalculateLegAngles:
		Finds the coax, femur, and tibia angles of each leg.

	Notes:
		Refer to the Kinematics Calculations document in the Docs folder for
		how the equations in this function were found.
	"""
	coax_angle = degrees(atan2(y, x))
	coax_rot = zRot(-coax_angle)
	#log.debug('%2.2f %2.2f %2.2f' % (x,y,z))
	#log.debug(coax_angle)
	leg_rotated = np.matmul(inv(coax_rot), np.array([[x, y, z]]).T)
	femur_angle = degrees(acos((tibia ** 2 - femur ** 2 - leg_rotated[2] ** 2
								- (leg_rotated[0] - coax) ** 2) /
							   (-2 * femur * (sqrt(leg_rotated[2] ** 2 +
												   (leg_rotated[0] - coax)**2))
								)))\
		- degrees(atan2(-leg_rotated[2],
						(leg_rotated[0] - coax)))
	tibia_angle = degrees(acos((leg_rotated[2] ** 2 + (leg_rotated[0] - coax)
								** 2 - femur ** 2 - tibia ** 2) /
							   (-2 * femur * tibia))) - 90

	if abs(coax_angle) <= 1e-10:
		coax_angle = 0

	if abs(femur_angle) <= 1e-10:
		femur_angle = 0

	if abs(tibia_angle) <= 1e-10:
		tibia_angle = 0

	return [coax_angle, femur_angle, tibia_angle]


def legModel(leg_angles: np.ndarray, body_model: np.ndarray) -> np.ndarray:
	"""
	Generates the model of the legs based on the servo angles of the legs.

	Recreates the leg model from all of the servo angles of the legs. This
	method is used when walking and turning to update the model of the robot
	for the next step.

	Parameters
	----------
	leg_angles: np.ndarray
		A 6x3 numpy array of each leg's coax, femur, and tibia servo angles.
	body_model: np.ndarray
		The 7x3 numpy array containing the locations of the coax servos.

	Returns
	-------
	leg_model: np.ndarray
		the 4x3x6 numpy array that holds the locations of the coax, femur,
		and tibia servos as well as the feet end positions.

	See Also
	--------
	legPos:
		Finds the positions for the leg segments.
	"""
	leg_model = np.empty([4, 3, 6])
	for i in range(6):
		# leg model takes the coax angle, femur angle, tibia angle, model of
		# the hexapod body, leg number
		leg_model[:, :, i] = legPos(leg_angles[i][0], leg_angles[i][1],
									leg_angles[i][2], body_model, i)
	return leg_model


def legPos(coax_angle: float, femur_angle: float, tibia_angle: float,
		   body_model: np.ndarray, leg_num: int, coax: float = 26.34,
		   femur: float = 76.2, tibia: float = 88.32) -> np.ndarray:
	"""
	Finds the positions for the leg segments.

	Takes the angles of all of the leg servos, the leg dimensions, and the
	model of the body to find the locations of each segment of a leg.

	Parameters
	----------
	coax_angle: float
		The angle of the coax servo or the servo directly attached to the body.
	femur_angle: float
		The angle of the femur servo or the servo that moves the knee.
	tibia_angle: float
		The angle of the tibia servo or the servo that moves the foot.
	body_model: np.ndarray
		The 7x3 numpy array containing the locations of the coax servos.
	leg_num: int
		The number of the leg. leg 0 is the front right leg and each
		successive leg is clockwise looking down at the hexapod.
	coax: float, default=26.34
		The length of the coax segment in millimeters or the segment from the
		body attachment point to the beginning of the femur.
	femur: float, default=76.2
		The length of the femur segment in millimeters or the segment from the
		beginning of the femur to the start of the tibia.
	tibia: float, default=88.32
		The length of the tibia segment in millimeters or the segment from the
		start of the tibia to the foot of the leg.

	Returns
	-------
	leg_positions: np.ndarray
		The global positions of the body attachment point, the coax end point,
		the femur end point, and the tibia end point or foot of the leg. This
		is a 1x4 numpy array.

	See Also
	--------
	legModel:
		Generates the model of the legs based on the servo angles of the legs.

	Notes
	-----
	The default leg segment lengths are from the CAD model I initially made.
	"""
	coax_rot = zRot(coax_angle)
	femur_rot = np.matmul(yRot(femur_angle), coax_rot)
	tibia_rot = np.matmul(yRot(tibia_angle), femur_rot)

	leg_coax = np.matmul(inv(coax_rot), np.array([[coax, 0, 0]]).T) + np.array([body_model[leg_num, :]]).T
	leg_femur = np.matmul(inv(femur_rot), np.array([[femur, 0, 0]]).T) + leg_coax
	leg_tibia = np.matmul(inv(tibia_rot), np.array([[0, 0, -tibia]]).T) + leg_femur

	leg_positions = np.concatenate((np.array([body_model[leg_num, :]]), leg_coax.T, leg_femur.T, leg_tibia.T), axis=0)
	return leg_positions


def recalculateLegAngles(feet_positions: np.ndarray,
						 body_model: np.ndarray) -> np.ndarray:
	"""
	Finds the coax, femur, and tibia angles of each leg.

	Finds the coax, femur, and tibia servo angles of each leg based on the
	location of each leg's foot.

	Parameters
	----------
	feet_positions: np.ndarray
		A 6x3 numpy array of each leg's end position in x, y, z.
	body_model: np.ndarray
		The 7x3 numpy array containing the locations of the coax servos.

	Returns
	-------
	leg_angles: np.ndarray
		A 6x3 numpy array of each leg's coax, femur, and tibia servo angles.

	See Also
	--------
	legAngle:
		Finds the angles for the coax, femur, and tibia leg segments.
	"""
	leg_angles = np.empty([6, 3])
	for i in range(6):
		leg_angles[i, :] = legAngle(feet_positions[i, 0] - body_model[i, 0],
									feet_positions[i, 1] - body_model[i, 1],
									feet_positions[i, 2] - body_model[i, 2])
	return leg_angles


def startLegPos(body_model: np.ndarray, width_mm=107, length_mm=214, start_x_offset=80, start_height: float = 60, corner_leg_rotation_offset=30, return_position=False) -> np.ndarray:
	"""
	Find the neutral position of the hexapod.

	Create the starting angles of the legs on the hexapod based on the
	standing radius on the ground and height off the ground.

	Parameters
	----------
	body_model: np.ndarray
		The 7x3 numpy array containing the locations of the coax servos.
	start_radius: float, default=180
		The radius in millimeters that the feet make with the ground when
		equally spaced around the hexapod
	start_height: float, default=60
		The distance off the ground from the top of the legs to the bottom
		in millimeters.

	Returns
	-------
	start_leg: np.ndarray
		The 6x3 numpy array of servo angles for the neutral position of the
		hexapod

	See Also
	--------
	recalculateAngles:
		Finds the coax, femur, and tibia angles of each leg.
	"""

	# L1|----|L2
	#   |    |
	# L3|    |L4
	#   |    |
	# L5|----|L6
	
	#log.debug('corner_leg_rotation_offset %d' % (corner_leg_rotation_offset,))
	corner_rotate = zRot(corner_leg_rotation_offset) #rotate clockwise
	rotate_180 = lambda m : np.matmul(zRot(180), m)
	
	relative_position = lambda : np.array([start_x_offset, 0, -start_height]) #the starting position of all the legs relative to their own local coordinate frame
	
	leg1 = np.matmul(inv(corner_rotate) ,     rotate_180(relative_position()))
	leg2 = np.matmul(inv(corner_rotate), relative_position())
	leg3 = rotate_180(relative_position())
	leg4 = relative_position()
	leg5 = np.matmul(corner_rotate, rotate_180(relative_position()))
	leg6 = np.matmul(corner_rotate, relative_position())
	
	start_leg_pos = np.stack((leg1, leg2, leg3, leg4, leg5, leg6), axis=0)
	#log.info(start_leg_pos)
	#adjustments to make the leg positions relative to the origin of the hexapod
	adjustment = np.array([
							  [-(width_mm/2),  length_mm/2, 0],
							  [(width_mm/2),   length_mm/2, 0],
							  [-(width_mm/2),  0,           0],
							  [(width_mm/2),   0,           0],
							  [-(width_mm/2), -length_mm/2, 0],
							  [(width_mm/2),  -length_mm/2, 0]
							 ])	
	start_leg_pos = start_leg_pos + adjustment
	#log.info(start_leg_pos)
	
	if (return_position == True):
		return start_leg_pos
	else:
		return recalculateLegAngles(start_leg_pos, body_model)


def leg_position_relative(x, y, z, body_model: np.ndarray, width_mm=107, length_mm=214, **lkwargs):
	
	relative_position = lambda : np.array([x, y, z]) #the starting position of all the legs relative to their own local coordinate frame
	
	rotate_180 = lambda m : np.matmul(zRot(180), m)
	leg1 = rotate_180(relative_position())
	leg2 = relative_position()
	leg3 = rotate_180(relative_position())
	leg4 = relative_position()
	leg5 = rotate_180(relative_position())
	leg6 = relative_position()
	
	start_leg_pos = np.stack((leg1, leg2, leg3, leg4, leg5, leg6), axis=0)
	#log.info(start_leg_pos)
	#adjustments to make the leg positions relative to the origin of the hexapod
	adjustment = np.array([
							  [-(width_mm/2),  length_mm/2, 0],
							  [(width_mm/2),   length_mm/2, 0],
							  [-(width_mm/2),  0,           0],
							  [(width_mm/2),   0,           0],
							  [-(width_mm/2), -length_mm/2, 0],
							  [(width_mm/2),  -length_mm/2, 0]
							 ])	
	start_leg_pos = start_leg_pos + adjustment
	#log.info(start_leg_pos)
							 
	start_leg = recalculateLegAngles(start_leg_pos, body_model)
	return start_leg