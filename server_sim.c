#include <sys/types.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h> 
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include "threadpool.h"
#include "PMurHash.h"
#define TABLE_SIZE 2000
#define PROBE_LEN 5

/*
Compilation instructions:
    Requires pthread, cURL

    gcc -g -O0 -std=gnu99 -lcurl -pthread server_sim.c threadpool.c PMurHash.c -o server_sim
    
    The input is an integer number of threads and a filename containing urls, one per line, for the server to retrieve. 
    It can be run like so:

    ./server_sim num_threads urls_file_path

*/

struct request {
    char url[256];
};

struct entry {
    MH_UINT32 key;
    MH_UINT32 nfu_cnt;
    // char a[3] is used to misallign cache to test benefit of cache aligned hash table and linear probing
    // char a[3];
};

static int MAXTHREADS;
static struct entry* cache; 
static pthread_rwlock_t rwlock;


void http_proxy(void* request);
int load_requests(struct request* requests, char* filename);
MH_UINT32 cache_get(char* request, int num);
MH_UINT32 curl_http(char* request, int num);
void cache_put(char* request, int num);


int main(int argc, char* argv[]) {

    MAXTHREADS = atoi(argv[1]);
    curl_global_init(CURL_GLOBAL_ALL);
    struct request* requests = (struct request*) malloc(sizeof(struct request) * 1000);
    int req_cnt = load_requests(requests, argv[2]);
    cache = (struct entry*) calloc(TABLE_SIZE * PROBE_LEN, sizeof(struct entry)); 
    threadpool_t* pool = threadpool_create(MAXTHREADS, MAX_QUEUE, 1);

    pthread_rwlock_t rwlock;
    pthread_rwlock_init(&rwlock,NULL);

    for(int i = 0; i < req_cnt; i++) {
        threadpool_add(pool, http_proxy, (void*) requests[i].url, 1);
    }

    pthread_rwlock_destroy(&rwlock);
    threadpool_destroy(pool, 1);
    curl_global_cleanup();
    free(cache);
    free(requests);
}


/*
    Curl the request received by the client, or retrieve the html
    from cache. Then send this response data over the socket to client 
*/
void http_proxy(void* req) {

    MH_UINT32 ret;
    char* request = (char*) req;
    int num = strlen(request);
    request[num-1] = '\0'; 
    // Check the web cache to see if there is already an html doc for the website stored. 
    // If one exists, retrieve that and send it to client. Otherwise, make the curl request 
    if((ret = cache_get(request, num)) != 0 ) {
        // printf("retrieving %s from cache...\n", request);
    } else if((ret = curl_http(request, num)) != 0 ) {
        // printf("retrieving %s via curl...\n", request);
    }
}


/*
    Query web cache to check if page exists on proxy
    server. If it does, return the file name (i.e.the hash)
*/
MH_UINT32 cache_get(char* request, int num) {

    MH_UINT32 hash = PMurHash32(0, (void *) request, num);
    MH_UINT32 ret = 0;
    int index = hash % TABLE_SIZE;

    pthread_rwlock_rdlock(&rwlock);
    for(int i = 0; i < PROBE_LEN; i++) {
        if(cache[index + i].key == hash) {
            ret = hash;
            cache[index + i].nfu_cnt++;
            break;
        }
    }

    pthread_rwlock_unlock(&rwlock);
    return ret;
}


/*
    Insert hash(url) into cache via linear probing
    If no slots found, insert into the index of the
    least frequently used item
*/
void cache_put(char* request, int num) {

    MH_UINT32 hash = PMurHash32(0, (void *) request, num);
    MH_UINT32 min_freq = UINT_MAX;
    int min_index;
    int index = hash % TABLE_SIZE;
    int inserted = 0;

    pthread_rwlock_wrlock(&rwlock);
    for(int i = 0; i < PROBE_LEN; i++) {

        if(cache[index + i].nfu_cnt < min_freq) {
            min_freq = cache[index + i].nfu_cnt;
            min_index = index + i;
        }

        if(cache[index + i].key == 0) {
            cache[index + i].key = hash;
            cache[index + i].nfu_cnt = 1;
            inserted = 1;
            break;
        }
    }

    if(!inserted) {
        cache[min_index].key = hash;
        cache[min_index].nfu_cnt = 1;
    }

    pthread_rwlock_unlock(&rwlock);
}

/*
    Execute curl to web server to retrieve http response
    data. Then store that data in the cache for immediate retrieval
    the next time the same webpage is requested by a client

*/
MH_UINT32 curl_http(char* request, int num) {

    CURL *curl_handle;
    CURLcode res;
    char filename[256];
    MH_UINT32 fileId = PMurHash32(0, (void *) request, num);
    // Curl the request and pipe the contents to a file with an unused file name 
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, request);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36");
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, 2000L);

    sprintf(filename, "webcache/%u", fileId);

    FILE* file = fopen(filename, "w");
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file);
    res = curl_easy_perform(curl_handle);
    fclose(file);

    //check that curl executed successfully 
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
        curl_easy_cleanup(curl_handle);
        printf("bad url %s\n", request);
        return 0;
    } 
    // add url to web cache
    cache_put(request, num);

    // cleanup curl  
    curl_easy_cleanup(curl_handle);
    return fileId;
}

/*
    Load url requests from file to simulate clients
*/
int load_requests(struct request* requests, char* filename) {
    char line[256];
    int req_cnt = 0;
    FILE *file = fopen(filename, "r");
    while(fgets(line, sizeof(line), file)!=NULL) {
        strcpy(requests[req_cnt].url, line);
        req_cnt++;
    }
    return req_cnt;
}

