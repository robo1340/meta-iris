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
from config import read_config
import argparse

from ssc32 import Servo
from ssc32 import Hexapod_SSC32

import controller

## @brief configure a logger and return a handle to it
## @param name name to give the logger
## @param log_path full path to the log file
## @param verbose set log level to DEBUG if True, set to INFO if false
def configure_logging(verbose=True):
	formatter = log.Formatter('%(asctime)s %(levelname)s %(message)s')
	lvl = log.DEBUG if (verbose) else log.INFO
	
	path = '/var/log/hexapod3.log'
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

'''
config = {
	'serial_path' 		: '/dev/ttySTM1',
	'serial_baud'		: 115200,
	'serial_timeout'	: 0.1,
	'advertisment_rate' : 15,
	'pantilt' : {
		'pan_ch' : 8,
		'tilt_ch' : 9,
		'pan_offset' : 0,
		'tilt_offset' : 0
	},
	'leg1' : {
		'pan_servo' : {
			'angles' : [-140, -90, 0],
			'pws'	 : [900,1500, 2500],
			'ch'	 : 28,
			'calibration' : {
				'angles' : [-90, 0],
				'pws'	 : [1500, 2500],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [500, 1425, 2400],
			'ch'	 : 29,
			'calibration' : {
				'angles' : [0,90],
				'pws'	 : [1425, 2400],				
			}
		},
		'elbow_servo' : {
			'angles' : [-160, -90, 0],
			'pws'	 : [2500, 1550, 600],
			'ch'	 : 30,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1550, 600],				
			}
		}
	},
	'leg2' : {
		'pan_servo' : {
			'angles' : [-150, -90, 0],
			'pws'	 : [950, 1500, 2425],
			'ch'	 : 12,
			'calibration' : {
				'angles' : [-90,   0],
				'pws'	 : [1500, 2425],				
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2500, 1550, 600],
			'ch'	 : 13,
			'calibration' : {
				'angles' : [0,90],
				'pws'	 : [1550, 600],				
			}
		},
		'elbow_servo' : {
			'angles' : [-165, -90, 0],
			'pws'	 : [525, 1500, 2425],
			'ch'	 : 14,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1500, 2425],				
			}
		}
	},		
	'leg3' : {
		'pan_servo' : {
			'angles' : [-140, -90, -40],
			'pws'	 : [800,1475, 1975],
			'ch'	 : 20,
			'calibration' : {
				'angles' : [-90, -40],
				'pws'	 : [1475, 1975],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [550, 1475, 2500],
			'ch'	 : 21,
			'calibration' : {
				'angles' : [0,90],
				'pws'	 : [1475, 2500],				
			}
		},
		'elbow_servo' : {
			'angles' : [-170, -90, -5],
			'pws'	 : [2450, 1400, 500],
			'ch'	 : 22,
			'calibration' : {
				'angles' : [-90,-5],
				'pws'	 : [1400, 500],				
			}
		}
	},	
	'leg4' : {
		'pan_servo' : {
			'angles' : [-135, -90, -41],
			'pws'	 : [2050,1575, 1025],
			'ch'	 : 4,
			'calibration' : {
				'angles' : [-90, -41], 
				'pws'	 : [1575, 1025],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2500, 1600, 625],
			'ch'	 : 5,
			'calibration' : {
				'angles' : [-90,90],
				'pws'	 : [2500, 625],				
			}
		},
		'elbow_servo' : {
			'angles' : [-165, -90, 0],
			'pws'	 : [550, 1500, 2500],
			'ch'	 : 6,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1500, 2500],				
			}
		}
	},			
	'leg5' : {
		'pan_servo' : {
			'angles' : [-180, -90, -45],
			'pws'	 : [500,1450, 1975],
			'ch'	 : 16,
			'calibration' : {
				'angles' : [-180, -90], 
				'pws'	 : [500, 1450],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [600, 1525, 2475],
			'ch'	 : 17,
			'calibration' : {
				'angles' : [-90,90],
				'pws'	 : [600, 2475],				
			}
		},
		'elbow_servo' : {
			'angles' : [-160, -90, 0],
			'pws'	 : [2500, 1525, 550],
			'ch'	 : 18,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1525, 550],				
			}
		}
	},		
	'leg6' : {
		'pan_servo' : {
			'angles' : [-180, -90, -30],
			'pws'	 : [725, 1575, 2125],#HS-485 values
			'ch'	 : 0,
			'calibration' : {
				'angles' : [-180, -90],
				#'pws'	 : [625, 1500],	
				'pws'	 : [725, 1575],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2450, 1475, 535],
			'ch'	 : 1,
			'calibration' : {
				'angles' : [-90,90],
				'pws'	 : [2450, 535],				
			}
		},
		'elbow_servo' : {
			'angles' : [-170, -90, 0],
			'pws'	 : [625, 1425, 2310],
			'ch'	 : 2,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1425, 2310],				
			}
		}
	},

	'shoulder_length_mm' : 75, #76
	'elbow_length_mm'	: 104,
}
'''

config = {
	'serial_path' 		: '/dev/ttySTM1',
	'serial_baud'		: 115200,
	'serial_timeout'	: 0.1,
	'advertisment_rate' : 15,
	'pantilt' : {
		'pan_ch' : 8,
		'tilt_ch' : 9,
		'pan_offset' : 0,
		'tilt_offset' : 0
	},
	'leg1' : {
		'pan_servo' : {
			'angles' : [-140, -90, 0],
			'pws'	 : [900,1500, 2500],
			'ch'	 : 28,
			'calibration' : {
				'angles' : [-90, 0],
				'pws'	 : [1500, 2500],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [500, 1500, 2500],
			'ch'	 : 29,
			'calibration' : {
				'angles' : [0,90],
				'pws'	 : [1500, 2500],				
			}
		},
		'elbow_servo' : {
			'angles' : [-160, -90, 0],
			'pws'	 : [2500, 1600, 500],
			'ch'	 : 30,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1600, 500],				
			}
		}
	},
	'leg2' : {
		'pan_servo' : {
			'angles' : [-150, -90, 0],
			'pws'	 : [950, 1500, 2425],
			'ch'	 : 12,
			'calibration' : {
				'angles' : [-90,   0],
				'pws'	 : [1500, 2425],				
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2500, 1500, 500],
			'ch'	 : 13,
			'calibration' : {
				'angles' : [0,90],
				'pws'	 : [1500, 500],				
			}
		},
		'elbow_servo' : {
			'angles' : [-165, -90, 0],
			'pws'	 : [525, 1500, 2425],
			'ch'	 : 14,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1500, 2425],				
			}
		}
	},		
	'leg3' : {
		'pan_servo' : {
			'angles' : [-140, -90, -40],
			'pws'	 : [800,1475, 1975],
			'ch'	 : 20,
			'calibration' : {
				'angles' : [-90, -40],
				'pws'	 : [1475, 1975],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [550, 1475, 2500],
			'ch'	 : 21,
			'calibration' : {
				'angles' : [0,90],
				'pws'	 : [1475, 2500],				
			}
		},
		'elbow_servo' : {
			'angles' : [-170, -90, -5],
			'pws'	 : [2450, 1400, 500],
			'ch'	 : 22,
			'calibration' : {
				'angles' : [-90,-5],
				'pws'	 : [1400, 500],				
			}
		}
	},	
	'leg4' : {
		'pan_servo' : {
			'angles' : [-135, -90, -41],
			'pws'	 : [2050,1575, 1025],
			'ch'	 : 4,
			'calibration' : {
				'angles' : [-90, -41], 
				'pws'	 : [1575, 1025],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2500, 1600, 625],
			'ch'	 : 5,
			'calibration' : {
				'angles' : [-90,90],
				'pws'	 : [2500, 625],				
			}
		},
		'elbow_servo' : {
			'angles' : [-165, -90, 0],
			'pws'	 : [550, 1500, 2500],
			'ch'	 : 6,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1500, 2500],				
			}
		}
	},			
	'leg5' : {
		'pan_servo' : {
			'angles' : [-180, -90, -45],
			'pws'	 : [500,1450, 1975],
			'ch'	 : 16,
			'calibration' : {
				'angles' : [-180, -90], 
				'pws'	 : [500, 1450],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [600, 1525, 2475],
			'ch'	 : 17,
			'calibration' : {
				'angles' : [-90,90],
				'pws'	 : [600, 2475],				
			}
		},
		'elbow_servo' : {
			'angles' : [-160, -90, 0],
			'pws'	 : [2500, 1525, 550],
			'ch'	 : 18,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1525, 550],				
			}
		}
	},		
	'leg6' : {
		'pan_servo' : {
			'angles' : [-180, -90, -30],
			'pws'	 : [725, 1575, 2125],#HS-485 values
			'ch'	 : 0,
			'calibration' : {
				'angles' : [-180, -90],
				#'pws'	 : [625, 1500],	
				'pws'	 : [725, 1575],						
			}
		},
		'shoulder_servo' : {
			'angles' : [-90,0,90],
			'pws'	 : [2450, 1475, 535],
			'ch'	 : 1,
			'calibration' : {
				'angles' : [-90,90],
				'pws'	 : [2450, 535],				
			}
		},
		'elbow_servo' : {
			'angles' : [-170, -90, 0],
			'pws'	 : [625, 1500, 2310],
			'ch'	 : 2,
			'calibration' : {
				'angles' : [-90,0],
				'pws'	 : [1500, 2500],				
			}
		}
	},

	'shoulder_length_mm' : 75, #76
	'elbow_length_mm'	: 104,
}

if __name__ == "__main__":
	#1. import config file
	#config = read_config()
	
	
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

	servos = []
	for name,side in [('leg1','left'),('leg2','right'),('leg3','left'),('leg4','right'),('leg5','left'),('leg6','right')]:
		leg_config = config[name]
		coxa = Servo.from_dict(leg_config['pan_servo'])
		femur = Servo.from_dict(leg_config['shoulder_servo'])
		tibia = Servo.from_dict(leg_config['elbow_servo'])
		servos.append(coxa)
		servos.append(femur)
		servos.append(tibia)
	ssc32 = Hexapod_SSC32(config['serial_path'], config['serial_baud'], config['serial_timeout'], servos)
	
	state = State(config, command_line_config, ssc32)
	
	#4. create global state object
	zmq_ctx = zmq.Context()
	state.zmq_ctx = zmq_ctx
	
	#5. set up zeroMQ objects
	state.push = create_push(state.zmq_ctx, "ipc:///tmp/transmit_msg.ipc") #zmq socket to send messages to the hub
	subscriber = create_subscriber(state.zmq_ctx, "ipc:///tmp/received_msg.ipc") #zmq socket to receive messages from the hub
	
	poller = zmq.Poller()
	poller.register(subscriber, zmq.POLLIN)
	
	log.info('hexapod entering main loop')
	while (not state.stopped):
		try:
			socks = dict(poller.poll(timeout=10))
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
			state.controller.sit()
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
	
