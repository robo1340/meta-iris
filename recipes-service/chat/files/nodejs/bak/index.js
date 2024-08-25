const express = require('express');
const app = express();
const http = require('http');
const server = http.createServer(app);
const { Server } = require("socket.io");
const io = new Server(server);

app.get('/', (req, res) => {
  res.sendFile(__dirname + '/index.html');
});

io.on('connection', (socket) => {
  console.log('A user connected');
    socket.on('disconnect', () => {
        console.log('A user disconnected');
     });

    socket.on('message', (data) => {
       // sends the data to everyone connected to the server
       socket.emit("response", data)
    });
});

server.listen(3000, () => {
  console.log('listening on *:3000');
});