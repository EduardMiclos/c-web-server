#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>

#define SERVER "raspberry-miclos/0.1"
#define SERVER_PROTOCOL "HTTP/2.0"

#define PORT_ID 8080
#define HEADER_LEN 512
#define MSG_MAX_LEN 2048
#define BACKLOG 10

#define SOCKET_SETUP_SUCCESS_MSG {perror("Setting up the socket...\n");}
#define SOCK_ADDR_SETUP_MSG {printf("Setting up ssock_addr [AF_INET, INADDR_ANY]...\n");}
#define SERVER_START_MSG {printf("Server started on port: %d\n", PORT_ID);}

#define ERR_HANDLE(msg) {printf(msg); exit(EXIT_FAILURE);}
#define SOCKET_ERR_HANDLE ERR_HANDLE("Error when trying to create a socket.\n")
#define SOCKET_BEHAVIOUR_ERR_HANDLE ERR_HANDLE("Error when trying to establish options for connection socket.\n")
#define BIND_ERR_HANDLE ERR_HANDLE("Binding file descriptor on port failed!\n")
#define LISTEN_ERR_HANDLE ERR_HANDLE("Server listening failed!\n")
#define CONNECT_ERR_HANDLE ERR_HANDLE("Connection not accepted!\n")

typedef struct {
	/** Examples of codes and messages:
	 * 	200 - OK
	 * 	404 - NOT FOUND
	 * 	500 - SERVER ERROR 
	 * 	etc.
	 */
	char *code;
	char *msg;
} status_t;


typedef struct {
		/** HTTP Version. */
	char 	*prot_v,
		
		*server,

		/** Examples of content types:
		 * 	text/html; charset = UTF-8
		 * 	application/json;
		 * 	etc.
		 */
		*content_type,

		*last_modified;

	int content_length;


	status_t status;
} header_t;


typedef struct {
	header_t header;

	char *content;
} response_t;


char* response_createheader(response_t response) {
	char *header = (char*)malloc(sizeof(char) * HEADER_LEN);

	snprintf(header, 
		 HEADER_LEN, 
		 "%s %s %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nLast-Modified: %s\r\nServer: %s\r\n\r\n",
		 response.header.prot_v, response.header.status.code, response.header.status.msg,
		 response.header.content_type, 
		 response.header.content_length,
		 response.header.last_modified,
		 response.header.server);

	return header;
}

void response_init(response_t *response) {
	(*response).header.prot_v = SERVER_PROTOCOL;
	(*response).header.server = SERVER;
	(*response).header.content_type = "text/html; charset=UTF-8";
	(*response).header.content_length = 0;
	(*response).header.last_modified = "";

	(*response).header.status.code = "200";
	(*response).header.status.msg = "OK";
}

int get_file_size(FILE *fptr) {
	fseek(fptr, 0, SEEK_END);
	int size = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	return size;
}

char *get_file_name(char *http_request) {
	char *token = strtok(http_request, " ");
	token = strtok(NULL, " ");

	return token;
}

char *get_file_content(FILE *fptr, int file_size) {
	char *content = (char *) malloc(sizeof(char) * (file_size + 1));
	fread(content, file_size, 1, fptr);

	return content;
}

int main() {
	/** Socket 1 file descriptor. */
	int sockfd;

	/** Socket 2 file descriptor. This socket is associated with the connection. */
	int sockfd_2;

	/** Message buffer. This is where we store the messages we receive from the client side. */
	char client_msg[MSG_MAX_LEN] = {0};

	/** Local socket address, describing the characteristics of our server: family, port etc.. */
	struct sockaddr_in local_addr;
	socklen_t local_addr_len = sizeof(local_addr);

	/** Remote socket address, telling us information about the client. */
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(remote_addr);

	int option = 1;

	/** Creating a connection-oriented socket with internet protocol. */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		SOCKET_ERR_HANDLE;

	SOCKET_SETUP_SUCCESS_MSG;

	/** Forcefully attaching socket to the port 8080. */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)))
		SOCKET_BEHAVIOUR_ERR_HANDLE;	

	/** Setting up the values of the local_addr struct. */
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(PORT_ID);

	SOCK_ADDR_SETUP_MSG;

	local_addr_len = sizeof(local_addr);
	if (bind(sockfd, (struct sockaddr*) &local_addr, local_addr_len) < 0)
		BIND_ERR_HANDLE;

	SERVER_START_MSG;


	/** Preparing the response to the client. */
	response_t response;
	response_init(&response);
		

	/** The server is now ready to accept requests. */
	if (listen(sockfd, BACKLOG) < 0)
		LISTEN_ERR_HANDLE;

	if ((sockfd_2 = accept(sockfd, (struct sockaddr*) &remote_addr, &remote_addr_len)) < 0)
		CONNECT_ERR_HANDLE;
	
	/** Reading the message sent from the client. */	
	recv(sockfd_2, &client_msg, MSG_MAX_LEN, 0);	


	char *file_name = get_file_name(client_msg);
	char httdocs[255] = "./httdocs/";
	
	if (access(strcat(httdocs, file_name), F_OK) == 0) {
		printf("OK");
	}
	else {
		FILE *fptr = fopen("httdocs/err.html", "r");
		
		int file_size = get_file_size(fptr);
		char *content = get_file_content(fptr, file_size);

		fclose(fptr);

		response.header.status.code = "404";
		response.header.status.msg = "Not Found";
		response.header.content_length = file_size;
		response.content = response_createheader(response);

		response.content = (char*)realloc(response.content, sizeof(char) * (HEADER_LEN + file_size + 1));
		response.content = strcat(response.content, content);


		send(sockfd_2, response.content, strlen(response.content) - 1, 0);

	}
	
	/** Closing the connection socket. */
	close(sockfd_2);

	/** Closing the listening socket. */
	shutdown(sockfd, SHUT_RDWR);

	return 0;
}
