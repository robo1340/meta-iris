import sys
import argparse
import os
import json
import time
import subprocess

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


'''
{
	"serial_devices" : {
		0 : 214 #the serial port id, and the PID supporting it
	},
	"interface" : {
		"name" : "sl0",
		"link_up" : true,
		"ip" : "10.0.0.1",
		"pid" : 1234,
		"dst" : "10.0.1.1"
	},
	"routes" : [
		"10.0.0.0",
		"10.0.1.0",
		"10.0.2.0"
	]
}
'''
def load_config(path):
	config = read_json(path)
	if ('serial_devices' not in config):
		config['serial_devices'] = {}
	if ('interface' not in config):
		config['interface'] = {}
	if ('routes' not in config):
		config['routes'] = []
	return config

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

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description = 'Create a tun interface or modify routes to it')
	parser.add_argument('-ip', '--host-ip', type=str, default='', help='the IP address to assign to the slip interface')
	parser.add_argument('-d', '--dst-subnet', type=str, default='', help='add destination class C subnet to add to the slip interface ex. 10.0.0.0')
	parser.add_argument('-k',   '--kill', action='store_true', help='kill all slip processes and destroy all slip interfaces')
	parser.add_argument('-m',   '--mtu', type=int, default=600, help='the MTU size of the slip interface')
	parser.add_argument('-r',   '--record-path', type=str, default='/tmp/slip_if.json', help='path to the slip interface record file')
	args = parser.parse_args().__dict__
	
	iface = 'tun0'
	
	if (args['kill']==True):
		os.system('ip link delete %s' % (iface,))
		
	if (args['host_ip'] != ''): #try to create a new slip interface
		#1. validate the ip addresses
		if (not is_ip_valid(args['host_ip'])):
			print('ERROR: IP %s is not valid' % (args['host_ip'],), file=sys.stderr)
			sys.exit(1)
		
		#create the interface
		os.system('ip tuntap add mode tun')
		time.sleep(0.1)
		#bring up the interface
		os.system('ip link set multicast off dev %s' % (iface,))
		os.system('ip addr add %s/24 dev %s' % (args['host_ip'], iface))
		#os.system('ip addr add %s dev %s' % (args['host_ip'], iface))
		#os.system('ip link set dev %s mtu %d' % (iface, args['mtu']))
		os.system('ip link set dev %s up' % (iface,))
		os.system('ip route add 10.0.0.0/8 dev %s' % (iface,)) #add a general route
		#os.system('ip -6 addr add fe80::9823:59ff:fef3:de91/64 dev tun0 nodad')
		
		#add a local route on the loopback interface, this is needed to receive broadcast UDP packets
		#octets = args['host_ip'].split('.')
		#lo_ip 	  = '%s.%s.%s.1/24' % (octets[0], octets[1], octets[2])
		#lo_subnet = '%s.%s.%s.0/24' % (octets[0], octets[1], octets[2])
		#os.system('ip addr add %s dev lo' % (lo_ip,))
		#os.system('ip route add %s dev lo' % (lo_subnet,))
		
		#print(json.dumps(config, indent=2))
		print(json.dumps({'interface':iface}))	

	if (args['dst_subnet'] != ''):
		#1. validate the ip addresses
		if (not is_ip_valid(args['dst_subnet'])):
			print('ERROR: subnet %s is not valid' % (args['dst_subnet'],), file=sys.stderr)
			sys.exit(1)
		
		else:
			octets = args['dst_subnet'].split('.')
			dst_subnet = '%s.%s.%s.0/24' % (octets[0], octets[1], octets[2])
			#os.system('ip route add %s dev %s proto kernel scope link src %s' % (dst_subnet, config['interface']['name'], config['interface']['ip']))
			os.system('ip route add %s dev %s' % (dst_subnet, iface))
	
	
