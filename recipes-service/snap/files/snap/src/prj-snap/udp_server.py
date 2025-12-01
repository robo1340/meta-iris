# We will need the following module to generate randomized lost packets
import socket

# Create a UDP socket
# Notice the use of SOCK_DGRAM for UDP packets
serverSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Assign IP address and port number to socket
serverSocket.bind(('', 1337))

while True:
	# Receive the client packet along with the address it is coming from
	message, address = serverSocket.recvfrom(1024)

	print('received: %s from %s' % (message,address))

	# Otherwise, the server responds
	serverSocket.sendto(message, address) 
	print('response sent to %s' % (address,))
