import argparse
import zmq
import time
import traceback
import json
import struct
from datetime import datetime
	
#create a subscriber socket and connect it to the ipc proxy
def create_subscriber(context, endpoint):
	subscriber = context.socket(zmq.SUB)
	subscriber.setsockopt(zmq.RCVHWM, 500)
	subscriber.connect(endpoint)
	return subscriber

parser = argparse.ArgumentParser(description = '')
#parser.add_argument('-e', '--envelopes', type=str, required=True, help='the base envelope to subscribe to or a comma delimited list of envelope')
#parser.add_argument('-m', '--additional-envelopes', type=str, default='', help='another comma delimitted list of envelopes to subscribe to, these will not be printed if -x option is used')
#parser.add_argument('-x', '--exact-envelope',   action='store_true',  help='if set, only decode payload from the exact envelope subscribed to, exclude payloads from sub envelopes')
#parser.add_argument('-j', '--json',   action='store_true',  help='attempt to decode the payload as a json object')
#parser.add_argument('-i', '--json-indent', type=int, default=2, help='indent to use when decoding json payload')
parser.add_argument('-st', '--stop-count', type=int, default=-1, help='exit after receiving this many messages')
parser.add_argument('-s', '--string',   action='store_true',  help='interpret received messages as strings')
parser.add_argument('-n', '--no-time',   action='store_true',  help='do not print date and time')
parser.add_argument('-a', '--endpoint', type=str, default="ipc:///tmp/to_rx.ipc", help='the endpoint to connect to')
parser.add_argument('-d', '--struct-descriptor-string', type=str, default='', help='a format string used to decode the packed struct payload, for example \'<IB\'')


if __name__ == "__main__":
	args = parser.parse_args()
	config = args.__dict__

	context = zmq.Context()
	sub = create_subscriber(context, config['endpoint']) 
	sub.setsockopt_string(zmq.SUBSCRIBE, "")
	sub.setsockopt(zmq.SUBSCRIBE, b"")
	
	count = 0

	while(True):
		if (config['stop_count'] > 0) and (count >= config['stop_count']):
			break
		try:
			if (sub.poll(timeout=1000, flags=zmq.POLLIN) == zmq.POLLIN):
				pay = sub.recv()
				
				if (config['no_time'] == False):
					print(datetime.now())
				
				if (config['string']):
					print(pay.decode('utf-8').strip('\x00'))
				else:
					print(pay)
				
		except KeyboardInterrupt:
			break
		except BaseException:
			print(traceback.format_exc())
			#pass 
			
			
	sub.close()
	context.term()
	print("%d messages received" % (count,))
	
