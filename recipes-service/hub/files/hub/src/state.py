import logging as log
import time
import os
import sys
import functools
import json
import zmq
import traceback
import struct
import random
import collections
from dataclasses import dataclass
from datetime import datetime

from packet import Packet
from packet import Peer
from packet import tlv_handlers_by_string

from request_handler import handle_get_peers
from request_handler import handle_get_link_peers

from util import safe_read_json
from util import safe_write_json
PEERS_PATH = '/hub/volatile/peers.json'
LINK_PEERS_PATH = '/hub/volatile/link_peers.json'

wall = lambda msg : os.system("wall -n \"%s\"" % (msg,))

#a simple task scheduler class because I don't want to use Timer to schedule tasks
class Task:
	def __init__(self, period, callback, random_precharge=False, variance_ratio=0):
		self.period = period
		self.callback = callback
		self.last_update = time.monotonic()
		self.variance_ratio = variance_ratio
		if (random_precharge == True):
			self.last_update = self.last_update - random.uniform(0, self.period)
	def reset(self):
		self.last_update = 0
	def run(self):
		if ((time.monotonic() - self.last_update) > self.period):
			self.last_update = time.monotonic()
			if (self.variance_ratio > 0):
				self.last_update += random.uniform(-1*self.period*self.variance_ratio, self.period*self.variance_ratio)
			self.callback()

class RateLimiter:
	def __init__(self, topic_rate_limits, default_rate_limit=1):
		self.topics = {}
		self.topic_rate_limits = topic_rate_limits
		self.default_rate_limit = default_rate_limit
	
	def ask(self, topic):
		if (topic not in self.topics):
			self.topics[topic] = time.monotonic()
			return True
		else:
			if (topic in self.topic_rate_limits):
				if (self.topic_rate_limits[topic] <= 0): #rate limiting disabled for this topic
					return True
				elif (time.monotonic() - self.topics[topic]) > self.topic_rate_limits[topic]:
					self.topics[topic] = time.monotonic()
					return True
			elif (time.monotonic() - self.topics[topic]) > self.default_rate_limit:
				self.topics[topic] = time.monotonic()
				return True
		return False

#a task scheduler that only allows the callback to be called with a
#specific set of arguments up to a maximum rate
#example: if max_period is set to 30
#then callback(1) may only be called every 30 seconds
# and callback(2) may only be called every 30 seconds
# callback(1) getting called does not affect the rate callback(2) can be called
class RateLimitedTask:
	def __init__(self, max_period, callback):
		self.max_period = max_period
		self.callback = callback
		self.kwargs_list = [] #a running list of all kwargs that are passed into run
		self.last_calls  = [] #a list of the last time kwargs was called with the corresponding kwargs in self.kwargs_list
	def run (self, **kwargs):
		if (kwargs not in self.kwargs_list): #if callback hasn't been called with these kwargs yet
			self.kwargs_list.append(kwargs)
			self.last_calls.append(time.monotonic())
			self.callback(**kwargs)
		else: #if the callback has already been called with these kwargs, make sure max_period has ellapsed
			i = self.kwargs_list.index(kwargs)
			if ((time.monotonic() - self.last_calls[i]) > self.max_period):
				self.last_calls[i] = time.monotonic()
				self.callback(**kwargs)

class CappedDict(collections.OrderedDict):
	default_max_size = 50
	def __init__(self, *args, **kwargs):
		self.max_size = kwargs.pop('max_size', self.default_max_size)
		super(CappedDict, self).__init__(*args, **kwargs)
		
	def __setitem__(self, key, val):
		if key not in self:
			max_size = self.max_size-1  # so the dict is sized properly after adding a key
			self._prune_dict(max_size)
		super(CappedDict, self).__setitem__(key, val)
		
	def update(self, **kwargs):
		super(CappedDict, self).update(**kwargs)
		self._prune_dict(self.max_size)
   
	def _prune_dict(self, max_size):
		if len(self) >= max_size:
			diff = len(self) - max_size
			deleted = 0
			for k in self.keys():
				del self[k]
				deleted += 1
				if (deleted >= diff):
					break

def read_ini(path='/tmp/packet_length.txt'):
	with open(path, 'r') as f:
		return int(f.read())
		

# A class to hold the state of the program
class State:
	def __init__(self, config, args, my_addr, my_callsign, nodejs_server):
		self.zmq_ctx = None
		self.push = None #zmq socket to push packets to transmit to the radio program
		self.pub = None #zmq socket to publish process packets from the radio program
		self.nodejs_server = nodejs_server #zmq socket used to send/receive control messages to the nodejs server
		self.stopped = False
		self.args = args
		self.config = config
		self.my_addr = my_addr
		self.my_callsign = my_callsign
		self.restart_snap = None
		
		self.max_length = read_ini()
		log.info('packet length %d' % (self.max_length,))
		self.MAX_LENGTH = lambda : self.max_length
		self.BROADCAST_ADDR = lambda : self.config['broadcast_addr'] 
		self.PEER_TIMEOUT_S = lambda : self.config['peer_timeout_s']
		self.DEFAULT_HOPS = lambda : self.config['default_hops']
		self.RETRANSMIT_WAIT_MULTIPLIER_S = lambda : self.config['retransmit_wait_multiplier_s']
		
		self.packets_to_retransmit = {} #keys are unique id's, value is Packet
		self.packets_retransmitted = CappedDict(max_size=50)
		self.packets_sent = CappedDict(max_size=50)
		self.peers = {int(x):Peer.from_dict(y) for x,y in safe_read_json(PEERS_PATH, silent=True, default={}).items()}
		self.link_peers = {int(x):Peer.from_dict(y) for x,y in safe_read_json(LINK_PEERS_PATH, silent=True, default={}).items()}
		#self.peers[1] = Peer(1,'fake')
		#self.sync_peers()
		self.peers_to_ignore = []
		for p in self.config['peers_to_ignore']:
			if (type(p) is int):
				self.peers_to_ignore.append(p)
			elif (type(p) is str) and (p.startswith('0x')):
				self.peers_to_ignore.append(int(p,base=16))
		
		self.rate_limiter = RateLimiter(self.config['topic_rate_limits'], self.config['default_topic_rate_limit_s'])
		
		self.sync_peers_task   = Task(60, self.sync_peers)
		#self.tx_peer_info_task    = Task(self.config['peer_info_tx_s'], self.tx_peer_info, random_precharge=True, variance_ratio=0.5)
		self.tx_peer_info_task    = Task(self.config['peer_info_tx_s'], self.tx_peer_info)
		#self.send_user_message_task    = Task(60, self.send_user_message, random_precharge=True)
		self.remove_stale_peers_task    = Task(60, self.remove_stale_peers)
		#self.module_id_request_task = RateLimitedTask(self.MOD_ID_REQ_RATE_MAX(), self.update_reported_properties)
	
	def lookup_peer_callsign(self, addr : int):
		if (addr in self.peers):
			return self.peers[addr].callsign
		else:
			return ''
			
	def lookup_peer_addr(self, callsign : str):
		for peer in self.peers.values():
			if (peer.callsign == callsign):
				return peer.addr
		return None
	
	def handle_packet(self, rssi, pay):
		p = Packet.unpack(pay, rssi, max_length=self.MAX_LENGTH())
		if (p is None):
			return
		elif (p.src == self.my_addr):
			return
		
		#for testing only
		if p.link_src in self.peers_to_ignore: #ignore this peer, pretend this message was never received
			log.warning('ignoring %s' % (p,))
			return
		
		if (p.link_src not in self.link_peers):
			log.info('new link_peer discovered: 0x%04X' % (p.link_src,))
			self.link_peers[p.link_src] = Peer(self.my_addr)
			self.sync_peers()
		else:
			self.link_peers[p.link_src].kick()
		
		if (p.unique_id not in self.packets_sent) and ((p.dst == self.my_addr) or (p.dst == self.BROADCAST_ADDR())):
			p.handle_payload(self)
			if ( ((p.dst == self.my_addr)or(p.dst==self.BROADCAST_ADDR())) and (p.want_ack == True) and (p.src != self.my_addr) ):
				self.tx_ack(p)
		if ((p.dst != self.my_addr) or (p.dst == self.BROADCAST_ADDR())): #packet might be retransmitted
			if (p.src not in self.peers):
				self.peers[p.src] = Peer(p.src)
			
			if (p.unique_id in self.packets_to_retransmit): #this packet has been received before and has now been retransmitted, do not retransmit it here
				#log.debug('already retransmitting')
				self.packets_to_retransmit.pop(p.unique_id)
			elif (p.unique_id in self.packets_retransmitted): #this packet has already been retransmitted by me
				#log.debug('already retransmitted')
				pass
			elif (p.dst == self.my_addr): #this packet was addressed to me
				#log.debug('addr to me')
				pass
			elif (p.unique_id in self.packets_sent): #this packet was originally mine
				#log.debug('i sent this')
				pass
			elif (p.decrement_hops()): #hops still available add to list to retransmit
				p.link_src = self.my_addr
				log.info('retransmit %s' % (p,))
				self.packets_to_retransmit[p.unique_id] = p
			else:
				pass
				#log.debug('no hops remain')
	
	def tx_packet_adv(self, **kwargs):
		#log.info(kwargs);
		if ('type' not in kwargs):
			log.warning('tx_packet_adv required keyword argument "type"')
			return
		elif (not self.rate_limiter.ask(kwargs['type'])):
			log.debug('type %s was throttled' % (kwargs['type'],))
			return
		
		if ('dst' not in kwargs):
			kwargs['dst'] = 0
		
		for key in ['dst','ack_id', 'ping_id', 'addr']:
			if (key in kwargs) and (type(kwargs[key]) is str) and (kwargs[key].lower().startswith('0x')):
				kwargs[key] = int(kwargs[key],base=16)
			elif (key in kwargs) and (type(kwargs[key]) is str) and (kwargs[key].isnumeric()):
				kwargs[key] = int(kwargs[key])
				
		if type(kwargs['dst']) is str: #callsign was passed in
			kwargs['dst'] = self.lookup_peer_addr(kwargs['dst'])
		
		hops_start = self.DEFAULT_HOPS() if ('hops_start' not in kwargs) else kwargs['hops_start']
		want_ack = False if ('want_ack' not in kwargs) else kwargs['want_ack']
			
		to_send = Packet(src=self.my_addr, dst=kwargs['dst'], max_length=self.MAX_LENGTH(), hops_start=hops_start, want_ack=want_ack, unique_id=None if 'unique_id' not in kwargs else kwargs['unique_id'])
		if 'type' in kwargs:
			if (kwargs['type'] in tlv_handlers_by_string):
				try:
					tlv_handlers_by_string[kwargs['type']].add_tlv_to_packet(to_send, **kwargs)
				except ValueError as ex:
					log.warning('could not add payload to packet "%s"' % (ex,))
			else:
				log.warning('no handler for type "%s"' % (kwargs['type'],))
		else:
			log.warning('packet formed from %s %s is being sent with no payload' % (kwargs,to_send))
		self.tx_packet(to_send)
	
	def tx_packet(self, p, retransmit=False):
		log.debug('sending "%s"' % (p,))
		if (self.args['no_tx'] == False):
			self.push.send(p.pack())
		else:
			log.debug('transmissions disabled')
		if (retransmit == False):
			self.packets_sent[p.unique_id] = p
		elif (retransmit == True):
			self.packets_retransmitted[p.unique_id] = True
	
	def tx_ack(self, p):
		log.info('sending ack for 0x%04x' % (p.unique_id,))
		to_send = Packet(src=self.my_addr, dst=self.BROADCAST_ADDR(), max_length=self.MAX_LENGTH(), hops_start=p.hops_start)
		tlv_handlers_by_string['ack'].add_tlv_to_packet(to_send, ack_id=p.unique_id)
		self.tx_packet(to_send)
	
	def tx_peer_info(self):
		to_send = Packet(src=self.my_addr, dst=self.BROADCAST_ADDR(), max_length=self.MAX_LENGTH())
		tlv_handlers_by_string['peer_info'].add_tlv_to_packet(to_send, addr=self.my_addr, callsign=self.my_callsign)
		self.tx_packet(to_send)
		#self.tx_packet_adv(dst=self.BROADCAST_ADDR(), src=self.my_addr, type='location',lat=37.5479+random.uniform(-0.01,0.01),lon=-97.2802+random.uniform(-0.01,0.01))
	def send_user_message(self):
		for peer in self.peers.values():
			msg = 'hello %s %d' % (peer.callsign,random.randint(0,100)) 
			self.tx_packet_adv(dst=peer.addr, type='message', want_ack=True, msg=msg)
	
	def remove_stale_peers(self):
		to_remove = []
		now = time.monotonic()
		for p,t in self.link_peers.items():
			if ((now - t.last_update) > self.PEER_TIMEOUT_S()):
				to_remove.append(p)
		for p in to_remove:
			log.debug('link_peer 0x%04X is stale' % (p,))
			self.link_peers.pop(p)
		if (len(to_remove) > 0):
			self.sync_peers()
			
		to_remove = []
		for i,p in self.peers.items():
			if ((now - p.last_update) > self.PEER_TIMEOUT_S()):
				to_remove.append((i,p))
		for i,p in to_remove:
			log.debug('peer %s is stale' % (p,))
			self.peers.pop(i)
		if (len(to_remove) > 0):
			self.sync_peers()
	
	def sync_peers(self):
		log.debug('sync_peers()')
		pay = {addr:peer.as_dict() for addr,peer in self.peers.items()}
		safe_write_json(PEERS_PATH, pay);
		self.pub.send_string('GET_PEERS', flags=zmq.SNDMORE)
		self.pub.send_json(pay)
		pay = {addr:peer.as_dict() for addr,peer in self.link_peers.items()}
		safe_write_json(LINK_PEERS_PATH, pay);
		self.pub.send_string('GET_LINK_PEERS', flags=zmq.SNDMORE)
		self.pub.send_json(pay)

	def retransmit_task(self):
		now = time.monotonic()
		to_remove = []
		for p in self.packets_to_retransmit.values():
			if ((now - p.rx_time) > p.rssi*self.RETRANSMIT_WAIT_MULTIPLIER_S()):
				log.info('retransmitting')
				self.tx_packet(p, retransmit=True)
				to_remove.append(p.unique_id)
		for r in to_remove:
			self.packets_to_retransmit.pop(r)
		
	def run(self):
		self.remove_stale_peers_task.run()
		#self.sync_peers_task.run();
		self.tx_peer_info_task.run()
		#self.send_user_message_task.run()
		self.retransmit_task()
		if (self.restart_snap is not None):
			#if (time.monotonic() - self.restart_snap) > 0.1:
			log.info('!!!restart snap')
			os.system('systemctl restart snap')
		
		
	def __str__(self):
		return '0x%04X (%s)' % (self.my_addr, self.my_callsign)










