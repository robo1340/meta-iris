import sys
import argparse
import os
import json
import time
import logging as log

def write_json(path, to_write={}, indent=2):
	try:
		with open(path,'w') as f:
			json.dump(to_write, f, indent=indent)
	except BaseException as ex:
		print('ERROR: could not write to \'%s\', %s' % (path,ex), file=sys.stderr)
		sys.exit(1)

def read_json(path):
	try:
		with open(path,'r') as f:
			return json.load(f)
	except BaseException: #failed to read record so write a default in its place
		write_json(path)
		return {}

## @brief safely lookup a key in a dictionary
## if the key does not exist, return a default value
## @param d the dictionary to lookup a value from
## @param key the key to lookup
## @param default the default value to return if key does not exist in d
## @param set_default the key can't be found and set_default is true, set the value of d[key] to default
def safe_lookup(d, key, default={}, set_default=True):
	if (type(d) is not dict):
		return default
	if not key in d:
		if (set_default is True):
			d[key] = default
		return default
	else:
		return d[key]

def is_ip_valid(s):
	a = s.split('.')
	if len(a) != 4:
		return False
	for x in a:
		if not x.isdigit():
			return False
		i = int(x)
		if i<0 or i>255:
			return False
	return True

def change_ip(slip_if, new_ip):
	os.system('ip link set dev %s down' % (slip_if,))
	os.system('ip addr flush dev %s' % (slip_if,))
	os.system('ip addr add %s dev %s' % (new_ip, slip_if))
	os.system('ip link set dev %s up' % (slip_if,))

MODEM_CONFIGS_PATHS = {
	'modem'   : '/snap/conf/si4463/all/wds_generated',
	'general' : '/snap/conf/si4463/all/general',
	'packet'  : '/snap/conf/si4463/all/packet',
	'preamble': '/snap/conf/si4463/all/preamble'
}

MODEM_CONFIG_SELECTED_PATHS = {
	'modem'   : '/snap/conf/si4463/modem/',
	'general' : '/snap/conf/si4463/other',
	'packet'  : '/snap/conf/si4463/other',
	'preamble': '/snap/conf/si4463/other'
}

CONFIG_LINK_NAME_KEY = {
	'modem'   : 'modem_config_link_name',
	'general' : 'general_config_link_name',
	'packet'  : 'packet_config_link_name',
	'preamble': 'preamble_config_link_name'	
}

QUICK_CONFIG_OPTIONS = {
	'' : {},
	'default' : {
		'set_config' : 'radio_config_Si4463_430M_10kbps.h',
		'payload'	 : 100,
		'block'		 : 20,
		'disable_reed_solomon' : False,
		'disable_convolutional' : False
	},
	'expanded' : {
		'set_config' : 'radio_config_Si4463_430M_10kbps.h',
		'payload'	 : 200,
		'block'		 : 20,
		'disable_reed_solomon' : False,
		'disable_convolutional' : False
	},
	'rc' : {
		'set_config' : 'radio_config_Si4463_430M_20kbps.h',
		'payload'	 : 22,
		'block'		 : 22,
		'disable_reed_solomon' : False,
		'disable_convolutional' : False
	},
	'rc_no_conv' : {
		'set_config' : 'radio_config_Si4463_430M_20kbps.h',
		'payload'	 : 22,
		'block'		 : 22,
		'disable_reed_solomon' : False,
		'disable_convolutional' : True	
	},
	'rc_no_reed' : {
		'set_config' : 'radio_config_Si4463_430M_20kbps.h',
		'payload'	 : 22,
		'block'		 : 22,
		'disable_reed_solomon' : True,
		'disable_convolutional' : False	
	},
	'rc_no_ecc' : {
		'set_config' : 'radio_config_Si4463_430M_20kbps.h',
		'payload'	 : 22,
		'block'		 : 22,
		'disable_reed_solomon' : True,
		'disable_convolutional' : True	
	},
	'default_no_ecc' : {
		'set_config' : 'radio_config_Si4463_430M_10kbps.h',
		'payload'	 : 100,
		'block'		 : 20,
		'disable_reed_solomon' : True,
		'disable_convolutional' : True
	}
	
}

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description = 'configure the snap interface')
	parser.add_argument('-q',   '--quick-config', choices=list(QUICK_CONFIG_OPTIONS.keys()), default='', help='configure modem setting to one of several pre-canned options')
	parser.add_argument('-t',   '--config-type', choices=['modem','general','packet','preamble'], default='modem', help='the config type being set')
	parser.add_argument('-l',  '--list-configs', action='store_true', help='list available modem configs')
	parser.add_argument('-lc',  '--list-current-config', action='store_true', help='list current modem configuration')
	parser.add_argument('-sm',  '--set-config', type=str, default='', help='set the modem config by name')
	parser.add_argument('-s',   '--set-config-by-match', type=str, default='', help='set the modem config matching comma delimitted strings')
	parser.add_argument('-nr',  '--no-restart', action='store_true', help='do not restart the snap service after making configuration changes')
	parser.add_argument('-mcln','--modem-config-link-name', type=str, default='1000_modem_config.h', help='set the modem config link name')
	parser.add_argument('-gcln','--general-config-link-name', type=str, default='9000_general.h', help='set the general config link name')
	parser.add_argument('-pcln','--packet-config-link-name', type=str, default='9000_packet.h', help='set the packet config link name')
	parser.add_argument('-ppln','--preamble-config-link-name', type=str, default='9000_preamble.h', help='set the preamble config link name')
	args = parser.parse_args().__dict__
	log.basicConfig(level=log.DEBUG)
	
	config_changed = False
	
	MODEM_CONFIGS_PATH = MODEM_CONFIGS_PATHS[args['config_type']]
	MODEM_CONFIG_SELECTED_PATH = MODEM_CONFIG_SELECTED_PATHS[args['config_type']]
	link_name_key = CONFIG_LINK_NAME_KEY[args['config_type']]
	
	if (args['list_configs']):
		print("Currently Selected Radio Config")
		os.system('ls -lh %s' % (MODEM_CONFIG_SELECTED_PATH,))
		os.system('cat /snap/conf/config.ini')
		print("\nAvailable:")
		os.system('ls -lh %s' % (MODEM_CONFIGS_PATH,))
	elif (args['list_current_config']):
		print("Currently Selected Radio Config")
		os.system('ls -lh %s' % (MODEM_CONFIG_SELECTED_PATH,))
		os.system('cat /snap/conf/config.ini')
	
	if (args['set_config'] != ''):
		if (not os.path.isfile(os.path.join(MODEM_CONFIGS_PATH, args['set_config']))):
			log.warning('\"%s\" does not exist' % (args['set_config'],))
		else:
			os.system('rm %s' % (os.path.join(MODEM_CONFIG_SELECTED_PATH,args[link_name_key] if (args['config_type'] != 'modem') else '*'),))
			old_cd = os.getcwd()
			os.chdir(MODEM_CONFIG_SELECTED_PATH)
			os.system('ln -s %s %s' % (os.path.join('..','all',os.path.basename(MODEM_CONFIGS_PATHS[args['config_type']]),args['set_config']), args[link_name_key]))
			os.chdir(old_cd)
			config_changed = True
	elif (args['set_config_by_match'] != ''):
		to_match = args['set_config_by_match'].split(',')
		log.info('Match Patterns %s' % (to_match,))
		for file in os.listdir(MODEM_CONFIGS_PATH):
			match_found = True
			for tm in to_match:
				if (tm not in file):
					match_found = False
			if (not match_found):
				continue
			log.info('Match Found %s' % (file,))
			os.system('rm %s' % (os.path.join(MODEM_CONFIG_SELECTED_PATH,args[link_name_key] if (args['config_type'] != 'modem') else '*'),))
			old_cd = os.getcwd()
			os.chdir(MODEM_CONFIG_SELECTED_PATH)
			os.system('ln -s %s %s' % (os.path.join('..','all',os.path.basename(MODEM_CONFIGS_PATHS[args['config_type']]),file), args[link_name_key]))
			os.chdir(old_cd)
			config_changed = True
			break
	elif (args['quick_config'] != ''): 
			q = QUICK_CONFIG_OPTIONS[args['quick_config']]
			log.info('setting quick config\n%s' %(q,))
			args['set_config'] = q['set_config']

			os.system('rm %s' % (os.path.join(MODEM_CONFIG_SELECTED_PATH,args[link_name_key] if (args['config_type'] != 'modem') else '*'),))
			old_cd = os.getcwd()
			os.chdir(MODEM_CONFIG_SELECTED_PATH)
			os.system('ln -s %s %s' % (os.path.join('..','all',os.path.basename(MODEM_CONFIGS_PATHS[args['config_type']]),args['set_config']), args[link_name_key]))
			os.chdir(old_cd)
			
			os.system('rm /snap/conf/config.ini')
			with open('/snap/conf/config.ini', 'w') as f:
				f.write('payload=%s\n' % (q['payload'],))
				f.write('block=%s\n' % (q['block'],))
				f.write('disable_reed_solomon=%s\n' % ('true' if q['disable_reed_solomon'] else 'false',))
				f.write('disable_convolutional=%s\n' % ('true' if q['disable_convolutional'] else 'false',))
			
			config_changed = True		
		

	if (config_changed) and (args['no_restart'] == False):
		log.info('Restarting snap')
		os.system('systemctl restart snap')
		#time.sleep(1)
		#log.info('Restarting hub')
		#os.system('systemctl restart hub')