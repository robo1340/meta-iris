import logging as log
import argparse
import json
import os
import random

def create_random_mac(path):
	mac = "02:00:00:%02x:%02x:%02x" % (random.randint(0, 255),random.randint(0, 255),random.randint(0, 255))
	with open(path,'w') as f:
		json.dump(mac, f)
	return mac

parser = argparse.ArgumentParser()
#parser.add_argument('-c', '--config', action = 'store_true', help='enable debug log output')
parser.add_argument('-c', '--config', type=str, default='/etc/random_mac.json', help = 'the randomized mac')
parser.add_argument('-i', '--interface', type=str, default='end0', help = 'the interface')
   
if __name__ == "__main__":
	args = parser.parse_args().__dict__
	log.basicConfig(level=log.DEBUG)
	mac_path = args['config']
	mac = ''
	dev = args['interface']
	
	if (not os.path.isfile(mac_path)):
		mac = create_random_mac(mac_path)
	else:
		with open(mac_path,'r') as f:
			mac = json.load(f)
	
	os.system('ip link set dev %s down' % (dev,))
	os.system('ip link set dev %s address %s' % (dev, mac))
	os.system('ip link set dev %s up' % (dev,))
	
	log.info('MAC set to \'%s\'' % (mac,))

