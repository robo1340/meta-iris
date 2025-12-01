import zmq
import sys
import time
import argparse

def safe_read_file(path):
	try:
		with open(path, 'r') as f:
			return f.read()
	except BaseException as ex:
		log.warning('error while reading at path \'%s\'' % (path,))
		log.warning(ex)
		return None

def create_push(context, endpoint):
	socket = context.socket(zmq.PUSH)
	socket.linger = 1000
	socket.connect(endpoint)
	return socket

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description = '')
	parser.add_argument('-p',   '--payload', type=str, default='', help='the ZMQ payload, will take file redirects if not specified')
	parser.add_argument('-pf',   '--payload-path', type=str, default='', help='path to a file containing the payload, overrides --payload')
	parser.add_argument('-e',   '--endpoint', type=str, default="ipc:///tmp/to_tx.ipc", help='the tcp socket used for request/response communication')
	args = parser.parse_args()
	config = args.__dict__
	
	if (config['payload_path'] == ''):
		if (config['payload'] == '') and (not sys.stdin.isatty()):
			config['payload'] = open(0).read()
	else: #load payload from file
		config['payload'] = safe_read_file(config['payload_path'])
		if (config['payload'] is None):
			sys.exit(3)
		
	context = zmq.Context()
	socket = create_push(context, config['endpoint'])
	
	try:
		print(config['payload'])
		socket.send(config['payload'].encode())
	except BaseException as ex:
		print(ex)
	finally:
		socket.close()
		context.term()
	

