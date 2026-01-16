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

from datetime import datetime

from dataclasses import dataclass
from dataclasses import dataclass, field

from binascii import crc_hqx as crc16

def get_bit_slice(number, start, end):
    '''
    Extracts a slice of bits from an integer. 'start' is the least significant bit of the slice, 'end' is exclusive.
    '''
    mask = (1 << (end - start)) - 1
    shifted_number = number >> start
    return shifted_number & mask
	
@dataclass
class Peer:
	addr : int
	callsign : str = ''
	last_update : int = 0
	
	def __post_init__(self):
		self.kick()
		log.debug('new Peer %s' % (self,))
	
	def update(self, callsign):
		self.callsign = callsign
	
	def kick(self):
		self.last_update = time.monotonic()
		
	def __str__(self):
		return '0x%04X (%s)' % (self.addr, self.callsign)
	
	@staticmethod
	def from_dict(d):
		return Peer(d['addr'],d['callsign'])
	
	def as_dict(self):
		return {'addr':self.addr, 'callsign':self.callsign, 'last_update' : self.last_update}

TYPE_END 		= 0x00
TYPE_ACK 		= 0x01
TYPE_PEER_INFO 	= 0x02
TYPE_LOCATION 	= 0x03
TYPE_MESSAGE	= 0x04
TYPE_WAYPOINT	= 0x05
TYPE_PING_REQ	= 0x06
TYPE_PING_REP	= 0x07
TYPE_BEACON 	= 0x08
TYPE_KEY_PRESS  = 0x09
TYPE_HEXAPOD  	= 0x0A
TYPE_ROVER  	= 0x0B
TYPE_UI_CONTROL	= 0x0C

FORMAT_STRUCT 		= 0x00
FORMAT_STRING 		= 0x01
FORMAT_JSON_DICT 	= 0x02
FORMAT_JSON_LIST 	= 0x03
	
format_to_string = {
	FORMAT_STRUCT 		: 'struct',
	FORMAT_STRING 		: 'string',
	FORMAT_JSON_DICT 	: 'json_dict',
	FORMAT_JSON_LIST 	: 'json_list'
}
string_to_format = {value: key for key, value in format_to_string.items()}

type_to_string = {
	TYPE_ACK 		: 'ack',
	TYPE_PEER_INFO 	: 'peer_info',
	TYPE_LOCATION 	: 'location',
	TYPE_MESSAGE	: 'message',
	TYPE_WAYPOINT	: 'waypoint',
	TYPE_PING_REQ	: 'ping_request',
	TYPE_PING_REP	: 'ping_response',
	TYPE_BEACON 	: 'beacon',
	TYPE_KEY_PRESS	: 'key_press',
	TYPE_HEXAPOD	: 'hexapod',
	TYPE_ROVER		: 'rover',
	TYPE_UI_CONTROL : 'ui_control'
}
string_to_type = {value: key for key, value in type_to_string.items()}

@dataclass
class TLV_Handler:
	typ 			: int #the TLV type this instance is meant to unpack
	form 			: int #the value format this instance is meant to unpack
	dict_template 	: dict = field(default_factory=lambda : {})
	value_unpacker 	: callable = field(default=lambda length, val : tuple())
	value_packer 	: callable = field(default=lambda arr : b'')
	
	def __post_init__(self):
		assert self.typ in type_to_string
		assert self.form in format_to_string

	def handle(self, state, tlv, p):
		to_send = self.dict_template.copy()
		unpacked = self.value_unpacker(tlv.length, tlv.val)
		unpacked = [x.decode() if (type(x) is bytes) else x for x in unpacked]
		for key in to_send.keys():
			to_send[key] = unpacked[to_send[key]]
		##add standard fields
		to_send['type'] = type_to_string[self.typ]
		#to_send['format'] = format_to_string[self.form]
		to_send['src'] = p.src
		to_send['src_callsign'] = state.lookup_peer_callsign(p.src)
		to_send['dst'] = p.dst
		to_send['unique_id'] = p.unique_id
		to_send['rssi'] = p.rssi
		
		if (self.typ == TYPE_PING_REQ) and (p.dst == state.my_addr): #respond to the ping request if its addressed to me
			ping_rep = Packet(src=state.my_addr, dst=p.src, hops_start=p.hops_start, max_length=state.MAX_LENGTH())
			ping_rep.append_payload(TYPE_PING_REP, tlv.val)
			state.tx_packet(ping_rep)
			return
		elif (self.typ == TYPE_PEER_INFO):
			if (to_send['addr'] in state.peers):
				state.peers[to_send['addr']].kick()
				state.peers[to_send['addr']].update(to_send['callsign'])
			else:
				state.peers[to_send['addr']] = Peer(to_send['addr'], to_send['callsign'])
				state.sync_peers()
		elif (self.typ == TYPE_ACK) and (to_send['ack_id'] in state.packets_sent):
			log.info('ack received for 0x%04X' % (to_send['ack_id'],))
		
		log.debug('pub "%s"' % (to_send,))
		state.pub.send_string(to_send['type'], flags=zmq.SNDMORE)
		state.pub.send_json(to_send)
	
	def __require_args(self, required : list, provided : dict):
		for r in required:
			if r not in provided:
				raise ValueError('add_tlv_to_packet requires "%s" keyword argument' % (r,))
	
	def add_tlv_to_packet(self, p, **kwargs):
		#log.debug(kwargs)
		self.__require_args(required=self.dict_template.keys(), provided=kwargs)
		arr = []
		for key in self.dict_template.keys():
			arr.append(kwargs[key])	
		#log.debug(arr)
		val = self.value_packer(arr)
		if (not p.append_payload(self.typ, val)):
			log.warning('Not enough space to append TLV %s to Packet' % (kwargs,))

def waypoint_value_packer(arr):
	lat = arr[0]
	lon = arr[1]
	msg = arr[2]
	color = arr[3]
	cancel = arr[4]
	return struct.pack('<BBdd%ds%ds?' % (len(msg),len(color)), len(msg), len(color), lat, lon, msg.encode(), color.encode(), cancel)

def waypoint_value_unpacker(length, val):
	msg_len, color_len = struct.unpack('<BB', val[0:2])
	return struct.unpack('<xxdd%ds%ds?' % (msg_len,color_len), val)
	

#waypoint_handler 	= TLV_Handler(TYPE_WAYPOINT, FORMAT_JSON_DICT, dict_template={'lat':0,'lon':1,'msg':2,'color':3,'cancel':4}, 
#value_unpacker = lambda length, val : (json.loads(val.decode())), 
#value_packer=lambda arr, dict_template : json.dumps({x:arr[y] for x,y in dict_template.items()}).encode())


ack_handler 		= TLV_Handler(TYPE_ACK, FORMAT_STRUCT, dict_template={'ack_id' : 0}, value_unpacker=lambda length, val : struct.unpack('<H', val), value_packer=lambda arr : struct.pack('<H', arr[0]))
peer_info_hander	= TLV_Handler(TYPE_PEER_INFO, FORMAT_STRUCT, dict_template={'addr':0,'callsign':1}, value_unpacker=lambda length, val : [x.decode() if (type(x) is bytes) else x for x in struct.unpack('<H%ds' % (length-2,),val)], value_packer=lambda arr : struct.pack('<H', arr[0])+arr[1].encode())
location_handler 	= TLV_Handler(TYPE_LOCATION, FORMAT_JSON_LIST, dict_template={'lat':0,'lon':1}, value_unpacker=lambda length, val : json.loads(val.decode()), value_packer=lambda arr : json.dumps(arr).encode())
message_handler 	= TLV_Handler(TYPE_MESSAGE, FORMAT_STRING, dict_template={'msg':0}, value_unpacker=lambda length, val : (val.decode(),), value_packer=lambda arr: arr[0].encode())
waypoint_handler 	= TLV_Handler(TYPE_WAYPOINT, FORMAT_JSON_DICT, dict_template={'lat':0,'lon':1,'msg':2,'color':3,'cancel':4}, value_unpacker=waypoint_value_unpacker, value_packer=waypoint_value_packer)
#waypoint_handler 	= TLV_Handler(TYPE_WAYPOINT, FORMAT_JSON_DICT, dict_template={'lat':0,'lon':1,'msg':2,'color':3,'cancel':4}, value_unpacker = lambda length, val : (json.loads(val.decode())), value_packer=lambda arr, dict_template : json.dumps({x:arr[y] for x,y in dict_template.items()}).encode())
#waypoint_handler 	= TLV_Handler(TYPE_WAYPOINT, FORMAT_JSON_DICT, dict_template={'lat':0,'lon':1}, value_unpacker = lambda length, val : (json.loads(val.decode())), value_packer=lambda arr, dict_template : json.dumps({x:arr[y] for x,y in dict_template.items()}).encode())
ping_req_handler	= TLV_Handler(TYPE_PING_REQ, FORMAT_STRUCT, dict_template={'ping_id' : 0}, value_unpacker=lambda length, val : struct.unpack('<B', val), value_packer=lambda arr : struct.pack('<B', arr[0]))
ping_rep_handler	= TLV_Handler(TYPE_PING_REP, FORMAT_STRUCT, dict_template={'ping_id' : 0}, value_unpacker=lambda length, val : struct.unpack('<B', val), value_packer=lambda arr : struct.pack('<B', arr[0]))
beacon_handler		= TLV_Handler(TYPE_BEACON,   FORMAT_STRUCT)
key_press_handler	= TLV_Handler(TYPE_KEY_PRESS, FORMAT_STRUCT, dict_template={'expires':0,'pressed':1,'held':2,'released':3,'key':4}, value_unpacker=lambda length, val : struct.unpack('<H???%ds' % (length-5,),val), value_packer=lambda arr : struct.pack('<H???', *arr[:-1])+arr[-1].encode())
hexapod_handler		= TLV_Handler(TYPE_HEXAPOD, FORMAT_STRUCT, dict_template={}, value_unpacker=lambda length, val : b'', value_packer=lambda arr : b'')
rover_handler		= TLV_Handler(TYPE_ROVER, FORMAT_STRUCT, dict_template={}, value_unpacker=lambda length, val : b'', value_packer=lambda arr : b'')
ui_control_handler 	= TLV_Handler(TYPE_UI_CONTROL, FORMAT_STRUCT, dict_template={'value':0,'id':1}, value_unpacker=lambda length, val : struct.unpack('<f%ds' % (length-5,),val), value_packer=lambda arr : struct.pack('<f', arr[-2])+arr[-1].encode())

tlv_handlers = {
	TYPE_ACK 		: ack_handler,
	TYPE_PEER_INFO 	: peer_info_hander,
	TYPE_LOCATION 	: location_handler,
	TYPE_MESSAGE	: message_handler,
	TYPE_WAYPOINT	: waypoint_handler,
	TYPE_PING_REQ	: ping_req_handler,
	TYPE_PING_REP	: ping_rep_handler,
	TYPE_BEACON 	: beacon_handler,
	TYPE_KEY_PRESS	: key_press_handler,
	TYPE_HEXAPOD	: hexapod_handler,
	TYPE_ROVER		: rover_handler,
	TYPE_UI_CONTROL : ui_control_handler
}
tlv_handlers_by_string = tlv_handlers.copy()
for old,new in type_to_string.items(): 
	tlv_handlers_by_string[new]=tlv_handlers_by_string.pop(old)

@dataclass
class TLV:
	typ: int
	val: bytes
	length:int = None
	
	def __post_init__(self):
		if (type(self.val) is str):
			self.val = self.val.encode()
		self.length = len(self.val)
		assert self.length < 65536
	
	def pack(self):
		return self.typ.to_bytes(1, byteorder='little') + self.length.to_bytes(2, byteorder='little') + self.val

	@staticmethod
	def unpack(raw):
		i = 0
		while (i < len(raw)):
			if ((i+3) > len(raw)):
				break
			typ = raw[i]
			if (typ == TYPE_END):
				break
			length = int.from_bytes(raw[i+1:i+3], byteorder='little')
			if ((i+3+length) > len(raw)):
				break
			val = raw[i+3:i+3+length]
			assert len(val) == length
			i += 3+length
			if (typ not in type_to_string):
				log.warning("unknown type 0x%02X" % (typ,))
			yield TLV(typ, val, length)
			if (i >= len(raw)):
				break


POST_HEADER_IND = 11
POST_CRC_IND = 2
PACKET_STRUCT_FORMAT 			= '<HHHHHB'
PACKET_STRUCT_FORMAT_NO_CRC 	=  '<HHHHB'
PACKET_STRUCT_FORMAT_ONLY_CRC	= '<H'

@dataclass
class Packet:
	src: int
	dst: int
	payload: bytearray = b''
	want_ack: bool = False
	hops_remaining: int = 3
	hops_start: int = 3
	unique_id: int = None
	link_src: int = None
	crc: int = None
	rssi: int = 0
	max_length : int=100
	rx_time : int = 0
	ack_received : bool = None
	
	def __post_init__(self):
		if (type(self.payload) is str):
			self.payload = bytearray(self.payload.encode())
		else:
			self.payload = bytearray(self.payload)
		
		self.hops_remaining = self.hops_start
		
		if (self.link_src is None):
			self.link_src = self.src
			
		if (self.unique_id is None): #generate the unique id now
			t = int(time.monotonic()*1000).to_bytes(8, byteorder='little')
			r = random.randint(0, 4294967296).to_bytes(4, byteorder='little')
			self.unique_id = crc16(t+r,0)
		if (self.crc is None):
			self.crc = self.generate_crc()
		if (self.want_ack == True):
			self.ack_received = False
		self.rx_time = time.monotonic()
	
	def append_payload(self, typ : int, val : bytes):
		remaining_space = self.max_length - POST_HEADER_IND - len(self.payload)
		if (remaining_space < (3+len(val))): #not enough space
			return False
		self.payload.append(typ)
		self.payload.extend(len(val).to_bytes(2, byteorder='little'))
		self.payload.extend(val)
		return True
	
	def handle_payload(self, state):
		log.info('handle_payload() -> %s' % (self,))
		for tlv in TLV.unpack(self.payload):
			tlv_handlers[tlv.typ].handle(state, tlv, self)
	
	def decrement_hops(self):
		self.hops_remaining = self.hops_remaining - 1
		if (self.hops_remaining <= 0):
			return False
		return True
	
	def generate_crc(self):
		crc_bytes = bytearray(struct.pack(PACKET_STRUCT_FORMAT_NO_CRC, self.unique_id, self.link_src, self.src, self.dst, self.pack_flags()))
		crc_bytes.extend(self.payload)
		crc_bytes.extend(bytes(self.max_length-2-len(crc_bytes)))
		crc = crc16(crc_bytes, 0)
		return crc
	
	@staticmethod 
	def verify_crc(pay : bytes):
		expected = int.from_bytes(pay[0:POST_CRC_IND], byteorder='little')
		actual = crc16(pay[POST_CRC_IND:], 0)
		if (expected != actual):
			log.warning('crc failure expected 0x%04X, actual 0x%04X' % (expected, actual))
			return False
		return True
	
	def pack_flags(self):
		want_ack = 1 if (self.want_ack == True) else 0
		return (self.hops_remaining & 0x07) | ((self.hops_start & 0x07)<<4) | (want_ack << 7)
	
	@staticmethod 
	def unpack_flags(flags):
		hops_remaining = get_bit_slice(flags, 0, 3)
		hops_start = get_bit_slice(flags, 4, 7)
		want_ack = get_bit_slice(flags, 7, 8)==1
		return (hops_remaining, hops_start, want_ack)
	
	def pack(self):
		flags = self.pack_flags()
		self.crc = self.generate_crc()
		to_return = bytearray(struct.pack(PACKET_STRUCT_FORMAT, self.crc, self.unique_id, self.link_src, self.src, self.dst, self.pack_flags()))
		to_return.extend(self.payload)
		return to_return

	@staticmethod
	def unpack(pay, rssi, max_length):
		if (not Packet.verify_crc(pay)):
			return None
		(crc, unique_id, link_src, src, dst, flags) = struct.unpack(PACKET_STRUCT_FORMAT, pay[0:POST_HEADER_IND])
		(hops_remaining, hops_start, want_ack) = Packet.unpack_flags(flags)
		return Packet(rssi=rssi, link_src=link_src, src=src, dst=dst, unique_id=unique_id, crc=crc, want_ack=want_ack, hops_remaining=hops_remaining, hops_start=hops_start, max_length=max_length, payload=pay[POST_HEADER_IND:])
	
	def __eq__(self, other):
		if not isinstance(other, Packet):
			return False
		return self.unique_id == other.unique_id
	
	def __str__(self):
		return '%s bytes src:0x%04X, link_src:0x%04X dst:0x%04X, id:0x%04X, crc:0x%04X |"%s"' % (len(self.payload.strip(b'\x00')), self.src, self.link_src, self.dst, self.unique_id, self.crc, self.payload.strip(b'\x00'))
