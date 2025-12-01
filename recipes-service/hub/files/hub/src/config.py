import json
import os
import random
import time

CONFIG_PATH = '/hub/conf/config.json'
ADDR_PATH = '/hub/conf/addr.cfg'
CALLSIGN_PATH = '/hub/conf/callsign.cfg'

ANIMALS = ['otter','dog','cat','horse','sheep','aligator','kowala','hippo','coyote','horse']
ATTRIBUTES = ['shy','angry','sad','rapey','pensive','bored','excited','nervous','happy']

hardcoded_default_config = {
	'max_length' : 100,
	'retransmit_wait_multiplier_s' : 0.1, #wait this long times the rssi of the received packet
	'broadcast_addr' : 0,
	'peer_timeout_s' : 300,
	'peer_info_tx_s' : 30,
	'default_hops'	: 3,
	'topic_rate_limit_s' : 1,
	'peers_to_ignore' : [] #a list of peers to ignore messages from to better simulate a mesh network, should be left empty during normal operation
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
	
def read_addr():
	try:
		with open(ADDR_PATH, 'r') as f:
			return int(f.read(),base=16)
	except BaseException as ex:
		with open(ADDR_PATH, 'w') as f:
			new_addr = random.randint(1,65536)
			print('WARNING: my addr could not be read so 0x%04X was automatically generated' % (new_addr,))
			f.write('0x%04X' % (new_addr,))
			return new_addr
			
def read_callsign():
	try:
		with open(CALLSIGN_PATH, 'r') as f:
			return json.load(f)
	except BaseException as ex:
		with open(CALLSIGN_PATH, 'w') as f:
			random.seed(int(time.monotonic()*1000)%1000)
			new_callsign = '%s_%s' % (random.choice(ATTRIBUTES), random.choice(ANIMALS))
			print('WARNING my callsign could not be read so "%s" was automatically generated' % (new_callsign,))
			json.dump(new_callsign, f)
			return new_callsign
