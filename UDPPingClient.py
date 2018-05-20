import time
import numpy as np 
import sys
from socket import *

proxy_op = 'n'
if len(sys.argv) == 1:
	print "Usage: <Server IP Address> <proxy_op (y/n)>"
	sys.exit()
elif len(sys.argv) > 2:
	proxy_op = sys.argv[2]

# Open socket to server on port 5999
server = sys.argv[1]
port = 5999
clientSocket = socket(AF_INET, SOCK_DGRAM)

times = []
sending = 0;
received = 0;
seq = 1;

# Send server a string of a's. If it responds
# back with the string of the same length and
# uppercased, then record the RTT. If the special
# proxy server option is provided, then the number of
# clients connected to the multi-threaded web proxy
# server is printed
while True:
	try:
		if proxy_op == 'y':
			clientSocket.sendto(proxy_op.encode(), (server, port))
			message, address = clientSocket.recvfrom(100)
			message = message.decode()
			print("Server %s is currently serving %s clients" % (server, message))
			sys.exit()
		start = time.time()
		message = "a" * 64
		sending += 64
		clientSocket.sendto(message.encode(), (server, port))
		message, address = clientSocket.recvfrom(100)
		message = message.decode()
		diff = time.time() - start
		if len(message) == 64 and message.isupper:
			received += 64
		diff = round(((time.time() - start) * 1000), 2)
		print("%d bytes from %s  seq = %d  time = %f ms" % (len(message), server, seq, diff)) 
		times.append(diff)
		seq += 1
		time.sleep(1)
	except KeyboardInterrupt:
		break

# Upon user termination of the connection, print out the
# statistics for the list of times corresponding to Rtt's for
# the iterations of pings.
times = np.array(times)
minimum = round(min(times), 2)
maximum = round(max(times), 2)
avg = round(np.mean(times),2)
mdev = round(np.std(times),2)
loss = round(((sending - received) / sending) * 100, 2)
print("\n\n%d bytes transmitted, %d bytes received, %d percent byte loss\n" \
	% (sending, received, loss))
print("rtt min/avg/max/mdev = %f / %f / %f / %f ms\n\n" \
	% (minimum, avg, maximum, mdev))
