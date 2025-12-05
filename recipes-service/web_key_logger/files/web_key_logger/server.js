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
var pub;

//var hub = zmq.socket("dealer");
//hub.setsockopt(zmq.ZMQ_IDENTITY, Buffer.from(ZMQ_ID));
//hub.setsockopt(zmq.ZMQ_RCVTIMEO, 5000);
//hub.connect('ipc:///tmp/hub_requests.ipc');

var prev_send_keypress_local;
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

function send_keypress_local_changed(new_value){
	let p = prev_send_keypress_local
	if ((new_value == true) && ((p == false)||(p === undefined))){ //setup publisher
		console.log('setup publisher')
		spawn('systemctl',['stop','hub']); 
		pub = zmq.socket("pub");
		pub.bind("ipc:///tmp/received_msg.ipc");
		
	}
	else if ((new_value == false) && (prev_send_keypress_local == true)){ //teardown publisher
		console.log('close publisher')
		if (pub !== undefined){
			pub.close()
			pub = undefined;
		}
		spawn('systemctl',['start','hub']); 
	}
	prev_send_keypress_local = new_value;
	
}

function teardown_local(){
	
	
}

//messages received from the client to be passed to the hub process locally
io.sockets.on("connection", function(socket) {
	console.log('connected');

	socket.on('key_press', function(msg) {
		//console.log(msg);
		//msg.type = 'key_press';
		
		if (msg.send_keypress_local != prev_send_keypress_local){
			send_keypress_local_changed(msg.send_keypress_local);
		}
		
		if (msg.send_keypress_local == true){
			pub.send([msg.type, JSON.stringify(msg)]);
		} else {
			push.send(JSON.stringify(msg));
		}
		
		
	});
});

//packet received from the hub process locally, passively forward to the client
sub.on("message", function(type_bytes, msg_bytes) {
	var cmd = text_decoder.decode(type_bytes);
	var msg = JSON.parse(text_decoder.decode(msg_bytes));
	io.emit(cmd, msg);
});

server.listen(443);
io.listen(server);

const http_app = express();

http_app.get("*", function(req, res, next) {
    res.redirect("https://" + req.headers.host + req.path);
});

http.createServer(http_app).listen(80, function() {
    console.log("Express TTP server listening on port 80");
});
