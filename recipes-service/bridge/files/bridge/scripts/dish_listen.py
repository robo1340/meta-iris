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
	parser = argparse.ArgumentParser(description = 'Receive a message over the radio')
	parser.add_argument('-p', '--port', type=int, default=1337, help='UDP port to listen on')
	parser.add_argument('-g', '--group', type=str, default='default', help='the message group to listen to')
	parser.add_argument('-r',   '--record-path', type=str, default='/tmp/slip_if.json', help='path to the slip interface record file')
	args = parser.parse_args().__dict__

	#2. read in the slip interface records
	#config = load_config(args['record_path'])
	
	#2.5 check is a slip device already exists with the same IP
	#if (len(config['routes']) == 0): #no radio routes yet
	#	print('No routes yet')
	#	sys.exit(0)

	ctx = zmq.Context.instance()
	dish = ctx.socket(zmq.DISH)
	dish.rcvtimeo = 1000

	#dish.bind('udp://*:%d' % (args['port'],))
	dish.bind('udp://127.0.0.1:1337')
	#dish.join(args['group'])
	
	poller = zmq.Poller()
	poller.register(dish, zmq.POLLIN)

	while(True):
		try:
			#if dish in dict(poller.poll()):
			try:
				msg = dish.recv(copy=True)
				print('Received %s:%s' % (msg.group, msg.bytes))
			except zmq.Again:
				continue
		
		except KeyboardInterrupt:
			print('shutting down')
			break
		except BaseException as ex:
			print('ERROR %s' % (ex,))
			break

	dish.close()
	ctx.term()


	
	
	
	
