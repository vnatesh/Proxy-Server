#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> 
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h> 
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <mysql/mysql.h>

#define PORT 3490
#define BACKLOG 10
#define MAXSIZE 1024
#define MAXTHREADS 100

/*

Compilation instructions:
    Requires mysql, cURL

    gcc -Wall -g -O0 -lcurl -pthread proxy_server.c -o proxy_server `mysql_config --cflags --libs`

*/

void* http_proxy(void* client_fd);
int send_data(int client_fd, char filename[]);
int check_cache(char request[], MYSQL *con);
int curl_http(char request[], MYSQL *con);
void finish_with_error(MYSQL *con);



int main(int argc, char *argv[]) {

    int socket_fd = 0;
    int client_fd = 0;
    struct sockaddr_in server; 
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int yes = 1;
    srand(time(NULL));


    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
        fprintf(stderr, "Socket failure!!\n");
        exit(1);
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    memset(&server, '0', sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT); 

    if ((bind(socket_fd, (struct sockaddr *) &server, sizeof(struct sockaddr )))== -1)    { //sizeof(struct sockaddr) 
        fprintf(stderr, "Binding Failure\n");
        exit(1);
    }

    if ((listen(socket_fd, BACKLOG))== -1){
        fprintf(stderr, "Listening Failure\n");
        exit(1);
    }

    curl_global_init(CURL_GLOBAL_ALL);
    pthread_t thread[100];
    int tID = 0;

    while(1) {

        tID = tID % MAXTHREADS;

        // The accept commmand waits for a new connection from client coming in
        // on socket (socket_fd)
        if ((client_fd = accept(socket_fd, (struct sockaddr*) NULL, NULL))==-1 ) {
            perror("accept");
            exit(1);
        }

        getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);
        printf("Server got connection from client %s\n", inet_ntoa(addr.sin_addr));

        // Launch client connection on separate thread so we can accept new
        // incoming connections without waiting for prevous client requests to finish
        if(pthread_create(&thread[tID], NULL, http_proxy, (void*) &client_fd) != 0) {
            printf("Thread creation failed\n");
        }

        tID++;
     }

     for(int i = 0; i < MAXTHREADS; i++) {
        pthread_join(thread[i], NULL);
     }

     curl_global_cleanup();
     close(client_fd);
}


/*
    Curl the request received by the client, or retrieve the html
    from cache. Then send this response data over the socket to client 
*/
void* http_proxy(void* client) {

    struct timeval start, end;
    struct timeval timeout;
    double diff_t;
    char request[MAXSIZE];
    char filename[100];
    int ret;
    int bytes = 0;
    int num = 0;
    int sret = 0; 
    int client_fd = *((int*) client);
    memset(filename, '0',sizeof(filename));

    fd_set readfds;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getpeername(client_fd, (struct sockaddr *)&addr, &addr_size);

    MYSQL *con = mysql_init(NULL);

    if (con == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(1);
    }  

    if (mysql_real_connect(con, "localhost", "root", "[newpassword]", "proxy_server", 0, NULL, 0) == NULL) {
        finish_with_error(con);
    }    

    while(1) {

        FD_ZERO(&readfds);
        FD_SET(client_fd, &readfds);
        timeout.tv_sec = 300;
        timeout.tv_usec = 0;
        sret = select(client_fd + 1, &readfds, NULL, NULL, &timeout);

        if(sret == 0) {
            printf("300 sec timeout exceeded. Client %s with fd = %d will be closed \n", 
                    inet_ntoa(addr.sin_addr), client_fd);
            break;
        
        } else {
            memset(request, '0',sizeof(request));
            if ((num = recv(client_fd, request, 1024, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            else if (num == 0) {
                printf("Connection closed by client %s with fd = %d \n", 
                    inet_ntoa(addr.sin_addr), client_fd);
                // continue to wait for other clients
                break;
            }
        }

        gettimeofday (&start, NULL);
        request[num-1] = '\0'; // this gets rid of the line feed/carriage return and
                               // formats the string properly for curl
        printf("Server: Request Received %s\n", request);

        // Check the web cache to see if there is already an
        // html for the website stored. If one exists, retrieve that
        // and send it to client. Otherwise, make the curl request 
        if((ret = check_cache(request, con)) != 0 ) {
            printf("retrieving data from cache...\n");
        } else if((ret = curl_http(request, con)) != 0 ) {
            printf("retrieving http response via curl...\n");
        }
        
        // memset(filename, '0',sizeof(filename));
        sprintf(filename, "/home/kas/Documents/webcache/%d", ret);
        bytes = send_data(client_fd, filename);
        gettimeofday (&end, NULL);
        diff_t = (((end.tv_sec - start.tv_sec)*1000000L
           +end.tv_usec) - start.tv_usec) / (1000000.0);
        printf("Reponse time: %f seconds\n\n", diff_t); 
        
        // insert client data into mysql table proxy_server.requests
        // timestamp, inet_ntoa(addr.sin_addr), request, bytes, diff_t
        char query[1024];
        sprintf(query, "INSERT INTO requests \
                        VALUES(0, CURRENT_TIMESTAMP(), '%s', '%s', %d, %f) ",
                        inet_ntoa(addr.sin_addr), request, bytes, diff_t);

        if (mysql_query(con, query)) {
            finish_with_error(con);
        }
    }

    mysql_close(con);
    return NULL;
}


/*
    Execute curl to web server to retrieve http response
    data. Then store that data in the cache for immediate retrieval
    the next time the same webpage is requested by a client

*/
int curl_http(char request[], MYSQL *con) {

    CURL *curl_handle;
    CURLcode res;
    char filename[100];
    int fileId = rand();
    // Curl the request and pipe the contents to a file with an unused file name 
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, request);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Safari/537.36");

    sprintf(filename, "/home/kas/Documents/webcache/%d", fileId);

    FILE* file = fopen(filename, "w");
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file);
    res = curl_easy_perform(curl_handle);
    fclose(file);

    //check that curl executed successfully 
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
        curl_easy_cleanup(curl_handle);
        return 0;
    } 

    // insert new file name and url into mysql web_cache (fileId, request)
    char query[1024];
    sprintf(query, "INSERT INTO web_cache \
                    VALUES(CURRENT_TIMESTAMP(), %d, '%s') ", fileId, request);

    if (mysql_query(con, query)) {
        finish_with_error(con);
    }

    // cleanup curl  
    curl_easy_cleanup(curl_handle);
    return fileId;
}


/*
    Read html response data from the server storage and 
    write it to the client socket in 1024 byte chunks
*/
int send_data(int client_fd, char filename[]) {

    FILE* html = fopen(filename, "r");
    int ret = 0;
    while(1) {        
        char buffer[MAXSIZE];
        int nread = fread(buffer, 1, MAXSIZE, html);
        if(nread > 0) {
            write(client_fd, buffer, nread);
            ret += nread;
        } else {
            break;
        }
    }
    printf("bytes sent to client: %d\n", ret);
    fclose(html);
    return ret;
}


/*
    Quit mysql with an error outputed to stderr
*/
void finish_with_error(MYSQL *con) {

    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);        
}


/*
    Query web cache to check if page exists on proxy
    server. If it does, return the file name
*/
int check_cache(char request[], MYSQL *con) {

    char query[1024];
    sprintf(query, "SELECT filename \
                    FROM web_cache \
                    WHERE url = '%s' ", request);
    if (mysql_query(con, query)) {
        finish_with_error(con);
    }

    MYSQL_RES *result = mysql_store_result(con);

    if (result == NULL) {
        finish_with_error(con);
    }

    MYSQL_ROW row;
    row = mysql_fetch_row(result);
    mysql_free_result(result);

    return (row != NULL ? atoi(row[0]) : 0);
}