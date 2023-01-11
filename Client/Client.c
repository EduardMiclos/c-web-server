#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define PORT_ID 8080
#define MAX_MSG_lEN 1024

#define ERR_HANDLE(msg) {printf(msg); exit(EXIT_FAILURE);}
#define SOCKET_ERR_HANDLE ERR_HANDLE("Error when trying to create a socket.");
#define CONNECT_ERR_HANDLE {printf("Connection error: %s\n", strerror(errno));}
#define SERVER_CONVERT_ERR_HANDLE ERR_HANDLE("Error when trying to convert IP Address to binary format!\n");

int main() {
    /** Socket file descriptor. */
    int sockfd;

    /** Client file descriptor. */
    int sockfd_2;

    struct sockaddr_in server_address;
    char *msg = "GET /index.html HTTP/1.1";
    char msgbuff[1024] = {0};
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        SOCKET_ERR_HANDLE;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT_ID);

    /** Converting the IPv4 and IPv6 addresses from text to binary format. */
    if (inet_pton(AF_INET, "192.168.0.194", &server_address.sin_addr) <= 0) 
        SERVER_CONVERT_ERR_HANDLE;

    if ((sockfd_2 = connect(sockfd, (struct sockaddr*) & server_address, sizeof(server_address))) < 0)
        CONNECT_ERR_HANDLE;

    send(sockfd, msg, strlen(msg), 0);
    printf("Message from client sent successfully!\n\n");

    sleep(5);
    recv(sockfd, msgbuff, sizeof(msgbuff), 0);
    printf("%s", msgbuff);
	
    close(sockfd_2);

    return 0;
}
