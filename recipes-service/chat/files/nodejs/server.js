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
		//console.log('new location for ' + msg.username + ': ' + msg.coords)
		spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "location"]);
		io.emit('location', msg);
	});
	socket.on('waypoint', function(msg) {
		//console.log('new location for ' + msg.username + ': ' + msg.coords)
		spawn('/bridge/src/prj-zmq-send/test',["\"" + JSON.stringify(msg) + "\"", "waypoint"]);
		io.emit('waypoint', msg);
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
	} else {
		console.log("message received on unknown topic " + topic);
		console.log(str)
	}	
});

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
