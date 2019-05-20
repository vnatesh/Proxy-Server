import matplotlib.mlab as mlab
import re


with open('out.txt','r') as f:
	a = f.read().split('\n\n')

a = [i.strip() for i in a][1:]
times = {}

for i in xrange(0,len(a)-1,2): 
	times[int(''.join(re.findall(r'\d+',a[i])))] = map(float, a[i+1].split(' '))

speedup = {i : [times[i][0] / j for j in times[i]] for i in times.keys()}
efficiency = {i : [speedup[i][j] / (2**j) for j in xrange(len(speedup[i])) ] for i in speedup.keys()}

print '\n\nProxy Server Perf. Metrics For num requests ' + str(times.keys()) + ' num_threads ' + str([1,2,4,8,16,32,64])
print
print 'Times = ' + str(times)
print
print 'Speedup = ' + str(speedup)
print
print 'Efficiency = ' + str(efficiency) + '\n\n'
#
#
### Uncomment below code to plot speedup and efficiency using matplotlib
#
#
# import matplotlib.pyplot as plt
# #
# plt.title("pThread proxy_server : Speedup vs Num_threads")
# plt.xlabel("Num Threads")
# plt.ylabel("Speedup T_ser / T_par")
# plt.tight_layout()
# for i in speedup.keys():
# 	plt.plot([2**j for j in xrange(len(speedup[i]))], speedup[i], label = "%d requests" % i, marker='o' )

# plt.legend()
# plt.show()

# plt.title("pThread proxy_server : Efficiency vs Num_threads")
# plt.xlabel("num threads")
# plt.ylabel("Efficiency (speedup / num_threads")
# plt.tight_layout()
# for i in efficiency.keys():
# 	plt.plot([2**j for j in xrange(len(efficiency[i]))], efficiency[i], label = "%d requests" % i, marker='o' )

# plt.legend()
# plt.show()

# plt.title("pThread proxy_server : Throughput vs Num_threads")
# plt.xlabel("num threads")
# plt.ylabel("requests/sec")
# plt.tight_layout()
# for i in times.keys():
# 	plt.plot([2**j for j in xrange(len(times[i]))], [i / j for j in times[i]], label = "%d requests" % i, marker='o' )

# plt.legend()
# plt.show()





############### Testing ####################

# # performance stats from running server_sim_bench.c on different requests batches of size times.keys()
# times = {164 : [73.497039, 27.685659, 13.557274, 7.063820, 4.807376, 2.932753, 2.282000], 328 : [129.737858, 54.527835, 28.676522, 14.408415, 7.478860, 4.795594, 3.831685], 656 : [246.100522, 111.959832, 52.359419, 25.071188, 14.594710, 7.508580, 5.270612]}
# speedup = {i : [times[i][0] / j for j in times[i]] for i in times.keys()}
# efficiency = {i : [speedup[i][j] / (2**j) for j in xrange(len(speedup[i])) ] for i in speedup.keys()}


# # asyncio requests/sec for different number of threads.
# times1 = {328: [70.05380368232727, 34.99897837638855, 20.59834623336792, 10.606179237365723, 8.270125389099121, 9.750989198684692, 9.176082611083984], 656: [166.1765537261963, 78.84569048881531, 39.40619254112244, 22.186023235321045, 16.550795078277588, 16.69521999359131, 17.034363269805908], 164: [37.40655970573425, 18.508413553237915, 9.639797925949097, 5.500182628631592, 4.2726616859436035, 4.396047592163086, 4.574560880661011], 1312: [304.3635308742523, 156.5487358570099, 80.70913887023926, 42.1446635723114, 31.10176992416382]}


# plt.title("Asyncio : Throughput vs Num_threads")
# plt.xlabel("Num Threads")
# plt.ylabel("requests/sec")
# plt.tight_layout()
# for i in times1.keys():
# 	plt.plot([2**j for j in xrange(len(times1[i]))], [i / j for j in times1[i]], label = "%d requests" % i, marker='o' )

# plt.legend()
# plt.show()

