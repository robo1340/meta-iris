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
from state import Servo
from state import Arm
from config import read_config
import argparse

## @brief configure a logger and return a handle to it
## @param name name to give the logger
## @param log_path full path to the log file
## @param verbose set log level to DEBUG if True, set to INFO if false
def configure_logging(verbose=True):
	formatter = log.Formatter('%(asctime)s %(levelname)s %(message)s')
	lvl = log.DEBUG if (verbose) else log.INFO
	
	path = '/var/log/robot_arm.log'
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
	#config = read_config()
	
	config = {
		'serial_path' 		: '/dev/ttySTM1',
		'serial_baud'		: 115200,
		'serial_timeout'	: 0.1,
		'advertisment_rate' : 15,
		
		'arm' : {
			'pan_servo' : {
				'angles' : [0,   90,  180],
				'pws'	 : [2500,1550, 650],
				'ch'	 : 0,
				'calibration' : {
					'angles' : [0,   90],
					'pws'	 : [2500, 1550],						
				}
			},
			'shoulder_servo' : {
				'angles' : [0,    90,   135], 
				'pws'	 : [500, 1550, 2375], #725
				'ch'	 : 1,
				'calibration' : {
					'angles' : [0,90],
					'pws'	 : [500, 1550],				
				}
			},
			'elbow_servo' : {
				'angles' : [0, 90, 115],
				'pws'	 : [500, 1500, 2200], #725
				'ch'	 : 2,
				'calibration' : {
					'angles' : [0,90],
					'pws'	 : [500, 1500],				
				}
			},
			'wrist_servo' : { 
				'angles' : [-135, -90, 0],
				'pws'	 : [500, 1500, 2500], #600
				'ch'	 : 3,
				'calibration' : {
					'angles' : [-90,0],
					'pws'	 : [1500, 2500],				
				}
			},
			'cuff_servo' : {
				'angles' : [-90, 0, 90],
				'pws'	 : [600, 1500, 2500],
				'ch'	 : 5,
				'calibration' : {
					'angles' : [-90,90],
					'pws'	 : [600, 2500],	#2450			
				}
			},
			'grip_servo' : {
				'angles' : [0,  45, 90],
				'pws'	 : [2375, 1500, 1125],
				'ch'	 : 4,
				'calibration' : {
					'angles' : [0,    90],
					'pws'	 : [2375, 1125],				
				}
			}
		},
		'base_height_mm' : 73,
		'shoulder_length_mm' : 144,
		'elbow_length_mm'	: 184,
		'wrist_length_mm' : 57.2,
		'gripper_length_mm' : 57.4
	}
	
	
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
	arm = Arm(ssc32, config['arm'], config['base_height_mm'], config['shoulder_length_mm'], config['elbow_length_mm'], config['wrist_length_mm'], config['gripper_length_mm'])
	
	state = State(config, ssc32, arm)
	
	#4. create global state object
	zmq_ctx = zmq.Context()
	state.zmq_ctx = zmq_ctx
	
	#5. set up zeroMQ objects
	state.push = create_push(state.zmq_ctx, "ipc:///tmp/transmit_msg.ipc") #zmq socket to send messages to the hub
	subscriber = create_subscriber(state.zmq_ctx, "ipc:///tmp/received_msg.ipc") #zmq socket to receive messages from the hub
	
	poller = zmq.Poller()
	poller.register(subscriber, zmq.POLLIN)
	
	log.info('arm entering main loop')
	while (not state.stopped):
		try:
			socks = dict(poller.poll(timeout=25))
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
	