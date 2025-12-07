
importScripts("/socket.io/socket.io.js");

function get_iso_timestamp(date) {
	var date = new Date();
    return date.toLocaleString('sv').replace(' ', 'T');
}

var my_addr;
var my_callsign;
var location_beacon_ms = 30000;

var socket;

const COMMANDS = ["GET_OS_VERSION", "GET_HUB_CONFIG", "SET_HUB_CONFIG","GET_RADIO_CONFIG","SET_RADIO_CONFIG","SET_MY_CALLSIGN","GET_MY_CALLSIGN","GET_MY_ADDR","GET_PEERS","GET_LINK_PEERS"]
const TLV_TYPES = ["ack","peer_info","location","message","waypoint","ping_request","ping_response","beacon"];
const META = ['rx_msg_cnt'];

//handlers for receiving messages from the frontend javascript
var handlers = {
	"get_radio_configs" : function(data){socket.emit('get_radio_configs', data);},
	"get_current_radio_config" : function(data){socket.emit('get_current_radio_config', data);},
	"get_current_radio_config_other" : function(data){socket.emit('get_current_radio_config_other', data);},
	"select_radio_config" : function(config){socket.emit('select_radio_config', config);}
};

function setup_handlers(){
	var wb = new WaypointBeacon();
	var b = new Beacon();
	
	for (const topic of COMMANDS) {
		handlers[topic] = function(pay){socket.emit(topic, pay);}
	}
	for (const topic of TLV_TYPES){
		if (topic == 'waypoint'){
			handlers[topic] = function(pay) {wb.update(pay);}		
		} 
		else {
			handlers[topic] = function(pay){ 
				socket.emit(topic,pay);
			}
		}
	}
	handlers["my_location"] = function(pay){b.update(pay);}
	
	//handlers for receiving messages from the server
	socket = io.connect();
	for (const topic in handlers) {
		if (topic == 'GET_MY_CALLSIGN'){
			socket.on(topic, function(pay){
				my_callsign = pay.callsign;
				postMessage([topic, pay, get_iso_timestamp()]);
			});
		}
		else if (topic == 'GET_MY_ADDR') {
			socket.on(topic, function(pay){
				my_addr = pay.addr;
				postMessage([topic, pay, get_iso_timestamp()]);
			});
		} else { //passively pass the message along to the frontend
			socket.on(topic, function(pay){postMessage([topic, pay, get_iso_timestamp()]);});
		}
	}
	socket.on('rx_msg_cnt', function(pay){postMessage(['rx_msg_cnt', pay, get_iso_timestamp()]);});
}

var state = 'stopped'

onmessage = function(e) {
	let hdr = e.data[0];
	let pay = e.data[1];
	//console.log('message from frontend',hdr,pay);
	
	if ((state == 'stopped') && (hdr == 'init')){
		console.log("backend initializing...");
		state = 'init';
		setup_handlers();
		handlers['GET_MY_CALLSIGN']();
		handlers['GET_PEERS']();
		handlers['GET_LINK_PEERS']();
		return;
	}
	else if ((state=='init') && (hdr == 'start')){
		console.log("backend starting...");
		state = 'started';
		return;
	}
	else if (state != 'started'){
		return;
	}
	
	
	const handler = handlers[hdr];
	if (handler === undefined){
		console.log("worker received unknown header: " + hdr);
		return;
	} else {
		if (pay !== undefined){
			pay.type = hdr;
		}
		handler(pay);
	}
}

////////////////////////////////////////////////////////////////
///////////////////////LOCATION RELATED/////////////////////////
////////////////////////////////////////////////////////////////


class WaypointBeacon {
	constructor(){
		this.w = undefined;
		this.running = false;
		this.waypoint_beacon_interval = undefined;
		this.last_tx = 0;
	}
	
	update(data){
		console.log('WaypointBeacon.update',data);
		let old = this.w;
		this.w = data;
		if (this.running === true){
			if (this.w.cancel === true){ //stop
				this.stop()
			}
			else if (this.w.period == 0){
				this.stop();
				this.run();
			}
			else if (this.w.period != old.period){ //period change
				this.stop()
				this.start()
			}
			else { //any other data change
				this.run();
			}
		}
		else { //not running
			if ((this.w.cancel !== true) && (this.w.lat != 0) && (this.w.lon != 0)){
				if (this.w.period == 0){ //send once only
					this.run();
				} else { //send on a schedule
					this.run();
					this.start()
				}
			}
			else if (this.w.cancel == true){
				this.stop();
			}
		}
	}
	
	stop(){
		console.log('WaypointBeacon.stop',this.w); 
		if (this.waypoint_beacon_interval !== undefined){
			clearInterval(this.waypoint_beacon_interval);
			this.waypoint_beacon_interval = undefined;
		}
		socket.emit('waypoint',this.w);
		this.running = false;
	}
	
	start(){
		this.running = true;
		console.log('WaypointBeacon.start',this.w);
		this.run = this.run.bind(this); //very important to ensure "this" is always this object
		this.waypoint_beacon_interval = setInterval(this.run, this.w.period*1000)
	}
	
	run(){
		//console.log('WaypointBeacon.run',this.w);
		if ((Date.now()-this.last_tx)<this.w.period*1000){
			return;
		} else {
			this.last_tx = Date.now();
		}
		socket.emit('waypoint',this.w);
	}
}

class Beacon {
	constructor(){
		this.w = undefined;
		this.running = false;
		this.beacon_interval = undefined;
		this.last_tx = 0;	
	}
	
	update(data){
		//console.log('Beacon.update',data);
		let old = this.w;
		this.w = data;
		if (this.running === true){
			if (this.w.cancel === true){ //stop
				this.stop()
			}
			else if (this.w.period == 0){
				this.stop();
				this.run();
			}
			else if (this.w.period != old.period){ //period change
				this.stop()
				this.start()
			}
		}
		else { //not running
			if ((this.w.cancel !== true) && (this.w.lat != 0) && (this.w.lon != 0)){
				if (this.w.period == 0){ //send once only
					this.run();
				} else { //send on a schedule
					this.start()
				}
			}
			else if (this.w.cancel == true){
				this.stop();
			}
		}
	}
	
	stop(){
		//console.log('Beacon.stop',this.w); 
		if (this.beacon_interval !== undefined){
			clearInterval(this.beacon_interval);
			this.beacon_interval = undefined;
		}
		this.running = false;
	}
	
	start(){
		this.running = true;
		//console.log('Beacon.start',this.w);
		this.run = this.run.bind(this); //very important to ensure "this" is always this object
		this.beacon_interval = setInterval(this.run, this.w.period*1000)
	}
	
	run(){
		//console.log('Beacon.run',this.w);
		if ((Date.now()-this.last_tx)<this.w.period*1000){
			return;
		} else {
			this.last_tx = Date.now();
		}
		//socket.emit('location', {'src':my_addr, 'dst':0, 'lat':37.5479, 'lon':-97.2802});
		socket.emit('location',this.w);
	}
	
}