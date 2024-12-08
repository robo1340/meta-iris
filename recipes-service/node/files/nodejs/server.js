var http = require("http");
var https = require("https");
var io = require('socket.io')();
var os = require('os');
var fs = require("fs");
var child_process = require("child_process");
var zmq = require("zeromq");
var sub = zmq.socket("sub");
sub.connect("ipc:///tmp/chat.ipc");
sub.subscribe("");
var timeSet = false;

const spawn = child_process.spawn;

var currentUsers = [];
var resourceToFunction = {};
var ifaces = os.networkInterfaces();

for (var a in ifaces) {
	for (var b in ifaces[a]) {
	    var addr = ifaces[a][b];
	    if (addr.family === 'IPv4' && !addr.internal) {
			console.log("Network IP: " + addr.address );
	    }
	}
}

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

var options = {
  key: fs.readFileSync('certificate/key.pem'),
  cert: fs.readFileSync('certificate/cert.pem')
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

io.sockets.on("connection", function(socket) {
    console.log("connected: " + socket.handshake.query.username);
    currentUsers.push(socket.handshake.query.username);
    socket.on('newMessage', function(msg) {
		spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "newMessage"]);
        io.emit('newMessage', msg);
    });
	socket.on('time', function(msg) {
		if ((timeSet == false) && ("timestamp" in msg)){   
			timeSet = true; 
			console.log('NOT IMPLEMENTED setting timestamp: ' + msg.timestamp);
			//spawn('date',["+%s", "-s", "@"+msg.timestamp]); 
			//spawn('date',["-s", "@"+msg.timestamp]);
			spawn('timedatectl',[]);
		}
	});
	socket.on('location', function(msg) {
		msg = set_location(msg)
		if (msg !== null){
			//console.log('new location for ' + msg.username + ': ' + msg.coords)
			spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "location"]);
			io.emit('location', msg);
			if (msg.coords !== undefined){
				last_user_location = msg.coords;
			}
		}
	});
	socket.on('waypoint', function(msg) {
		//console.log('new location for ' + msg.username + ': ' + msg.coords)
		spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "waypoint"]);
		io.emit('waypoint', msg);
	});	
	socket.on('get_radio_configs', function(msg) {
		var files;
		console.log(msg)
		if (msg['type'] == 'modem'){
			files = fs.readdirSync('/bridge/conf/si4463/all/wds_generated');
		}
		else if (msg['type'] == 'general'){
			files = fs.readdirSync('/bridge/conf/si4463/all/general');
		}
		else if (msg['type'] == 'packet'){
			files = fs.readdirSync('/bridge/conf/si4463/all/packet');
		}
		else if (msg['type'] == 'preamble'){
			files = fs.readdirSync('/bridge/conf/si4463/all/preamble');
		}

		console.log(files);
		io.emit('get_radio_configs', {'config_paths':files,'config_type':msg['type']});
	});
	socket.on('get_current_radio_config', function(msg) {
		var files = fs.readdirSync('/bridge/conf/si4463/modem/');
		console.log(files);
		if (files.length == 0){return;}
		io.emit('get_current_radio_config', fs.realpathSync('/bridge/conf/si4463/modem/' + files[0]));
	});
	socket.on('get_current_radio_config_other', function(msg) {
		var files = fs.readdirSync('/bridge/conf/si4463/other/');
		console.log(files);
		//if (files.length == 0){return;}
		var to_return = [];
		for (i in files) {
			to_return.push(fs.realpathSync('/bridge/conf/si4463/other/'+files[i]));
		}
		//console.log(to_return);
		io.emit('get_current_radio_config_other', to_return);
	});
	socket.on('select_radio_config', function(msg) {
		console.log('reconfiguring bridge ' + msg);
		spawn('/usr/bin/python3', ["/bridge/scripts/bridge_configure.py", "-t", msg['config_type'], "-sm", msg['selected']]); 
	});
	socket.on('ping_request', function(msg) {
		//console.log(msg);
		if (msg['dst'] == safe_execute(get_station_callsign)){
			var rsp = {'src':safe_execute(get_station_callsign),'dst':msg['src'],'ping_id':msg['ping_id']}
			io.emit('ping_response', rsp);
			spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(rsp) + "\"", "ping_response"]);
		} else {
			spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "ping_request"]);
			io.emit('ping_request', msg);			
		}
	});
	socket.on('ping_response', function(msg) {
		//console.log(msg);
		spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "ping_response"]);
		io.emit('ping_response', msg);			
	});
	socket.on('properties', function(msg){
		if (msg.name === undefined){return;}
		if (msg.action === undefined) {msg.action = 'get';}
		
		if (msg.name == 'station_callsign'){
			if (msg.action == 'get'){
				io.emit('properties', {'name':'station_callsign','action':'rep','value':safe_execute(get_station_callsign)});
			}
		}
		
	});
	
});

sub.on("message", function(topic, msg) {
	var topic = topic.toString()
	var str = msg.toString().split(String.fromCharCode(0)).join("")	
	str = str.substr(1,str.length-2); //strip off the quotes
	//console.log(str)
	//console.log(":" + topic + ":");
	if (topic.startsWith("location")){
		io.emit('location', JSON.parse(str));
	}
	else if (topic.startsWith("newMessage")){
		io.emit('newMessage', JSON.parse(str));
	}
	else if (topic.startsWith("waypoint")){
		io.emit('waypoint', JSON.parse(str));
	} 
	else if (topic.startsWith("ping_request")){
		msg = JSON.parse(str);
		if (msg['dst'] == safe_execute(get_station_callsign)){
			console.log('ping response sent to ' + msg['src']);
			var rsp = {'src':safe_execute(get_station_callsign),'dst':msg['src'],'ping_id':msg['ping_id']}
			spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(rsp) + "\"", "ping_response"]);
		} else {
			io.emit('ping_request', msg);			
		}
	}
	else if (topic.startsWith("ping_response")){
		io.emit('ping_response', JSON.parse(str));
	}
	else {
		console.log("message received on unknown topic " + topic);
		console.log(str)
	}	
});

var intervalId = setInterval(function() {
	callsign = safe_execute(get_station_callsign);
	if (callsign === null){return;}
	gps_location = safe_execute(get_location);
	if (gps_location === null) {
		gps_location = [0,0]; //set to dummy coordinates
	}
	msg = {'username':callsign, 'coords':gps_location,'type':'station'};
	//console.log(msg);
	spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "location"]);
	io.emit('location', msg);
}, 15000);

//server.listen(8085);
server.listen(443);
io.listen(server);

const express = require('express');
const httpApp = express();

httpApp.get("*", function(req, res, next) {
    res.redirect("https://" + req.headers.host + req.path);
});

http.createServer(httpApp).listen(80, function() {
    console.log("Express TTP server listening on port 80");
});
