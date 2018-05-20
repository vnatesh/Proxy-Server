import os
import sys
import re
from socket import *

# Bind the socket to port 5999
port = 5999
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.bind(('', port))

# Listen for clients and handle them sequentially 
# (as opposed to async multithreaded). If the special proxy
# server option is read from a client, then send back the
# number of clients being handled by the proxy server. This
# is based on the number of threads the proxy server process
# currently owns 
while True:
	message, clientAddress = serverSocket.recvfrom(100)
	if message.decode() == 'y':
		proxyClients = os.popen('pid=$(pgrep proxy_server); cat /proc/$pid/status | grep Threads').read()
		proxyClients = ''.join(re.findall(r'\d+', proxyClients))
		proxyClients = str(int(proxyClients) - 1)
		serverSocket.sendto(proxyClients.encode(), clientAddress) 
		sys.exit()
	message = message.decode().upper()
	serverSocket.sendto(message.encode(), clientAddress) 
