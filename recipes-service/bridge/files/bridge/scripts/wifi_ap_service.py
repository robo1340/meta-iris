



import logging as log
import argparse
import json
import sys
import os
import time

def read_json(path):
	try:
		with open(path,'r') as f:
			return json.load(f)
	except BaseException: #failed to read record so write a default in its place
		write_json(path)
		return {}


#parser = argparse.ArgumentParser()
#parser.add_argument('-v', '--verbose', action = 'store_true', help='enable debug log output')
#parser.add_argument('-p', '--passphrase', type=str, default='irisdefault', help = 'the wifi AP passphrase')
   
if __name__ == "__main__":
	#config = parser.parse_args().__dict__
	log.basicConfig(level=log.DEBUG)
	
	callsign = read_json('/bridge/conf/radio_callsign.json')
	config = read_json('/bridge/conf/config.json')
	
	passphrase = 'diagnostics'
	if ('wifi_passphrase' in config):
		passphrase = config['wifi_passphrase']
	
	os.system('python3 /bridge/scripts/create_wifi_ap.py -ip 192.168.3.1 -s %s -p %s' % (callsign, passphrase))
	
	log.info('wifi AP started %s:%s' % (callsign,passphrase))
	time.sleep(5)
	
	while True:
		status = os.system('systemctl is-active --quiet dnsmasq')
		if (status != 0):
			log.error('dnsmasq lost, restarting')
			break
		status = os.system('pidof hostapd > /dev/null 2>&1')
		if (status != 0):
			log.error('hostapd lost, restarting')
			break
		time.sleep(3)
	
