import json
import os
import random
import time

CONFIG_PATH = '/hexapod/conf/config.json'

#servo calibrations are stored in the config
#for each coxa servo 0 degrees points forward and the angle progresses counter-clockwise looking from the top of the robot
#for each femur servo 0 degrees makes the femur horizontal and 90 degrees makes it point stright up
#for each tibia servo 0 degrees makes it point straight out parallel with the femur -90 makes it form a right angle with the femur pointing down
#the pws list defines the pulse widths that correspond to the angles list
#the calibration list defines the indices of angles and pws that will be used to form a linear regression

hardcoded_default_config = {
	'serial_path' 		: '/dev/ttySTM1',
	'serial_baud'		: 115200,
	'serial_timeout'	: 0.1,
	'advertisment_rate' : 15,
	'pantilt' : {
		'pan_ch' : 8,
		'tilt_ch' : 9,
		'pan_offset' : 0,
		'tilt_offset' : 0
	},
	'leg1' : {
		'coxa_servo' : {
			'angles' : [-140, -90, 0],
			'pws'	 : [900,1500, 2425],
			'ch'	 : 28,
			'calibration' : [1,2]
		},
		'femur_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [500, 1475, 2425],
			'ch'	 : 29,
			'calibration' : [1,2]
		},
		'tibia_servo' : {
			'angles' : [-160, -90, 0],
			'pws'	 : [2500, 1600, 625],
			'ch'	 : 30,
			'calibration' : [1,2]
		}
	},
	'leg2' : {
		'coxa_servo' : {
			'angles' : [-150, -90, 0],
			'pws'	 : [950, 1500, 2425],
			'ch'	 : 12,
			'calibration' : [1,2]
		},
		'femur_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2500, 1500, 575],
			'ch'	 : 13,
			'calibration' : [1,2]
		},
		'tibia_servo' : {
			'angles' : [-165, -90, 0],
			'pws'	 : [525, 1500, 2450],
			'ch'	 : 14,
			'calibration' : [1,2]
		}
	},		
	'leg3' : {
		'coxa_servo' : {
			'angles' : [-140, -90, -50],
			'pws'	 : [800,1475, 2025],
			'ch'	 : 20,
			'calibration' : [1,2]
		},
		'femur_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [550, 1500, 2450],
			'ch'	 : 21,
			'calibration' : [1,2]
		},
		'tibia_servo' : {
			'angles' : [-170, -90, 0],
			'pws'	 : [2450, 1450, 500],
			'ch'	 : 22,
			'calibration' : [1,2]
		}
	},	
	'leg4' : {
		'coxa_servo' : {
			'angles' : [-135, -90, -55],
			'pws'	 : [2050,1625, 1050],
			'ch'	 : 4,
			'calibration' : [1,2]
		},
		'femur_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2500, 1575, 625],
			'ch'	 : 5,
			'calibration' : [1,2]
		},
		'tibia_servo' : {
			'angles' : [-165, -90, 0],
			'pws'	 : [550, 1500, 2500],
			'ch'	 : 6,
			'calibration' : [1,2]
		}
	},			
	'leg5' : {
		'coxa_servo' : {
			'angles' : [-180, -90, -45],
			'pws'	 : [500,1475, 1975],
			'ch'	 : 16,
			'calibration' : [0,1]
		},
		'femur_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [600, 1550, 2475],
			'ch'	 : 17,
			'calibration' : [1,2]
		},
		'tibia_servo' : {
			'angles' : [-160, -90, 0],
			'pws'	 : [2500, 1550, 575],
			'ch'	 : 18,
			'calibration' : [1,2]
		}
	},		
	'leg6' : {
		'coxa_servo' : {
			'angles' : [-180, -90, -30],
			'pws'	 : [700, 1575, 2125],#HS-485 values
			'ch'	 : 0,
			'calibration' : [0,1]
		},
		'femur_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2450, 1525, 600],
			'ch'	 : 1,
			'calibration' : [1,2]
		},
		'tibia_servo' : {
			'angles' : [-170, -90, -5],
			'pws'	 : [625, 1525, 2425],
			'ch'	 : 2,
			'calibration' : [1,2]
		}
	}
}

#for each key not in dst, place the key and value in src
def load_dict_values(dst, src):
	for key, val in src.items():
		if not key in dst:
			dst[key] = val

#check all keys in the config
#where a key can't be found, insert the hardcoded default
def verify_config(config):
	try:
		load_dict_values(config, hardcoded_default_config)
		return config
	except BaseException:
		print('Error , exception occurred while parsing config file, reverting to defaults')
		return hardcoded_default_config

def write_default_config(path):
	try:
		os.makedirs(os.path.dirname(path), exist_ok=True)
		with open(path, 'w') as f:
			json.dump(hardcoded_default_config, f, indent=2)	
	except BaseException as ex:
		pass

def read_config():
	to_return = {}
	try:
		with open(CONFIG_PATH, 'r') as f:
			to_return = json.load(f)
	except BaseException as ex:
		write_default_config(CONFIG_PATH)
	return verify_config(to_return)