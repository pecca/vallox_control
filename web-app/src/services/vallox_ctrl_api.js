const IP = window.location.hostname;
const PORT = 3005;
const URL = 'ws://' + IP + ':' + PORT;
const TIMEOUT = 2000;

//const ws = new WebSocket(URL);

export function readDigitVars() {
  return new Promise(function (resolve, reject) {
    const ws = new WebSocket(URL);
    let timeout = setTimeout(function() {
      console.log('GetListOfContentFilesResp timeout');
      reject('timeout');
    }, TIMEOUT);

    ws.onopen = () => {
      const msg =  {
          get: 'digit_vars'
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

export function writeDigitVar(name, value) {
  let ws = new WebSocket(URL);
  ws.onopen = () => {
    const msg =  {
      set: {
        digit_var: {}
      }
    };
    msg.set.digit_var[name] = value;
    ws.send(JSON.stringify(msg));
  }
}
