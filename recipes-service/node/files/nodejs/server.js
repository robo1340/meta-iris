var http = require("http");
var https = require("https");
var io = require('socket.io')();
var os = require('os');
var fs = require("fs");
var express = require('express');
var child_process = require("child_process");
var zmq = require("zeromq");
var sub = zmq.socket("sub");
var rx_msg_cnt = 0;
const ZMQ_ID = 'hub';
sub.connect("ipc:///tmp/received_msg.ipc");
sub.subscribe("");
var push = zmq.socket("push");
push.connect("ipc:///tmp/transmit_msg.ipc");
var hub = zmq.socket("dealer");
hub.setsockopt(zmq.ZMQ_IDENTITY, Buffer.from(ZMQ_ID));
hub.setsockopt(zmq.ZMQ_RCVTIMEO, 5000);
hub.connect('ipc:///tmp/hub_requests.ipc');

var timeSet = false;

var text_decoder = new TextDecoder('ascii');

const spawn = child_process.spawn;

/*
function getMethods(obj) {
  var result = [];
  for (var id in obj) {
    try {
      if (typeof(obj[id]) == "function") {
        result.push(id + ": " + obj[id].toString());
      }
    } catch (err) {
      result.push(id + ": inaccessible");
    }
  }
  return result;
}
console.log(getMethods(hub));
*/

let my_addr = null;
let my_callsign = null;
let peers = {};
let link_peers = {};
const COMMANDS = ["GET_OS_VERSION","GET_HUB_CONFIG", "SET_HUB_CONFIG","GET_RADIO_CONFIG","SET_RADIO_CONFIG","SET_MY_CALLSIGN","GET_MY_CALLSIGN","GET_MY_ADDR","GET_PEERS","GET_LINK_PEERS"]
const TLV_TYPES = ["ack","peer_info","location","message","waypoint","ping_request","ping_response","beacon"];

function req_hub(cmd, pay='') {
	console.log('req_hub',cmd,pay);
	hub.send(['',cmd,pay]);
	//rsp = hub.read();
	//if (rsp === null){
	//	console.log('WARNING: req_hub failed to receive a response for', cmd);
	//	return ['',{}]
	//}
	//console.log(rsp);
	//var rsp_cmd = text_decoder.decode(rsp[0]);
	//var rsp_pay = JSON.parse(text_decoder.decode(rsp[1]));
	//if (callback !== null){
	//	callback(rsp_cmd, rsp_pay);
	//} else {
	//	return [rsp_cmd, rsp_pay];
	//}
}



//handlers for messages received from the hub process
class Handlers {
	constructor(initial_handlers){
		this.handlers = initial_handlers;
	}
	
	static handle_my_callsign(cmd, pay){
		my_callsign = pay.callsign;
		my_addr = pay.addr;
	}
	
	static handle_get_peers(cmd, pay){
		peers = pay;
	}
	
	static handle_get_link_peers(cmd,pay){
		link_peers = pay;
	}
	
	static handle_nack(cmd, pay){
		console.log("WARNING: NACK received\n", pay);
	}
	
	handle(cmd, pay){
		//console.log(cmd,pay);
		let h = this.handlers[cmd];
		if (h !== undefined){
			h(cmd, pay);
		}
		io.emit(cmd, pay); //always pass on the message to the client
	}
	
}

handler = new Handlers({
	"GET_MY_CALLSIGN" 	: Handlers.handle_my_callsign,
	"GET_PEERS" 		: Handlers.handle_my_callsign,
	"GET_LINK_PEERS" 	: Handlers.handle_my_callsign,
	"NACK" 				: Handlers.handle_my_callsign
});

//packet received from the hub process locally, passively forward to the client
hub.on("message", function(empty, type_bytes, msg_bytes) {
	//console.log(empty, type_bytes, msg_bytes);
	var cmd = text_decoder.decode(type_bytes);
	var msg = '';
	if (cmd == "NACK"){
		msg = text_decoder.decode(msg_bytes);
	} else {
		msg = JSON.parse(text_decoder.decode(msg_bytes));
	}
	handler.handle(cmd, msg);
});

//req_hub('COMMANDS');

/*
var ifaces = os.networkInterfaces();
for (var a in ifaces) {
	for (var b in ifaces[a]) {
	    var addr = ifaces[a][b];
	    if (addr.family === 'IPv4' && !addr.internal) {
			console.log("Network IP: " + addr.address );
	    }
	}
}
*/

var mime = {
    html: 'text/html',
    txt: 'text/plain',
    css: 'text/css',
    gif: 'image/gif',
    jpg: 'image/jpeg',
    png: 'image/png',
    svg: 'image/svg+xml',
	ico: 'image/x-icon',
    js: 'application/javascript',
	pbf: 'application/octet-stream'
};

function serve_check(req, resp, type){
	fs.stat(__dirname + req.url, function(err, stat) {
		if (err == null) {
			//console.log(req.url)
			//console.log(type)
			resp.writeHead(200, {"Content-Type": type}); 
			fs.createReadStream(__dirname + req.url).pipe(resp);
		} else {
			resp.writeHead(400, "Invalid URL/Method"); 
			resp.end();				
		}
	});
}

function serve_no_check(req_url, resp, type){
	resp.writeHead(200, {"Content-Type": type}); 
	fs.createReadStream(__dirname + req_url).pipe(resp);
}

var options = {
  key: fs.readFileSync('certificate/key.pem'),
  cert: fs.readFileSync('certificate/cert.pem')
};

var server = https.createServer(options, function(req, resp) {
	var type = mime[req.url.split('.').pop()] || 'text/plain';
	if (req.url == "/certificate/cert.pem"){
		serve_no_check("/certificate/cert.pem", resp, 'application/octet-stream');
	}
	if (type == "application/octet-stream"){
		serve_check(req, resp, type);
	}
	else if ((req.url == "/") || (req.url == "")){
		serve_no_check("/index.html", resp, 'text/html');
	}
	else if (req.url.startsWith("/images/")){
		serve_check(req, resp, type);
	}
	else if (req.url.split("/").length == 2){
		serve_check(req, resp, type);
	}
	else if (req.url.startsWith("/node_modules/")){
		serve_check(req, resp, type);
	}
	else {
		serve_no_check("/index.html", resp, 'text/html');
    }
});

function safe_execute(func){
	try {
		return func();
	} catch { 
		return null;
	}
}

function get_station_callsign(){
	var to_return = JSON.parse(fs.readFileSync('/bridge/conf/radio_callsign.json', 'utf8'));
	if (to_return !== null){
		return to_return;
	} else { return null;}
}

var last_user_location = null;

function get_station_gps_location(){
	var to_return = JSON.parse(fs.readFileSync('/tmp/gps.json', 'utf8'));
	if (to_return !== null){
		return to_return;
	} else {return null;}	
}

function get_location(){
	var to_return = safe_execute(get_station_gps_location);
	if (last_user_location !== null){
		return last_user_location;
	} else { 
		return null;
	}
}

/*
function set_location(msg){
	
	if (fs.existsSync('/tmp/gps.json')) {
		gps_location = safe_execute(get_station_gps_location);
		if (gps_location !== null){
			msg.coords = gps_location;
		}
	}
	
	if (msg.coords === null) {
		return null;
	}
	return msg;
}
*/

//messages received from the client to be passed to the hub process locally
io.sockets.on("connection", function(socket) {
	console.log('connected');
	
	//set up handlers for commands going to the hub
	for (const command of COMMANDS) {
		socket.on(command, function(pay){
			req_hub(command, JSON.stringify(pay))
		});
		//console.log('registered handler for ', command)
	}
	
	//set up handlers for packets being sent to the hub for transmission
	for (const type of TLV_TYPES){
		if (type == 'location'){
			socket.on(type, function(msg) {
				msg.type = type;
				push.send(JSON.stringify(msg)); 
				io.emit('location', msg);
			});
		}
		else {
			socket.on(type, function(msg) {
				msg.type = type;
				push.send(JSON.stringify(msg))
				io.emit(type, msg);
			});
		}
		
	}
	
	socket.on('time', function(msg) {
		if ((timeSet == false) && ("timestamp" in msg)){   
			timeSet = true; 
			console.log('NOT IMPLEMENTED setting timestamp: ' + msg.timestamp);
			//spawn('date',["+%s", "-s", "@"+msg.timestamp]); 
			//spawn('date',["-s", "@"+msg.timestamp]);
			spawn('timedatectl',[]);
		}
	});	
});

//packet received from the hub process locally, passively forward to the client
sub.on("message", function(type_bytes, msg_bytes) {
	var cmd = text_decoder.decode(type_bytes);
	var msg = JSON.parse(text_decoder.decode(msg_bytes));
	rx_msg_cnt += 1;
	io.emit(cmd, msg);
	io.emit('rx_msg_cnt',{'rx_msg_cnt':rx_msg_cnt});
});

/*
var intervalId = setInterval(function() {
	callsign = safe_execute(get_station_callsign);
	if (callsign === null){return;}
	gps_location = safe_execute(get_location);
	if (gps_location === null) {
		gps_location = [0,0]; //set to dummy coordinates
	}
	//msg = {'username':callsign, 'coords':gps_location,'type':'station'};
	//console.log(msg);
	//spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "location"]);
	//io.emit('location', msg);
}, 15000);
*/

//server.listen(8085);
server.listen(443);
io.listen(server);

const http_app = express();

http_app.get("*", function(req, res, next) {
    res.redirect("https://" + req.headers.host + req.path);
});

http.createServer(http_app).listen(80, function() {
    console.log("Express TTP server listening on port 80");
});
