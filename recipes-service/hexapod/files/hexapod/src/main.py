import sys
import os
import zmq
import json
import time
import logging as log
from logging.handlers import RotatingFileHandler
import traceback
import functools
from state import State
from state import SSC32
from state import Hexapod
from config import read_config
import argparse

## @brief configure a logger and return a handle to it
## @param name name to give the logger
## @param log_path full path to the log file
## @param verbose set log level to DEBUG if True, set to INFO if false
def configure_logging(verbose=True):
	formatter = log.Formatter('%(asctime)s %(levelname)s %(message)s')
	lvl = log.DEBUG if (verbose) else log.INFO
	
	path = '/var/log/hexapod.log'
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
	subscriber.setsockopt(zmq.SUBSCRIBE, b"")
	subscriber.connect(endpoint)
	return subscriber

def create_push(context, endpoint):
	socket = context.socket(zmq.PUSH)
	socket.linger = 1000
	socket.connect(endpoint)
	return socket


if __name__ == "__main__":
	#1. import config file
	config = read_config()
	
	#1.1 parse command line arguments
	parser = argparse.ArgumentParser(description='execute unittest')
	parser.add_argument('-F', '--force-output-stdout' , action='store_true', help='log everything to stdout')
	args = parser.parse_args()
	command_line_config = args.__dict__
	
	#2. configure logging
	#configure_logging_commandline(verbose=True)
	if (command_line_config['force_output_stdout'] == True): #log everything to stdout
		configure_logging_commandline(verbose=True)
	else: #create log files
		configure_logging(verbose=True)
		#configure_logging(verbose=False)

	ssc32 = SSC32(config['serial_path'], config['serial_baud'], config['serial_timeout'])
	hexapod = Hexapod(ssc32, config)
	state = State(hexapod, config, command_line_config)
	
	#4. create global state object
	zmq_ctx = zmq.Context()
	state = State(hexapod, config, command_line_config)
	state.zmq_ctx = zmq_ctx
	
	#5. set up zeroMQ objects
	state.push = create_push(state.zmq_ctx, "ipc:///tmp/transmit_msg.ipc") #zmq socket to send messages to the hub
	subscriber = create_subscriber(state.zmq_ctx, "ipc:///tmp/received_msg.ipc") #zmq socket to receive messages from the hub
	
	poller = zmq.Poller()
	poller.register(subscriber, zmq.POLLIN)
	
	log.info('hexapod entering main loop')
	while (not state.stopped):
		try:
			socks = dict(poller.poll(timeout=50))
			for sock in socks:
				if (socks[sock] != zmq.POLLIN):
					continue
				if (sock is subscriber): #message received to be turned into packet
					cmd = sock.recv_string()
					msg = sock.recv_json()
					state.handle_sub(cmd, msg)

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
	log.info('hexapod Shutting Down')
	subscriber.close()
	state.push.close()
	state.zmq_ctx.term()
	