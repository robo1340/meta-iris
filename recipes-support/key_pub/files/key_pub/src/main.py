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

from threading import Thread
from sshkeyboard import listen_keyboard, stop_listening


# Function to execute when a key is pressed
def on_key_press(q, key):
	#log.debug(f"Key '{key}' pressed")
	q.put_nowait(('pressed',key))


# Function to execute when a key is released (optional)
def on_key_release(q, key):
	#log.debug(f"Key '{key}' released")
	q.put_nowait(('released',key))

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

def create_pub(context, endpoint):
	socket = context.socket(zmq.PUB)
	socket.linger = 1000
	socket.bind(endpoint)
	return socket

if __name__ == "__main__":
	#1. import config file
	config = read_config()
	
	#1.1 parse command line arguments
	parser = argparse.ArgumentParser(description='execute unittest')
	parser.add_argument('-F', '--force-output-stdout' , action='store_true', help='log everything to stdout')
	parser.add_argument('-l', '--send-local' , action='store_true', help='send key press events locally instead of over the radio')
	args = parser.parse_args()
	command_line_config = args.__dict__

	log.basicConfig(level=log.DEBUG)
	
	#4. create global state object
	zmq_ctx = zmq.Context()
	state = State(config, command_line_config)
	state.zmq_ctx = zmq_ctx
	
	#5. set up zeroMQ objects
	if (command_line_config['send_local'] == True):
		log.info('Sending key press events locally, shutting down hub while key press client is running')
		os.system('systemctl stop hub')
		state.push = create_pub(state.zmq_ctx, "ipc:///tmp/received_msg1.ipc")
		time.sleep(1)
	else:
		state.push = create_push(state.zmq_ctx, "ipc:///tmp/transmit_msg.ipc") #zmq socket to send messages to the hub
	subscriber = create_subscriber(state.zmq_ctx, "ipc:///tmp/received_msg.ipc") #zmq socket to receive messages from the hub
	
	poller = zmq.Poller()
	poller.register(subscriber, zmq.POLLIN)
	
	def keyboard_thread_task(state):
		listen_keyboard(
			on_press=functools.partial(on_key_press, state.key_queue),
			on_release=functools.partial(on_key_release, state.key_queue),
			delay_second_char=0.75,
			delay_other_chars=0.1,
			sequential=True,
			#debug=True
		)
	keyboard_thread = Thread(target=keyboard_thread_task, args=(state,))
	keyboard_thread.start()
	
	log.info('key_pub entering main loop')
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
	log.info('key_pub Shutting Down')
	if (command_line_config['send_local'] == True):
		os.system('systemctl start hub')
	stop_listening() # Stop listening for key presses
	keyboard_thread.join()
	subscriber.close()
	state.push.close()
	state.zmq_ctx.term()
	