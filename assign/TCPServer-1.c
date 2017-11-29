/*
 Author: Visalakshi Gopalakrishnan
 Student ID:00000976426
 COEN 233 - Computer Networks.

 Description: Server side - Reliable Transfer over a Reliable Channel

 -Server binds to arbitrary port number 8090
 -Accepts connection from client
 -Receives all data in chunks of 10 bytes
 -Client sends output file to store data
 -Writes data in chunks of 5 bytes in the out put file sent by client
 -Closes the connection when data is transferred successfully.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 8090 /*Arbitrary port number */
#define BUF_SIZE 255
#define READ_SIZE 5
#define QUEUE_SIZE 5  /*Allow 5 call holds */

void errorMessage(const char *);

int main(int argc, char *argv[]) {

	int rc;
	int sockfd, newSockfd; /* Socket Descriptor */

	struct sockaddr_in serv, client; /* Internet socket name (addr) */
	struct sockaddr_in *portptr; /* pointer to get port number */
	struct sockaddr addr; /* generic socket name (addr) */
	int clientlen = sizeof(client);
	char buffer[BUF_SIZE]; /* Buffer for out going file */

	/* Initialize Socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		errorMessage("Error, socket opening failed \n");
	} else {
		printf("Initialing Socket \n");
	}

	/*Build address structure to bind to socket */
	memset(&serv, 0, sizeof(serv));
	memset(&client, 0, sizeof(client));
	serv.sin_family = AF_INET; /* Internet Domain */
	serv.sin_port = 8090; /* System binds to port 8090 */
	serv.sin_addr.s_addr = INADDR_ANY; /* Accept any client */

	/* Binding to socket */
	if (bind(sockfd, &serv, sizeof(serv)) < 0) {
		close(sockfd);
		errorMessage("Error, Bind failed \n");
	} else {
		printf("Binding to Socket \n");
	}

	/* Mark socket as passive "listen" socket */
	if (listen(sockfd, QUEUE_SIZE) < 0) {
		errorMessage("Error, Listen failed \n");
	} else {

		printf("Server running and listening...waiting for connections. \n");
	}

	/* Socket is now set up and bound. Wait for connection and process it */
	while (1) {

		if ((newSockfd = accept(sockfd, &client, &clientlen)) < 0) {
			errorMessage("Error, accept failed \n");
		} else {
			printf("Accepting connection from client \n");
		}

		bzero(buffer, BUF_SIZE);

		/* Get the output File name from client to store data after accepting connection */
		int i = 0;
		while (1) {
			/* Read name of output file one byte at a time till newline character */
			rc = read(newSockfd, buffer + i, 1);

			if (buffer[i] == '\n') {
				buffer[i] = '\0';
				break;
			}
			i++;
		}

		printf("Got output file name from client %s \n", buffer);

		FILE *fr = fopen(buffer, "w");
		int bytesRecv = 0;

		bzero(buffer, READ_SIZE);

		if (fr == NULL) {
			errorMessage("File cannot be opened file on server.\n");
		}

		/* Getting data from client */
		while (1) {

			rc = recv(newSockfd, buffer, READ_SIZE, 0);

			if (rc < 0) {
				errorMessage("Error, reading from socket failed \n");
			}
			if (rc == 0) {
				printf("Closing connection \n");
				break;
			}

			/* Writing data to output file in chunks of 5 bytes */
			int write_file = fwrite(buffer, sizeof(char), rc, fr);

			if (write_file < rc) {
				errorMessage("File write failed on server.\n");
			}

			bytesRecv += rc;

			 printf("Data received by Server and written in chunks of 5 [%d] bytes - [%s] \n",
					bytesRecv, buffer);

			bzero(buffer, READ_SIZE);
		}

		printf("Received total of [%d] bytes from client \n", bytesRecv);

		fclose(fr);

		printf("Closing newSockfd \n");
		close(newSockfd);

	}

	/* Close connection after data received*/
	close(sockfd);
	return 0;
}

/* Print Error message and exit */
void errorMessage(const char *message) {

	perror(message);
	exit(1);

}
