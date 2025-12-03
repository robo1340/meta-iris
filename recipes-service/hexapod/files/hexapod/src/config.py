import json
import os
import random
import time

CONFIG_PATH = '/key_pub/conf/config.json'

hardcoded_default_config = {
	'serial_path' 		: '/dev/ttySTM1',
	'serial_baud'		: 115200,
	'serial_timeout'	: 1,
	'LH' 	: 1000,
	'LM' 	: 1400,
	'LL'	: 1800,
	'RH' 	: 2000,
	'RM' 	: 1600,
	'RL'	: 1200,
	"offsets" : {
		2 : 80,
		8 : 80
	},
	'base_speed' : 150,
	'max_speed' : 200,
	'advertisment_rate' : 15
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