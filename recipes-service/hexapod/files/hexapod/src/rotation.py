from math import radians, cos, sin
import numpy as np


def xRot(theta: float) -> np.ndarray:
    angle = radians(theta)
    mat = np.array([[1, 0, 0],
                   [0, cos(angle), sin(angle)],
                   [0, -sin(angle), cos(angle)]])
    return mat


def yRot(theta: float) -> np.ndarray:
    angle = radians(theta)
    mat = np.array([[cos(angle), 0, sin(angle)],
                   [0, 1, 0],
                   [-sin(angle), 0, cos(angle)]])
    return mat


def zRot(theta: float) -> np.ndarray:
    angle = radians(theta)
    mat = np.array([[cos(angle), sin(angle), 0],
                   [-sin(angle), cos(angle), 0],
                   [0, 0, 1]])
    return mat

def ypr(rotx, roty, rotz, x, y, z):
	alpha = radians(rotz) #yaw
	beta  = radians(roty) #roll
	gamma = radians(rotx) #pitch
	
	mat = np.array([[cos(alpha)*cos(beta), cos(alpha)*sin(beta)*sin(gamma) - sin(alpha)*cos(gamma), cos(alpha)*sin(beta)*cos(gamma) + sin(alpha)*sin(gamma), x],
	                [sin(alpha)*cos(beta), sin(alpha)*sin(beta)*sin(gamma) + cos(alpha)*cos(gamma), sin(alpha)*sin(beta)*cos(gamma) - cos(alpha)*sin(gamma), y],
					[          -sin(beta),                                    cos(beta)*sin(gamma),                                    cos(beta)*cos(gamma), z],
					[                   0,                                                       0,                                                       0, 1]
				   ])
	return mat	
	
def dh_transform(joint_angle, joint_mult, theta_offset, alpha, d, a):
	theta = radians(joint_angle)*joint_mult + radians(theta_offset)
	#print("%s %s %s %s" % (joint_angle, joint_mult, theta_offset, theta))
	alpha = radians(alpha)
	
	mat = np.array([[cos(theta), -sin(theta)*cos(alpha),  sin(theta)*sin(alpha), a*cos(theta)],
	                [sin(theta),  cos(theta)*cos(alpha), -cos(theta)*sin(alpha), a*sin(theta)],
					[         0,             sin(alpha),             cos(alpha),            d],
					[         0,                      0,                      0,            1]
				   ])
	return mat
