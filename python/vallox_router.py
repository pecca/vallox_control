import threading
import socket
import time
import sys
import json
import logging
import requests

VALLOX_CONTROL_UDP_PORT = 8056
VALLOX_ROUTER_TCP_PORT = 8093

DATABASE = 'vallox'

INFLUXDB_SERVER_IP = '52.226.78.29'

udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('', VALLOX_ROUTER_TCP_PORT)

logging.basicConfig(filename='vallox_router.log',level=logging.DEBUG)

logging.info('starting up on port %s' % VALLOX_ROUTER_TCP_PORT)

tcp_sock.bind(server_address)

DIGIT_VARS = "digit_vars"
CONTROL_VARS = "control_vars"
DS18B20_VARS = "ds18b20_vars"

def decode_json(json_str):
    ret = {}
    if len(json_str) > 0 and json_str.count('{') == json_str.count('}'):
        try:
            ret = json.loads(json_str)
            return ret
        except Exception as e:
            logging.error(e)
            return ret
    else:
        logging.error("parenthesis are incorrect: " + json_str)
        return ret

def tcp_recv(socket):
    msgOk = False
    recv_msg = ''
    while not msgOk:
        try:
            recv_msg += socket.recv(8000)
        except Exception as e:
            logging.error(e)
            return ""
        if len(recv_msg) > 0 and recv_msg.count('{') == recv_msg.count('}'):
            msgOk = True
    return recv_msg
    
    
class UDPClientThread(threading.Thread):
    def __init__(self):
        super(UDPClientThread, self).__init__()
        self.kill_received = False
        self.vars = {}
        self.vars[DIGIT_VARS] = {}
        self.vars[CONTROL_VARS] = {}
        self.vars[DS18B20_VARS] = {}
        
    def get_vars_from_vallox_ctrl(self, type):
        send_msg = '{"get":"%s"}' % (type)
        try:
            udp_socket.sendto(send_msg, ("localhost",VALLOX_CONTROL_UDP_PORT))
            recv_msg, addr = udp_socket.recvfrom(8000)
            self.vars[type] = decode_json(recv_msg)
        except Exception as e:
            logging.error(e)   
        

    def get_vars(self, type):
        return self.vars[type]

    def update_vars_to_database(self):
        var_list = []
        for type in self.vars:
            for group in self.vars[type]:
                for var in self.vars[type][group]:
                    var_obj = self.vars[type][group][var]
                    value = var_obj['value']
                    ts = var_obj['ts']    
                    if isinstance(value, int) or isinstance(value, float):
                        influxdb_var = {'name' : var, 'columns' : ['val'], 'points' : [[value]]}
                        var_list.append(influxdb_var)
        try:
            r = requests.post('http://%s:8086/db/%s/series?u=root&p=root' % (INFLUXDB_SERVER_IP, DATABASE), data=json.dumps(var_list))
            if r.status_code != 200:
                print 'Failed to add point to influxdb --aborting.'
        except Exception as e:
            print str(e)

    
    def run(self):
        while(not self.kill_received):
            self.get_vars_from_vallox_ctrl(DIGIT_VARS)
            time.sleep(2)
            self.get_vars_from_vallox_ctrl(CONTROL_VARS)
            time.sleep(2)            
            self.get_vars_from_vallox_ctrl(DS18B20_VARS)
            time.sleep(10)
            self.update_vars_to_database()

udp_thread = UDPClientThread()
udp_thread.daemon = True
try:
    udp_thread.start()
except (KeyboardInterrupt, SystemExit):
    sys.exit()
        
# Listen for incoming connections
tcp_sock.listen(1)

responseCnt = 0

while True:
    # Wait for a connection
    logging.info('waiting for a connection')
    
    tcp_connection, client_address = tcp_sock.accept()
    try:
        logging.info('connection from ' + str(client_address))
        while True:
            msg_recv = tcp_recv(tcp_connection)
            get_var = decode_json(msg_recv)
            if (get_var != {}):
                send_msg = json.dumps(udp_thread.get_vars(get_var['get']))
                tcp_connection.sendall(send_msg)
                responseCnt += 1
            else:
                logging.info('empty message received') 
                time.sleep(5)
            if not responseCnt % 50:
                logging.info('responseCnt' + str(responseCnt))
            
    except:
        tcp_connection.close()
        continue
            
    finally:
        # Clean up the connection
        tcp_connection.close()

try:
    udp_thread.join()
except KeyboardInterrupt:    
    udp_thread.kill_received = True    
