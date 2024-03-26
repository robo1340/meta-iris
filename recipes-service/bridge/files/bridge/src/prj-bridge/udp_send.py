import socket
import time
import sys
UDP_IP = "10.1.1.254"
UDP_PORT = 1337
MESSAGE = "message:%d" % (int(time.time()),)
MESSAGE_LONG = MESSAGE*5

serverSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Assign IP address and port number to socket
serverSocket.bind(('', UDP_PORT))

print('sending %s -> %s:%s' % (MESSAGE, UDP_IP, UDP_PORT))
# Otherwise, the server responds
serverSocket.sendto(MESSAGE_LONG.encode(), (UDP_IP, UDP_PORT))
#time.sleep(0.001)
serverSocket.sendto(MESSAGE.encode(), (UDP_IP, UDP_PORT))
#time.sleep(0.001)
serverSocket.sendto(MESSAGE.encode(), (UDP_IP, UDP_PORT))
start = time.time()

while True:
	# Receive the client packet along with the address it is coming from
	message, address = serverSocket.recvfrom(1024)
	print('response received: %s from %s in %s seconds' % (message,address,time.time()-start))
	break



