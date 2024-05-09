import sys
import argparse
import os
import json
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

MODEM_CONFIGS_PATH = '/bridge/conf/si4463/all/wds_generated'
MODEM_CONFIG_SELECTED_PATH = '/bridge/conf/si4463/modem/'

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description = 'configure the bridge interface')
	parser.add_argument('-lm',   '--list-modem-configs', action='store_true', help='list available modem configs')
	parser.add_argument('-sm', '--set-modem-config', type=str, default='', help='set the modem config by name')
	parser.add_argument('-nr',   '--no-restart', action='store_false', help='do not restart the bridge service after making configuration changes')
	parser.add_argument('-mcln', '--modem-config-link-name', type=str, default='1000_modem_config.h', help='set the modem config link name')
	args = parser.parse_args().__dict__
	log.basicConfig(level=log.DEBUG)
	
	config_changed = False
	
	if (args['list_modem_configs']):
		print("Currently Selected Radio Config")
		os.system('ls -lh %s' % (MODEM_CONFIG_SELECTED_PATH,))
		print("\nAvailable:")
		os.system('ls -lh %s' % (MODEM_CONFIGS_PATH,))
	
	if (args['set_modem_config'] != ''):
		if (not os.path.isfile(os.path.join(MODEM_CONFIGS_PATH, args['set_modem_config']))):
			log.warning('\"%s\" does not exist' % (args['set_modem_config'],))
		else:
			os.system('rm %s' % (os.path.join(MODEM_CONFIG_SELECTED_PATH,'*'),))
			old_cd = os.getcwd()
			os.chdir(MODEM_CONFIG_SELECTED_PATH)
			os.system('ln -s %s %s' % (os.path.join('..','all','wds_generated',args['set_modem_config']), args['modem_config_link_name']))
			os.chdir(old_cd)
			config_changed = True

	if (config_changed) and (args['no_restart'] == False):
		os.system('systemctl restart bridge')