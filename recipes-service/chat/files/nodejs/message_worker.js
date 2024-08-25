
importScripts("/socket.io/socket.io.js");

function get_iso_timestamp(date) {
	var date = new Date();
    return date.toLocaleString('sv').replace(' ', 'T');
}

var my_username;
var location_beacon_ms;
var my_location;
var my_waypoint;

var socket;
var started = false;

var hdr = '';
var pay = '';

var waypoint_beacon = null;


handlers = {
	"newMessage" : function(data){socket.emit('newMessage', data);},
	"location" 	 : function(data){socket.emit('location', data);},
	"waypoint"	 : function(data){my_waypoint=data; socket.emit('waypoint', data);},
	"my_location": function(data){my_location = data;},
	"start_waypoint_beacon" : setup_waypoint_beacon,
	"stop_waypoint_beacon" : cancel_waypoint_beacon,
	"username"   : function(data){my_username=data}
};

onmessage = function(e) {
	//console.log('Worker: Message received');
	//console.log(e);
	
	hdr = e.data[0];
	pay = e.data[1];
	
	if ((started == false) && (hdr == 'start')){
		console.log("worker starting...");
		started = true;
		my_username = pay['username'];
		location_beacon_ms = pay['location_beacon_ms'];
		socket = io.connect({query: "username=" + my_username});
		socket.on("newMessage", function(data){postMessage(['newMessage',data, get_iso_timestamp()]);});
		socket.on("location", 	function(data){postMessage(['location',data, get_iso_timestamp()]);});
		socket.on("waypoint", 	function(data){postMessage(['waypoint',data, get_iso_timestamp()]);});
		setup_location_beacon();
		return;
	}
	else if (started == false){
		return;
	}
	
	const handler = handlers[hdr];
	if (handler === undefined){
		console.log("worker received unknown header: " + hdr);
		return;
	}
	handler(pay);
}

////////////////////////////////////////////////////////////////
///////////////////////LOCATION RELATED/////////////////////////
////////////////////////////////////////////////////////////////

function setup_location_beacon(){
	setInterval(send_location_beacon, location_beacon_ms);
}

function setup_waypoint_beacon(period_seconds){
	cancel_waypoint_beacon();
	if (period_seconds == 0) {return;}
	waypoint_period_ms = period_seconds*1000;
	waypoint_beacon = setInterval(send_waypoint_beacon, waypoint_period_ms);
}

function cancel_waypoint_beacon(){
	if (waypoint_beacon !== null){
		clearInterval(waypoint_beacon);
		waypoint_beacon = null;
	}
}

const MAX_LOCATION_TX = 5000;
var waypoint_period_ms = 5000;
var last_location_tx = -1;


function send_location_beacon(){
	if ((Date.now()-last_location_tx)<MAX_LOCATION_TX){
		return;
	} else {
		last_location_tx = Date.now();
	}
	//console.log("send_location_beacon()");
	if (my_location !== null){
		my_location.username = my_username;
		socket.emit('location', my_location)
	} else {
		console.log("user location unknown, no location beacon sent");
	}
}

var last_waypoint_tx = -1;

function send_waypoint_beacon(){
	if ((Date.now()-last_waypoint_tx)<waypoint_period_ms){
		return;
	} else {
		last_waypoint_tx = Date.now();
	}
	console.log("send_waypoint_beacon()");
	if (my_waypoint !== null){
		my_waypoint.username = my_username;
		socket.emit('waypoint', my_waypoint)
	}
}