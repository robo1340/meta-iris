import zmq
import sys
import argparse
import json
import time

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

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description = 'Send a message over the radio')
	parser.add_argument('-p', '--port', type=int, default=1337, help='UDP port to send on')
	parser.add_argument('-g', '--group', type=str, default='default', help='the message group to send to')
	parser.add_argument('-m',   '--msg', type=str, default='default', help='the message to send')
	parser.add_argument('-r',   '--record-path', type=str, default='/tmp/slip_if.json', help='path to the slip interface record file')
	args = parser.parse_args().__dict__

	#2. read in the slip interface records
	config = load_config(args['record_path'])
	
	#2.5 check is a slip device already exists with the same IP
	if (len(config['routes']) == 0): #no radio routes yet
		print('No routes yet')
		sys.exit(0)

	ctx = zmq.Context.instance()
	radio = ctx.socket(zmq.RADIO)
	radio.set(zmq.LINGER, 1000)
	#radio.connect('udp://%s:%d' % (config['routes'][0],args['port']))
	radio.connect('udp://127.0.0.1:1337')
	radio.send(args['msg'].encode())  #, group=args['group'])
	radio.close()
	ctx.term()	

	
	
	
	
