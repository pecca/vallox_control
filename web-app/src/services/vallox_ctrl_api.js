const IP = window.location.hostname;
const PORT = 3005;
const URL = 'ws://' + IP + ':' + PORT;
const TIMEOUT = 4000;

//const ws = new WebSocket(URL);

export function readVars(type) {
  return new Promise(function (resolve, reject) {
    const ws = new WebSocket(URL);
    let timeout = setTimeout(function() {
      reject('timeout');
    }, TIMEOUT);

    ws.onopen = () => {
      const msg =  {
          get: type
      };
      ws.send(JSON.stringify(msg));
      ws.onmessage = (e) => {  // a message was received
        const recvMsg = JSON.parse(e.data);
        clearTimeout(timeout);
        resolve(recvMsg)
      }
    }
  });
}

export function writeVar(type, name, value) {
  let ws = new WebSocket(URL);
  ws.onopen = () => {
    const msg =  {
        set: {}
    };
    msg.set[type][name] = value;
    ws.send(JSON.stringify(msg));
  }
}
