import logging as log
import json
from json import JSONDecodeError
import os
from packet import type_to_string
import traceback
from config import CALLSIGN_PATH
import util
from pathlib import Path
from util import safe_write_json
from util import safe_read_json
import time

SNAP_INI_PATH = '/snap/conf/config.ini'
ACK  = 'ACK'
NACK = 'NACK'

def read_ini(path):
	to_return = {}
	with open(path, 'r') as f:
		for line in f.readlines():
			key,val = line.rstrip('\n').split('=',maxsplit=2)
			to_return[key] = val
	return to_return
	
def write_ini(path,contents={}):
	with open(path,'w') as f:
		for key,val in contents.items():
			f.write('%s=%s\n' % (key,val))
	return True

#####################################################################################
################################ UTILITY FUNCTIONS ##################################
#####################################################################################

def exception_suppressor(func):
	def meta_func(*args, **kwargs):
		try:
			to_return = func(*args,**kwargs)
		except BaseException as ex:
			log.warning("Exception caught in %s" % (func.__name__,))
			log.warning(traceback.format_exc())
			return (NACK, "Exception occurred while handling request in %s()" % (func.__name__))
		else:
			return to_return
	return meta_func

#get_id  = lambda req: req[0]
#get_ver = lambda req: req[2]
#get_cmd = lambda req: req[3]
get_pay  = lambda req: req[3]

##@brief check the payload to make sure it is json formatted with the expected key-value pairs
##@param payload the payload string
##@return returns a dict object of key-value pairs in the payload if valid, returns None otherwise
def validate_payload(payload, expected_keys=[]):
	try: #make sure the payload sent is proper json
		payload_dict = json.loads(payload) #this will throw an exception if payload isn't json
		for key in expected_keys:
			if (key not in payload_dict):
				return (None, 'expected key: \"%s\" not found in payload' % (key,))
		return (payload_dict, None)
	except JSONDecodeError:
		return (None,'failed to parse json payload')        

#####################################################################################
################################ HANDLER FUNCTIONS ##################################
#####################################################################################



@exception_suppressor
def handle_get_my_callsign(cmd, pay, state):
	return(cmd, json.dumps({'callsign':state.my_callsign, 'addr':state.my_addr}))

@exception_suppressor
def handle_set_my_callsign(cmd, pay, state):
	(pay_dict, error_msg) = validate_payload(pay, expected_keys=['callsign'])
	if (error_msg is not None):
		return (NACK, error_msg)
	state.my_callsign = pay_dict['callsign']
	util.safe_write_json(CALLSIGN_PATH, state.my_callsign)
	state.tx_peer_info()
	return (cmd, json.dumps(pay_dict))

@exception_suppressor
def handle_get_peers(cmd, pay, state):
	to_return = {}
	for peer in state.peers.values():
		to_return[peer.addr] = peer.as_dict()
	return(cmd, json.dumps(to_return))

@exception_suppressor
def handle_get_link_peers(cmd, pay, state):
	to_return = {}
	for peer in state.link_peers.values():
		to_return[peer.addr] = peer.as_dict()
	return(cmd, json.dumps(to_return))

@exception_suppressor
def handle_get_radio_config(cmd, pay, state):	
	selected = {
		'general' 	: None,
		'packet'	: None,
		'preamble'	: None
	}
	
	SELECTED_OTHER_DIR = '/snap/conf/si4463/other/'
	for file in os.listdir(SELECTED_OTHER_DIR):
		path = os.path.join(SELECTED_OTHER_DIR, file)
		if os.path.islink(path):
			for key in selected.keys():
				if key in path:
					selected[key] = str(Path(path).resolve())
					#selected[key] = os.path.abspath(os.readlink(path))
	
	SELECTED_DIR = '/snap/conf/si4463/modem/'
	for file in os.listdir(SELECTED_DIR):
		path = os.path.join(SELECTED_DIR, file)
		if os.path.islink(path):
			selected['modem'] = str(Path(path).resolve())
			#selected['modem'] = os.path.abspath(os.readlink(path))
			break
	
	assert 'modem' in selected, 'could not find symlink in "/snap/conf/si4463/modem"'
	
	def list_dir_full_path(path):
		to_return = []
		for file in os.listdir(path):
			to_return.append(os.path.join(path,file))
		return to_return
	
	to_return = {
		'available':{
			'modem' 	: list_dir_full_path('/snap/conf/si4463/all/wds_generated'),
			'general' 	: list_dir_full_path('/snap/conf/si4463/all/general'),
			'packet' 	: list_dir_full_path('/snap/conf/si4463/all/packet'),
			'preamble' 	: list_dir_full_path('/snap/conf/si4463/all/preamble')
		},
		'selected'  : selected,
		'snap_args' : read_ini(SNAP_INI_PATH) 
	}
	#log.debug(json.dumps(to_return, indent=2))
	return(cmd, json.dumps(to_return))

@exception_suppressor
def handle_get_hub_config(cmd, pay, state):
	return(cmd, json.dumps(state.config))

@exception_suppressor
def handle_set_hub_config(cmd, pay,state):
	(pay_dict, error_msg) = validate_payload(pay)
	if (error_msg is not None):
		return (NACK, error_msg)	
	for key,val in pay_dict.items():
		state.config[key] = val
	safe_write_json('/hub/conf/config.json',state.config)
	state.stopped = True
	return(cmd, json.dumps(pay_dict))
	

@exception_suppressor
def handle_set_radio_config(cmd, pay, state):
	(pay_dict, error_msg) = validate_payload(pay, expected_keys=['radio','snap_args'])
	if (error_msg is not None):
		return (NACK, error_msg)
	for typ, path in pay_dict['radio'].items():
		if (typ in ['modem','general','packet','preamble']):
			os.system('/usr/bin/python3 /snap/scripts/snap_configure.py -t %s -sm %s --no-restart' % (typ, path))
	write_ini(SNAP_INI_PATH, pay_dict['snap_args'])
	state.restart_snap = time.monotonic()
	return handle_get_radio_config(cmd, '', state)

@exception_suppressor
def handle_get_os_version(cmd, pay, state):
	version = safe_read_json('/home/version.json', silent=True, default='-1.-1.-1')
	return (cmd, json.dumps(version))

COMMANDS = {
	"GET_MY_CALLSIGN"   : handle_get_my_callsign,
	"SET_MY_CALLSIGN"	: handle_set_my_callsign,
	"GET_MY_ADDR"       : handle_get_my_callsign,
	"GET_PEERS"         : handle_get_peers,
	"GET_LINK_PEERS"    : handle_get_link_peers,
	"GET_RADIO_CONFIG" 	: handle_get_radio_config,
	"SET_RADIO_CONFIG"	: handle_set_radio_config,
	"GET_HUB_CONFIG"	: handle_get_hub_config,
	"SET_HUB_CONFIG"	: handle_set_hub_config,
	"GET_OS_VERSION"	: handle_get_os_version
}

def handle_request(cmd, pay, state):

	log.debug('handling request with command:%s, payload:%s' % (cmd,pay))
	
	if (cmd in COMMANDS):
		handler = COMMANDS[cmd]
		(rsp_cmd, rsp_payload) = handler(cmd, pay, state)
		if (rsp_cmd == NACK):
			return (rsp_cmd, rsp_payload)
		else:
			return (rsp_cmd, rsp_payload)
	elif (cmd == 'COMMANDS'):
		return(cmd, json.dumps(list(COMMANDS.keys())))
	elif (cmd == 'TLV_TYPES'):
		return(cmd, json.dumps(list(type_to_string.values())))
	else:
		return (NACK, 'command not recognized')
