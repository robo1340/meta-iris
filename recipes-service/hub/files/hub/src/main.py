import sys
import os
import zmq
import json
import time
import logging as log
from logging.handlers import RotatingFileHandler
import traceback

from state import State
from config import read_config
from config import read_addr
from config import read_callsign
import argparse
import json

from request_handler import handle_request

wall = lambda msg : os.system("wall -n \"%s\"" % (msg,))

## @brief configure a logger and return a handle to it
## @param name name to give the logger
## @param log_path full path to the log file
## @param verbose set log level to DEBUG if True, set to INFO if false
def configure_logging(verbose=True):
	formatter = log.Formatter('%(asctime)s %(levelname)s %(message)s')
	lvl = log.DEBUG if (verbose) else log.INFO
	
	path = '/var/log/hub.log'
	handler = RotatingFileHandler(path, maxBytes=10*10000, backupCount=5)
	handler.setFormatter(formatter)
	
	log.basicConfig(handlers=(handler,), level=lvl)


def configure_logging_commandline(verbose=True):
	lvl = log.DEBUG if (verbose) else log.INFO
	log.basicConfig(level=lvl)

#create a subscriber socket and connect it to the ipc proxy
def create_subscriber(context, endpoint):
	subscriber = context.socket(zmq.SUB)
	subscriber.setsockopt(zmq.RCVHWM, 500)
	subscriber.connect(endpoint)
	return subscriber

def create_push(context, endpoint):
	socket = context.socket(zmq.PUSH)
	socket.linger = 1000
	socket.connect(endpoint)
	return socket

def create_pull(context, endpoint):
	socket = context.socket(zmq.PULL)
	socket.linger = 1000
	socket.bind(endpoint)
	return socket
	
def create_pub(context, endpoint):
	socket = context.socket(zmq.PUB)
	socket.linger = 1000
	socket.bind(endpoint)
	return socket

#create a dealer socket that will be responsible for the req/rep pattern
def create_responder_socket(context, endpoint):
	to_return = context.socket(zmq.ROUTER)
	to_return.linger = 1000
	#to_return.setsockopt_string(zmq.IDENTITY, 'hub')
	to_return.bind(endpoint)
	return to_return

if __name__ == "__main__":
	#1. import config file
	config = read_config()
	my_addr = read_addr()
	my_callsign = read_callsign()
	
	#1.1 parse command line arguments
	parser = argparse.ArgumentParser(description='execute unittest')
	parser.add_argument('-F', '--force-output-stdout' , action='store_true', help='log everything to stdout')
	parser.add_argument('-n', '--no-tx' , action='store_true', help='log everything to stdout')
	args = parser.parse_args()
	command_line_config = args.__dict__
	
	#2. configure logging
	#configure_logging_commandline(verbose=True)
	if (command_line_config['force_output_stdout'] == True): #log everything to stdout
		configure_logging_commandline(verbose=True)
	else: #create log files
		configure_logging(verbose=True)
		#configure_logging(verbose=False)
	
	#4. create global state object
	zmq_ctx = zmq.Context()
	nodejs_server = create_responder_socket(zmq_ctx, 'ipc:///tmp/hub_requests.ipc') 
	state = State(config, command_line_config, my_addr, my_callsign, nodejs_server)
	state.zmq_ctx = zmq_ctx
	#5. set up zeroMQ objects
	
	state.push = create_push(state.zmq_ctx, "ipc:///tmp/to_tx.ipc") #zmq socket to send messages to snap
	state.pub = create_pub(state.zmq_ctx, "ipc:///tmp/received_msg.ipc") #zmq socket to publish handled messages
	
	pull = create_pull(state.zmq_ctx, "ipc:///tmp/transmit_msg.ipc") #zmq socket to pull message to be made into packets
	subscriber = create_subscriber(state.zmq_ctx, "ipc:///tmp/to_rx.ipc") #zmq socket to receive messages from snap
	subscriber.setsockopt(zmq.SUBSCRIBE, b"")
	
	
	poller = zmq.Poller()
	poller.register(subscriber, zmq.POLLIN)
	poller.register(pull, zmq.POLLIN)
	poller.register(nodejs_server, zmq.POLLIN)
	
	log.info('hub entering main loop... %s' % (state,))
	while (not state.stopped):
		try:
			socks = dict(poller.poll(timeout=100))
			for sock in socks:
				if (socks[sock] != zmq.POLLIN):
					continue
				
				if (sock is pull): #message received to be turned into packet
					msg = sock.recv_json()
					if ('dst' in msg):
						state.tx_packet_adv(**msg)
					else:
						log.warning('key "dst" not included in message %s' % (msg,))
				elif (sock is subscriber): #packet received from radio
					pay = sock.recv()
					rssi = int.from_bytes(sock.recv(), byteorder='little')
					#log.info(pay)
					#log.info(rssi)
					state.handle_packet(rssi, pay)
				elif (sock is nodejs_server): #handle a request
					identity = sock.recv()
					empty = sock.recv()
					command = sock.recv_string()
					payload = sock.recv_string()
					(rsp_cmd, rsp_pay) = handle_request(command, payload, state)
					#log.debug(identity)
					#log.debug(empty)
					#log.debug(command)
					#log.debug(payload)
					sock.send(identity, flags=zmq.SNDMORE)
					sock.send(empty, flags=zmq.SNDMORE)
					sock.send_string(rsp_cmd, flags=zmq.SNDMORE)
					sock.send_string(rsp_pay)
				
			state.run() #poller timed out, do non-zmq things that need to happen at relatively low frequency
				
		except KeyboardInterrupt:
			state.stopped = True
		except zmq.ZMQError as ex:
			log.error("A zmq specific error has occurred")
			log.error(ex)
		except BaseException as ex:
			log.error("A general error has occurred")
			log.error(ex)
			log.error(type(ex).__name__)
			log.error(traceback.format_exc())
		finally:
			pass
			#time.sleep(10)
			
	#cleanup
	log.info('hub Shutting Down')
	subscriber.close()
	state.push.close()
	state.pub.close()
	pull.close()
	nodejs_server.close()
	state.zmq_ctx.term()
	