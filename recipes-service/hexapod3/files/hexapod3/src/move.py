"""
Functions to calculate linear and angular movement for the hexapod.

These are a collection of functions to find how the hexapod will take steps to
move linearly in any direction aor to turn itself in the x-y plane about the
z axis.

Functions
---------
emgToTurn:
	Turns a dynamic angle based on a normalized EMG input.
emgToWalk:
	Walks a dynamic distance based a normalized EMG input.
omniWalk:
	Walks in any direction based on the previous step.
resetStance:
	Completes the final step in simultaneous turning and walking.
resetTurnStance:
	Completes the final step in turning to a neutral stance.
resetWalkStance:
	Completes the final step in walking to a neutral stance.
simultaneousWalkTurn:
	Makes a step that allows both a turn and a walk in any direction.
stepForward:
	Calculate the x, y, z position updates to move in a step in a direction.
stepTurn:
	Calculate the positions of each foot when turning about an angle.
stepTurnFoot:
	Calculate the position of a foot when turning the hexapod about an angle.
turn:
	Creates the series of feet positions to turn the hexapod about the z axis.
walk:
	Creates a series of feet positions to use when walking in a direction.
"""
from math import degrees, radians, sin, cos, atan2, hypot
import numpy as np
from math import sqrt
from leg import getFeetPos, recalculateLegAngles, legModel
from typing import Tuple
import logging as log 

def walk(turn_feet_positions: np.ndarray,
						 right_foot: bool, previous_walk_step: float = 0,
						 previous_walk_angle: float = 0,
						 previous_turn_angle: float = 0,
						 walk_distance: float = 30,
						 walk_angle: float = 90,
						 turn_angle: float = 15,
						 step_height: float =15,
						 step_depth: float=10,
						 z_resolution=6) -> Tuple[np.ndarray, bool, float, float, float,np.ndarray]:
	"""
	Makes a step that allows both a turn and a walk in any direction.

	Allows the hexapod to walk and/or turn at the same time by finding the
	feet positions to turn the hexapod and then applying the translation from
	walking.

	Parameters
	----------
	turn_feet_positions: np.ndarray
		The 6x3 numpy array containing the locations of the feet after a turn.
	right_foot: bool
		An indicator if the right or left set of legs are taking the step.
		The "right" set are legs 0, 2, and 4 and the "left" are 1, 3, and 5.
	previous_walk_step: float, default=0
		The distance the hexapod walked the last time this function was called.
	previous_walk_angle: float, default=0
		The angle the hexapod walked the last time this function was called.
	previous_turn_angle: float
		The angle the hexapod turned the last time this function was called.
	walk_distance: float, default=30
		The step size of the current step.
	walk_angle: float, default=90
		The angle that the hexapod will walk in.
	turn_angle: float, default=15
		The angle that the hexapod will turn to.

	Returns
	-------
	[turn_feet_positions, right_foot, previous_walk_step, previous_walk_angle,
	 previous_turn_angle, move_positions]:
		Tuple[np.ndarray, bool, float, float, float, np.ndarray]

		The updated input parameters for the next time the function is run as
		well as the positions to move to.

	See Also
	--------
	emgToWalk:
		Walks a dynamic distance based a normalized EMG input.
	emgToTurn:
		Turns a dynamic angle based on a normalized EMG input.
	omniWalk:
		Walks in any direction based on the previous step.

	Notes
	-----
	The walking code is the same as the omniWalk code. Simultaneous walking
	and turning is done by having the rotation of the hexapod found first
	before the translation of the hexapod. This is the same concept as done
	when applying a rotation and translation matrix to a point.
	"""
	
	turn_positions = step(turn_feet_positions, right_foot=right_foot, z_resolution=z_resolution)

	# components of previous step
	previous_x = previous_walk_step * cos(radians(previous_walk_angle))
	previous_y = previous_walk_step * sin(radians(previous_walk_angle)) 
	# components of desired step
	current_x = walk_distance * cos(radians(walk_angle))
	current_y = walk_distance * sin(radians(walk_angle))
	# combined step
	x = previous_x + current_x
	y = previous_y + current_y
	step_magnitude = hypot(x, y)
	step_angle = degrees(atan2(y, x))
	
	if (turn_angle == 0):
		walk_positions = stepForward(step_angle=step_angle, distance=step_magnitude, right_foot=right_foot, step_height=step_height, step_depth=step_depth, z_resolution=z_resolution)
	else:
		walk_positions = stepForwardTurn(step_angle=90, distance=30 if (turn_angle<0) else -30, right_foot=right_foot, step_height=step_height, step_depth=step_depth, z_resolution=z_resolution)

	move_positions = turn_positions + walk_positions

	#log.info('walk_positions')
	#log.info(walk_positions)
	#log.info('turn_positions')
	#log.info(turn_positions)
	#log.info('move_positions')
	#log.info(move_positions)

	previous_walk_step = walk_distance
	previous_walk_angle = walk_angle
	previous_turn_angle = turn_angle
	right_foot = not right_foot

	return (turn_positions[-1,:,:], right_foot, previous_walk_step, previous_walk_angle, previous_turn_angle, list(move_positions))


def stepForward(step_angle: float = 90, distance: float = 30,
				step_height: float = 15, right_foot: bool = True,
				step_depth: float=20, z_resolution: float = 10) -> np.ndarray:

	z = np.array([(-((x**2)/step_height) + step_height) for x in np.linspace(-step_height, step_height, z_resolution)])
	x = np.linspace(0, distance * cos(radians(step_angle)), z.size)
	y = np.linspace(0, distance * sin(radians(step_angle)), z.size) 
	#log.debug(x)
	#log.debug(y)
	#log.debug(z)
	
	lead_foot_left = np.dstack((x, -y, z)).reshape(z.size, 1, 3)
	lead_foot_right = np.dstack((x, y, z)).reshape(z.size, 1, 3)
	drag_foot_left = np.dstack((- x, y, np.full(z.size, -step_depth))).reshape(z.size, 1, 3)
	drag_foot_right = np.dstack((- x, -y, np.full(z.size, -step_depth))).reshape(z.size, 1, 3)

	# L1|----|L2
	#   |    |
	# L3|    |L4
	#   |    |
	# L5|----|L6
	# legs 2, 3, and 6 are the right legs and legs 1, 4, and 5 as the left legs
	#                            L1(left)        L2(right)         L3(left)       L4(right)       L5(left)       L6(right)
	if right_foot:  # right foot
		feet = np.concatenate((drag_foot_left, lead_foot_right, lead_foot_left, drag_foot_right, drag_foot_left, lead_foot_right),axis=1)
	else:
		feet = np.concatenate((lead_foot_left, drag_foot_right, drag_foot_left,lead_foot_right, lead_foot_left, drag_foot_right), axis=1)

	#log.info(feet)
	return feet


def stepForwardTurn(step_angle: float = 90, distance: float = 30,
				step_height: float = 15, right_foot: bool = True,
				step_depth: float=20, z_resolution: float = 10) -> np.ndarray:

	z = np.array([(-((x**2)/step_height) + step_height) for x in np.linspace(-step_height, step_height, z_resolution)])
	x = np.linspace(0, distance * cos(radians(step_angle)), z.size)
	y = np.linspace(0, distance * sin(radians(step_angle)), z.size)
	
	
	lead_foot = np.dstack((x, y, z)).reshape(z.size, 1, 3)
	drag_foot = np.dstack((- x, -y, np.full(z.size, -step_depth))).reshape(z.size, 1, 3)	

	if right_foot:  # right foot
		feet = np.concatenate((drag_foot, lead_foot, lead_foot, drag_foot, drag_foot, lead_foot),axis=1)
	else:
		feet = np.concatenate((lead_foot, drag_foot, drag_foot,lead_foot, lead_foot, drag_foot), axis=1)

	#log.info(feet)
	return feet

def step(feet_pos: np.ndarray, right_foot: bool = True, z_resolution: float = 10) -> np.ndarray:
	for i in range(6):
		right_active = right_foot if ((i==1)or(i==2)or(i==5)) else not right_foot
		footstep = step_foot(foot_x=feet_pos[i, 0], foot_y=feet_pos[i, 1], foot_z=feet_pos[i, 2], right_foot=right_active,z_resolution=z_resolution)
		previous_foot = footstep if (i==0) else np.concatenate((previous_foot, footstep), axis=1)

	return previous_foot


def step_foot(foot_x: float, foot_y: float, foot_z: float, right_foot: bool = True, z_resolution: float = 10) -> np.ndarray:
	radius = hypot(foot_x, foot_y)
	foot_angle = degrees(atan2(foot_y, foot_x))

	step_height=0
	z = np.array([sqrt((step_height**2)-(x**2)) + foot_z for x in np.linspace(-step_height, step_height, z_resolution)])	
	x = np.empty(z.size)
	y = np.empty(z.size)
	angles = np.linspace(foot_angle, foot_angle, z.size)
	for i, angle in enumerate(angles):
		x[i] = radius * cos(radians(angle))
		y[i] = radius * sin(radians(angle))
 
	if right_foot:  # right foot
		angles = np.linspace(foot_angle, foot_angle, z.size)
	else:
		angles = np.linspace(foot_angle, foot_angle, z.size)
		z = np.zeros(z.size) + foot_z

	for i, angle in enumerate(angles):
		x[i] = radius * cos(radians(angle))
		y[i] = radius * sin(radians(angle))

	#log.info(z)

	return np.dstack((x, y, z)).reshape(z.size, 1, 3)
