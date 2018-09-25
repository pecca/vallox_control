var express = require('express');
var router = express.Router();

var Buffer = require('buffer').Buffer;
var dgram = require('dgram');

var VALLOX_CONTROL_IP = process.env.VALLOX_CONTROL_IP;
var VALLOX_CONTROL_PORT = process.env.VALLOX_CONTROL_PORT;
var TOKEN = process.env.TOKEN;

const UDP_TIMEOUT = 10000;

const sendReceiveMessage = (messageSend) => new Promise((resolve, reject) => {
  const message = Buffer.from(JSON.stringify(messageSend));
  const client = dgram.createSocket('udp4');
  client.bind(VALLOX_CONTROL_PORT);
  client.send(message, VALLOX_CONTROL_PORT, VALLOX_CONTROL_IP, (err) => {});
  const receiveTimeout = setTimeout(() => {
    client.close();
    reject({error: "device not responding"});
  }, UDP_TIMEOUT);
  client.once('message', (msg, rinfo) => {
    client.close();
    clearTimeout(receiveTimeout);
    resolve(JSON.parse(msg.toString()));
  });
});

/* GET vallox control */
router.get('/', function(req, res, next) {
  const action = req.query.action;
  let message;
  const token = req.query.token;
  if (action === "get") {
    const type = req.query.type;
    message = {
      id: 0,
      get: type
    };
  } else if (action === "set") {
    const type = req.query.type;
    const variable = req.query.variable;
    const value = req.query.value;
    message =  {
      id: 0,
      set: {
        [type]: {
          [variable]: Number(value)
        } 
      }
    };
  } else {
    res.status(500).json({error: "invalid URL parameters"});
  }
  if (token !== TOKEN) {
    res.status(404).json({error: "not authorized"});
  } else if (message) {
    sendReceiveMessage(message).then((value) => {
      res.json(value);
    }).catch((error) => {
      res.status(500).json(error);
    })
  }
});

module.exports = router;
