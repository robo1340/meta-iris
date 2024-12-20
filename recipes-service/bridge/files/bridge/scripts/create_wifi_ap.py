import logging as log
import argparse
import json
import sys
import os

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

'''
#class for an interface that is an internet interface serving IP Addresses      
class EthnernetServerDHCP(NetworkInterfaceType):
	def __init__(self, interface):
		self.interface = interface #the NetworkInterface object
		self.ip = None
		self.dst_subnet = None

	def get_link_info(self):
		return None

	def load_type_specific_attributes(self, type_specific_attributes):
		self.ip         = get_key_from_attributes(type_specific_attributes, 'ip', default=None, device_name=self.interface.device, check_func=ip_check)
		ips = self.ip.split('.')
		self.dst_subnet = get_key_from_attributes(type_specific_attributes, 'dst_subnet', default='%s.%s.%s.0/24' % (ips[0],ips[1],ips[2]), device_name=self.interface.device)
		
		ip.set_interface_up(self.interface.device)
		os.system('ip addr flush dev %s' % (self.interface.device,))
		os.system('ip addr add %s/24 dev %s > /dev/null 2>&1' % (self.ip, self.interface.device))
		os.system('ip route add %s via %s dev %s proto static > /dev/null 2>&1' % (self.dst_subnet, self.ip, self.interface.device))  
		
		dnsmasq = safe_lookup(type_specific_attributes, 'dnsmasq', default={}) #get the dnsmasq settings
		restart_dnsmasq = safe_lookup(dnsmasq, 'restart_dnsmasq', default=True) #if this options is set, restart dnsmasq after writing the conf file
		dnsmasq_conf = safe_lookup(dnsmasq, 'dnsmasq_conf', default=[])
		if (dnsmasq_conf == []):
			dnsmasq_conf.append("interface=%s" % (self.interface.device,))
			dnsmasq_conf.append("dhcp-range=%s.%s.%s.2,%s.%s.%s.30,255.255.255.0,12h" % (ips[0],ips[1],ips[2],ips[0],ips[1],ips[2]))
			dnsmasq_conf.append("dhcp-option=3,%s" % (self.ip,))
			dnsmasq_conf.append("dhcp-option=6,%s" % (self.ip,))
			dnsmasq_conf.append("server=8.8.8.8")
			dnsmasq_conf.append("log-queries")
			dnsmasq_conf.append("log-dhcp")
			dnsmasq_conf.append("listen-address=127.0.0.1")
		util.create_dnsmasq_conf(dnsmasq_conf)
		if (restart_dnsmasq == True):
			util.restart_dnsmasq()
		
	def set_link_up_attempt_start(self):
		pass 
	def set_link_up_attempt_fail(self):
		pass
	def check_link_meta(self):
		return True
	def connect_attempt_start(self):
		return self.interface.connect_success_callback()       
'''

##@brief attempt to create a hostapd conf file based on attributes in the network config file
##@param attributes the type_specific_attributes['hostapd']['hostapd_conf']
##  of a wifi_ap entry in the network config file
##@param path, where the created hostapd conf file will be stored
##@throws throws a ValueError if a hostapd conf file cannot be created from attributes
def create_hostapd_conf(attributes, path='/tmp/hostapd.conf'):
	lines = [] #array of lines to write to the file
	HOSTAPD_ALLOWED_OPTIONS = ["interface","hw_mode","channel","ieee80211d","country_code","ieee80211n","wmm_enabled","ssid","auth_algs","wpa","wpa_key_mgmt","rsn_pairwise","wpa_passphrase"]
	for key, val in attributes.items():
		if key in HOSTAPD_ALLOWED_OPTIONS:
			lines.append('%s=%s\n' % (key,str(val)))
		else:
			log.warning('option \"%s\" with value \"%s\" is not allowed in wpa_supplicant conf' % (key,str(val)))
	lines.append('\n')
	with open(path, 'w') as conf_file: #open the conf file to be written
		conf_file.writelines(lines)

##@brief attempt to create a dnsmasq conf file from an array of strings
##@param lines an array of strings that will be written to dnsmadq.conf
##@path where the created dnsmasq conf file will be stored
##@throws throws a ValueError if lines is not an array of strings
def create_dnsmasq_conf(lines, path='/tmp/dnsmasq.conf'):
	if (isinstance(lines,list) == False):
		raise ValueError('lines argument passed to create_dnsmasq_conf is not a list! \'%s\'' % (lines,))
	
	current_lines = [] #the lines currently in dnsmasq.conf
	
	#try:
	#	with open(path, 'r') as f:
	#		current_lines = f.readlines()
	#except BaseException: #this will happen if dnsmasq does not yet exist
	#	pass
	#current_lines = [l.rstrip('\r\n') for l in current_lines] #strip newline character away
	#
	#for new_line in lines: #iterate over the lines to be added to make sure they aren't already in the conf file
	#	if (new_line not in current_lines):
	#		current_lines.append(new_line)
	
	with open(path, 'w') as conf_file:
		for line in lines:
			if (line != ''):
				conf_file.write(line + '\n')

def verify_interface(desired):
	selected_if = None
	interfaces = set(os.listdir('/sys/class/net'))
	if (desired in interfaces):
		return desired
	else:
		log.warning('specified wifi interface %s not found' % (desired,))
		for i in interfaces:
			if (i.startswith('wl')):
				log.warning('using %s instead' % (i,))
				selected_if = i
	
	if (selected_if is None):
		log.error('No wifi interface found')
		sys.exit(1)
	

parser = argparse.ArgumentParser()
parser.add_argument('-v', '--verbose', action = 'store_true', help='enable debug log output')
parser.add_argument('-k', '--kill', action = 'store_true', help='just kill hostapd and exit')
parser.add_argument('-i', '--interface', type=str, default='wlu1', help = 'the wifi interface to bring up')
parser.add_argument('-ip', '--ip', type=str, default='192.168.3.1', help = 'ip address to assign to the interface')
parser.add_argument('-s', '--ssid', type=str, default='irisdefault', help = 'the wifi AP SSID')
parser.add_argument('-p', '--passphrase', type=str, default='irisdefault', help = 'the wifi AP passphrase')
parser.add_argument('-ch', '--channel', type=int, default=6, help = 'the wifi AP channel')
   
if __name__ == "__main__":
	config = parser.parse_args().__dict__
	log.basicConfig(level=lvl==log.DEBUG if config['verbose'] else log.INFO)
	os.system('killall hostapd > /dev/null 2>&1')
	os.system('killall wpa_supplicant > /dev/null 2>&1')
	os.system('ip addr flush dev %s' % (config['interface'],))
	if (config['kill'] == True):
		log.info('Hostapd killed')
		sys.exit(0)
	
	selected_if = verify_interface(config['interface'])
	
	ips = config['ip'].split('.')
	subnet = '%s.%s.%s.0/24' % (ips[0],ips[1],ips[2])
	conf_path = '/tmp/hostapd.conf' #path to the hostapd.conf file
	
	hostapd_conf = {
		'interface' 	: selected_if,
		'channel' 		: config['channel'],
		'ssid' 			: config['ssid'],
		'wpa_passphrase': config['passphrase'],
		'hw_mode'		: 'g',
		'ieee80211d'	: 0,
		'wmm_enabled'	: 0,
		'auth_algs'		: 1,
		'wpa'			: 2,
		'wpa_key_mgmt'	: 'WPA-PSK',
		'rsn_pairwise'	: 'CCMP'
	}
	create_hostapd_conf(attributes=hostapd_conf, path=conf_path)
	
	dnsmasq_conf = []
	dnsmasq_conf.append("interface=%s" % (config['interface'],))
	#dnsmasq_conf.append("interface=%s" % (config['interface'],))
	dnsmasq_conf.append("dhcp-range=%s.%s.%s.5,%s.%s.%s.30,255.255.255.0,12h" % (ips[0],ips[1],ips[2],ips[0],ips[1],ips[2]))
	dnsmasq_conf.append("dhcp-option=3,%s" % (config['ip'],))
	dnsmasq_conf.append("dhcp-option=6,%s" % (config['ip'],))
	dnsmasq_conf.append("server=8.8.8.8")
	dnsmasq_conf.append("log-queries")
	dnsmasq_conf.append("log-dhcp")
	dnsmasq_conf.append("listen-address=127.0.0.1")
	create_dnsmasq_conf(dnsmasq_conf)
	os.system('systemctl restart dnsmasq')
	
	os.system('hostapd -B %s' % (conf_path,))
	
	#configure the bridge and add end1 and wlu1 to the bridge
	#os.system('ip addr flush dev %s' % (config['interface'],)
	#os.system('ip addr add dev %s %s.%s.%s.%d/24' % (config['interface'],ips[0],ips[1],ips[2],int(ips[3])+1))
	#os.system('ip link set dev %s up' % (config['interface'],))
	
	os.system('ip addr flush dev %s > /dev/null 2>&1' % (config['interface'],))
	os.system('ip addr add %s/24 dev %s > /dev/null 2>&1' % (config['ip'], config['interface']))
	os.system('ip route add %s via %s dev %s proto static > /dev/null 2>&1' % (subnet, config['ip'], config['interface']))
	
	log.info('AP %s on %s brought up with passphrase %s' % (config['ssid'],config['interface'],config['passphrase']))

