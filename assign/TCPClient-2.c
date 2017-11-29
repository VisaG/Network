/*
 Author: Visalakshi Gopalakrishnan
 Student ID:00000976426
 COEN 233 - Computer Networks.

 Description:Client side - Reliable Transfer over a Reliable Channel
 This program takes 4 command line argument - input file, output file, IP Address, port number
 -Client connects to server using IP Address (localhost) and arbitrary port number 8090
 -Upon connection sends the filename of output file
 -Sends data in chunks of 10 bytes to write to output file
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

#define WRITE_LENGTH 10
#define QUEUE_SIZE 5  /*Allow 5 call holds */

void errorMessage(const char *);

int main(int argc, char *argv[]) {

	int sockfd; /* Listen on sockfd, new connection on newSockfd */
	struct sockaddr_in client_serv; /* Internet socket name (addr) */
	struct hostent *server;

	/* argv[1]-input file, argv[2]-Output file,
	  argv[3]-IP Address, argv[4] port number*/
	if (argc < 5) {
		errorMessage("Usage: client server-name file-name \n");
	}

	/* argv[3] = localhost*/
	server = gethostbyname(argv[3]);
	if (server == NULL) {
		errorMessage("Error, look-up host IP address failed \n");
	}

	/* Passive open. Wait for connection */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		errorMessage("Error, socket opening failed \n");
	} else {
		printf("Initialing Socket \n");
	}

	/* Build address structure to bind to socket */
	memset(&client_serv, 0, sizeof(client_serv));
	client_serv.sin_family = AF_INET; /* Internet Domain */
	memcpy(&client_serv.sin_addr.s_addr, server->h_addr, server->h_length);
	client_serv.sin_port = atoi(argv[4]); /*argv[4] = PortNumber*/

	/* Connect to server */
	if (connect(sockfd, (struct sockaddr *) &client_serv, sizeof(client_serv))
			< 0) {
		close(sockfd);
		errorMessage("Error, Bind failed \n");
	} else {
		printf("Client connection successful \n");
	}


	/* Sending Output File name */
	if (write(sockfd, argv[2], strlen(argv[2])) < 0) {
		errorMessage("Sending Output file name failed");
	} else {
		printf("File name sent - %s \n", argv[2]);
	}


	/* Sending new line for completion of file name */
	if (write(sockfd, "\n", 1) < 0) {
		errorMessage("Failed to send new line");
	}


	/* Opening Input File name */
	char* file_name = argv[1];
	char sdbuf[WRITE_LENGTH];

	printf("Input file name is : %s \n", file_name);

	FILE *fs = fopen(file_name, "r");
	if (fs == NULL) {

		errorMessage("ERROR: File %s not found.\n");
		exit(1);
	}

	bzero(sdbuf, WRITE_LENGTH);
	int fileBlk;
	int bytesSent = 0;
	while ((fileBlk = fread(sdbuf, sizeof(char), WRITE_LENGTH, fs)) > 0) {

		if (send(sockfd, sdbuf, fileBlk, 0) < 0) {
			errorMessage("ERROR: Failed to send file.\n");
			exit(1);
		}
		bytesSent += fileBlk;

		 printf("Data sent from client in chunks of 10 [%d] bytes - [%s] \n",
							 bytesSent,sdbuf);

		bzero(sdbuf, WRITE_LENGTH);

	}
	 printf("Input File %s from Client was sent with number of [%d] bytes  \n",
			file_name, bytesSent);

	close(sockfd);

	return 0;
}

/* Print Error message and exit */
void errorMessage(const char *message) {

	perror(message);
	exit(1);

}
