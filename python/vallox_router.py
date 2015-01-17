import threading
import socket
import time
import sys

udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('', 8092)
print >>sys.stderr, 'starting up on %s port %s' % server_address
tcp_sock.bind(server_address)

digit_vars_str = "";
control_vars_str = "";
ds18b20_vars_str = "";

class UDPClientThread(threading.Thread):
     def __init__(self):
         super(UDPClientThread, self).__init__()
         self.kill_received = False

     def run(self):
        global digit_vars_str, control_vars_str, ds18b20_vars_str
        while(not self.kill_received):
            data = '{"get":"digit_vars"}'
            udp_socket.sendto(data, ("localhost",8056))
            digit_vars_str, addr = udp_socket.recvfrom(8000)
            time.sleep(1)
            
            data = '{"get":"control_vars"}'
            udp_socket.sendto(data, ("localhost",8056))
            control_vars_str, addr = udp_socket.recvfrom(8000)
            time.sleep(1)
            
            data = '{"get":"ds18b20_vars"}'
            udp_socket.sendto(data, ("localhost",8056))
            ds18b20_vars_str, addr = udp_socket.recvfrom(8000)
            time.sleep(10)


udp_thread = UDPClientThread()
udp_thread.daemon = True
try:
    udp_thread.start()
except (KeyboardInterrupt, SystemExit):
    sys.exit()
        
# Listen for incoming connections
tcp_sock.listen(1)

while True:
    # Wait for a connection
    print >>sys.stderr, 'waiting for a connection'
    connection, client_address = tcp_sock.accept()
    try:
        print >>sys.stderr, 'connection from', client_address

        while True:
            data = connection.recv(8000)
            print data
            if data.find("digit_vars") > -1:
                response_data = digit_vars_str
            elif data.find("control_vars") > -1:          
                response_data = control_vars_str
            elif data.find("ds18b20_vars") > -1:
                response_data = ds18b20_vars_str
            else:
                response_data = "no data"
            
            connection.sendall(response_data)
            #print response_data
    except:
        connection.close()
        continue
            
    finally:
        # Clean up the connection
        connection.close()

try:
    udp_thread.join()
except KeyboardInterrupt:    
    udp_thread.kill_received = True    
