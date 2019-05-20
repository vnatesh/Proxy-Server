import requests
import asyncio
import time
import concurrent.futures
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab


def sync_req(urls, times):
	s = time.time()
	num_requests = len(urls)
	responses = [requests.get(urls[i]) for i in range(num_requests)]
	print(time.time() - s)


async def asyncio_test(urls, maxWorkers):
    with concurrent.futures.ThreadPoolExecutor(max_workers=maxWorkers) as executor:
        loop = asyncio.get_event_loop()
        futures = [
            loop.run_in_executor(
                executor, 
                requests.get, 
                urls[i],
            )
            for i in range(len(urls))
        ]
        for response in await asyncio.gather(*futures):
        	pass


def async_req(urls, max_workers):
	s = time.time()
	loop = asyncio.get_event_loop()
	loop.run_until_complete(asyncio_test(urls, max_workers))
	print(time.time() - s)



def run_simulation():
	with open("links/links_asyncio_test.txt",'r') as f:
		urls = f.read().split('\n')
	urls = urls[:-1]
	times = {}
	# run sim over k problem sizes using 1,2,4,... threads
	for k in [8, 4, 2, 1]:
		print()
		print()
		print("problem size: %d requests" % (len(urls)/k))
		print()
		times[k] = []
		for i in [1,2,4,8,16,32,64]:
			print(i)
			s = time.time()
			async_req(urls[0:int(len(urls)/k)], i)
			times[k].append(time.time() - s)
			time.sleep(10)
	speedup = {i : [times[i][0] / j for j in times[i]] for i in times.keys()}
	efficiency = {i : [speedup[i][j] / (2**j) for j in range(len(speedup[i])) ] for i in speedup.keys()}
	print('\nPython Asyncio Perf. Metrics For num requests ' + str(times.keys()) + ' num_threads ' + str([1,2,4,8,16,32,64]))
	print()
	print('Times = ' + str(times))
	print()
	print('Speedup = ' + str(speedup))
	print()
	print('Efficiency = ' + str(efficiency) + '\n\n')
	#
	#
	#
	###### Uncomment below code to plot throughput, speedup and efficiency for different num_threads and num_requests
	#
	#
	# plt.title("Asyncio : Speedup vs Num_threads")
	# plt.xlabel("Num Threads")
	# plt.ylabel("Speedup T_ser / T_par")
	# plt.tight_layout()
	# for i in speedup.keys():
	# 	plt.plot([2**j for j in range(len(speedup[i]))], speedup[i], label = "%d requests" % i, marker='o' )
	# plt.legend()
	# plt.show()
	# plt.title("Asyncio : Efficiency vs Num_threads")
	# plt.xlabel("num threads")
	# plt.ylabel("Efficiency (speedup / num_threads")
	# plt.tight_layout()
	# for i in efficiency.keys():
	# 	plt.plot([2**j for j in range(len(efficiency[i]))], efficiency[i], label = "%d requests" % i, marker='o' )
	# plt.legend()
	# plt.show()
	# plt.title("Asyncio : Throughput vs Num_threads")
	# plt.xlabel("Num Threads")
	# plt.ylabel("requests/sec")
	# plt.tight_layout()
	# for i in times.keys():
	# 	plt.plot([2**j for j in range(len(times[i]))], [i / j for j in times[i]], label = "%d requests" % i, marker='o' )
	# plt.legend()
	# plt.show()

if __name__ == '__main__':
	run_simulation()



