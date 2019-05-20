Welcome to the proxy server program. This file contains info on how to run the simulations and generate data. All simulations were run on the NYU crunchy1.cims.nyu.edu	server. 

The 'links' directory is where you may place files containing url requests.

To run the proxy server on many client requests (urls), we call the server_sim file with a single file input containing url links line by line

Compilation instructions:
    Requires pthread, cURL

    gcc -g -O0 -std=gnu99 -lcurl -pthread server_sim.c threadpool.c PMurHash.c -o server_sim
    
    The input is an integer number of threads and a filename containing urls, one per line, for the server to retrieve. 
    It can be run like so:

    ./server_sim num_threads urls_file_path



To run the server on multiple url files and collect performance stats, we compile and run the server_sim_bench file. Each file containing links must be named like 'num_links.txt' where num_links is the integer number of urls in the file

Compilation instructions:
    Requires pthread, cURL

    gcc -g -O0 -std=gnu99 -lcurl -pthread server_sim_bench.c threadpool.c PMurHash.c -o server_sim_bench
    
    This program generates performance stats for the proxy server using different input sizes
    and thread counts of 2,4,8,16,32,64
    It can be run like so:

    ./server_sim_bench 343.txt 23423.txt 234.txt



The 'network' directory contains a real implementation of the server using sockets. It accepts new connections from clients and launches a new thread for each client. It then sends back the http reponse over the socket. This file should NOT BE RUN on the crunchy server as it requires accepting incoming connections, uses mysql for its cache, requires port forwarding on the network router, and requires the 'bcat' html rendering utility. Nevertheless, instructions for operating it are included below:

Compilation instructions for proxy_server.c
    Requires mysql, cURL

    gcc -Wall -g -O0 -lcurl -pthread proxy_server.c -o proxy_server `mysql_config --cflags --libs`


Compilation instructions for proxy_client.c

    Need to install 'bcat' terminal utility to render html in browser

    sudo gem install bcat

    gcc -Wall -g -O0 proxy_client.c -o proxy_client



Note that when running the simulations, you may see messages like:

	curl_easy_perform() failed: Timeout was reached
	bad url https://www.youtube.com/user/ScreenRant

This occurs when a link is expired, redirects in a loop, or that website has blocked curl requests. The proxy server imposes a timeout on curl requests that take longer than 2 sec. 




To gather performance stats for the python asyncio library, run asyncio_bench.py like so :

	python3 asyncio_bench.py



To run both asyncio and server_sim_bench files, run the whole simulation like so:

	./run_sim.sh

Make sure to run all src files in the project directory itself




