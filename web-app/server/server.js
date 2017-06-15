var Buffer = require('buffer').Buffer;
var dgram = require('dgram');
var WebSocketServer = require('ws').Server;
var wss = new WebSocketServer({port: 3005});

var SERVER_IP = '88.113.210.216'
var SERVER_PORT = 8056

var udpClient = dgram.createSocket('udp4');
udpClient.bind(SERVER_PORT);

wss.on('connection', function(ws) {

    //When a message is received from udp server send it to the ws client
    udpClient.once('message', function(msg, rinfo) {
      try {
        ws.send(msg.toString());
      } catch (e) {}
    });

    //When a message is received from ws client send it to udp server.
    ws.once('message', function(message) {
      var msgBuff = new Buffer(message);
      udpClient.send(msgBuff, 0, msgBuff.length, SERVER_PORT, SERVER_IP);
    });
});
