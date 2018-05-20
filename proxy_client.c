#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h> 

#define PORT 3490 // this port doesn't seem to be used by another application
#define MAXSIZE 1024

/*

Compilation instructions:

    Need to install 'bcat' terminal utility to render html in browser

    sudo gem install bcat

    gcc -Wall -g -O0 proxy_client.c -o proxy_client
*/

int recv_timeout(int socket_fd , int timeout, int htmlFile);


int main(int argc, char *argv[]) {


    struct sockaddr_in server_info;
    int socket_fd = 0;
    int bytes_read = 0;
    char request[1024];

    if (argc != 2) {
        fprintf(stderr, "Usage: <server IP address> \n");
        exit(1);
    }

    // create TCP socket file descriptor
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
        fprintf(stderr, "Error: Could not create socket\n");
        exit(1);
    }

    // set memory for a socket address struct. It contains parameters
    // for IP address, port, family (AF_INET) 
    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(PORT);

    // get IP address of server from user input and set the ip address parameter
    // in the sockadrr struct server_info
    if(inet_pton(AF_INET, argv[1], &server_info.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    // socket_fd: Specifies the file descriptor associated with the socket.
    // server_info: Points to a sockaddr structure containing the peer address. The length and format of the address depend on the address family of the socket.
    // address_len: Specifies the length of the sockaddr structure pointed to by the address argument.

    // create TCP connection between client socket_fd and server socket server_info
    if (connect(socket_fd, (struct sockaddr*) &server_info, sizeof(struct sockaddr))<0) {
        fprintf(stderr, "Connection Failure\n");
        perror("connect");
        exit(1);
    }

    while(1) {

        struct timeval start, end;

        memset(request, '0', sizeof(request));
        printf("Enter website url: \n");
        fgets(request,MAXSIZE-1,stdin);

        gettimeofday(&start, NULL);
        int ret = send(socket_fd,request, strlen(request),0); 
        if (ret == -1) {

            fprintf(stderr, "Failure Sending Message\n");
            close(socket_fd);
            exit(1);

        } else {

            printf("HTTP Request sent to proxy server: %s\n",request);

            char fileName[] = "/tmp/htmlXXXXXX";
            int htmlFile = mkstemp(fileName);

            bytes_read = recv_timeout(socket_fd, 1, htmlFile);
            // bytes_read = recv_timeout(socket_fd, htmlFile);
            if ( bytes_read <= 0 ) {
                printf("Either Connection Closed or Error\n");
            }

            gettimeofday(&end, NULL);
            printf("Total RTT: %f seconds\n", (((end.tv_sec - start.tv_sec) * 1000000L
                + end.tv_usec) - start.tv_usec) / (1000000.0));
            printf("Response received from proxy server. Rendering webpage...\n");
            char command[100];
            strcpy(command, "cat ");
            strcat(command, fileName);
            strcat(command, " | bcat"); 
            system(command);
           }
    }

    close(socket_fd);
}


/*

 Function to keep receiving http response data 
 from proxy server in 1024 byte chunks and write 
 data to html file, until the total wait time for a
 new chunk has exceeded the set timeout
 
 */

int recv_timeout(int socket_fd , int timeout, int htmlFile) {
    int size_recv;
    int total_size = 0;
    struct timeval begin;
    struct timeval now;
    char response[MAXSIZE];
    double timediff;
    
    // Make socket non blocking so we can serve multiple 
    // asynchronous connections without I/O blocking
    fcntl(socket_fd, F_SETFL, O_NONBLOCK);
     
    //start time
    gettimeofday(&begin , NULL);
     
    while(1) {

        gettimeofday(&now , NULL);
        //time elapsed in seconds
        timediff = (now.tv_sec - begin.tv_sec) + 1e-6 * (now.tv_usec - begin.tv_usec);
         
        // If some data has been received but the we've exceeded the timeout, then
        // stop receiving new data
        if( total_size > 0 && timediff > timeout ) {
            break;
        // If no data was received at all, then wait 2*timeout longer
        } else if(timediff > timeout*2) {
            break;
        }
         
        memset(response ,0 , MAXSIZE);  // clear the response buffer to accept new data from server
        if((size_recv =  recv(socket_fd , response , MAXSIZE , 0) ) < 0) {
            // If nothing received wait another 0.1 sec
            usleep(100000);
        } else {
            write(htmlFile, response, size_recv); 
            total_size += size_recv;
            gettimeofday(&begin , NULL);
        }
    }
    
    printf("\n\ntotal bytes received: %d\n\n", total_size);
    return total_size;

}