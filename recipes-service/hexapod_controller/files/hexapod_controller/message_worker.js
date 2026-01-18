importScripts("/socket.io/socket.io.js");

function get_iso_timestamp(date) {
	var date = new Date();
    return date.toLocaleString('sv').replace(' ', 'T');
}

var socket;

const TLV_TYPES = ["key_press"];
const TYPES = ['rover','hexapod'];


function setup_handlers(){	
	//handlers for receiving messages from the server
	socket = io.connect();
	for (const topic in TLV_TYPES) {
		socket.on(topic, function(pay){postMessage([topic, pay, get_iso_timestamp()]);});
	}
}

var state = 'stopped'

onmessage = function(e) {
	let hdr = e.data[0];
	let pay = e.data[1];
	
	if (state == 'stopped'){
		setup_handlers();
		state = 'started';
	}
	
	//console.log('message from frontend',hdr,pay);
	if (hdr == 'key_press'){
		console.log(hdr, pay.key);
	}
	
	socket.emit(hdr, pay);

}
