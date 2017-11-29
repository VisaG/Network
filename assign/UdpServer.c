/*
 Author: Visalakshi Gopalakrishnan
 Student ID:00000976426
 COEN 233 - Computer Networks.

 PART-2 Server Side:Reliable Transfer over an Unreliable Channel with Bit Errors that can also loose packets
 -Program is based Stop and Wait (S&W) reliable protocol
 -Server binds to arbitrary port number 8090
 -Client sends output file with Data and appropriate SEQ number for that frame
 -Server Waits to receive data from client
 -Receives data in chunks of 10 bytes
 -Sends ACK after receiving data
 -Handles duplicate frame and checksum error, while blocking on write
 -Writes data in chunks of 10 bytes in the out put file sent by client
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
#define READ_SIZE 10

#define SIMULATE_TIMEOUT_ERROR 7
#define SIMULATE_ACK_LOSS_ERROR 4

typedef struct {
	char data[READ_SIZE];
	int dataSize;
} Packet;


typedef struct {
	unsigned long checkSum;					/* Checksum of the frame */
	int seq;								/* Sequence of the frame recd. 0 or 1 */
	int ack;								/* Acknowledgment of the frame sent 0 or 1*/
	int fin;								/* Indicate end of the File */
	Packet packet;
} Frame;

/* Error message function */
void errorMessage(const char *);

/* Function to calculate checkSum of frame */
unsigned long hash(unsigned char *str, int slen);

/* Generate random number to induce error */
int generateRandom();

int main(int argc, char *argv[]) {

	int rc;
	int sockfd; 							/* Socket Descriptor */

	struct sockaddr_in serv, client;
	int clientlen = sizeof(client);
	char buffer[BUF_SIZE];					/* Buffer for out going file */
	Frame frameServ, frameClient;

	while (1) {

		/* Initialize Socket or generate error if failed */
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			errorMessage("Error, socket opening failed \n");
		} else {
			printf("\nInitialing Socket \n");
		}


		memset(&frameServ, 0, sizeof(frameServ));
		memset(&frameClient, 0, sizeof(frameClient));
		memset(buffer, 0, sizeof(buffer));

		/*Build address structure to bind to socket */
		memset(&serv, 0, sizeof(serv));
		memset(&client, 0, sizeof(client));
		serv.sin_family = AF_INET;
		serv.sin_port = 8090; 				/* System binds to port 8090 */
		serv.sin_addr.s_addr = INADDR_ANY;	/* Accept any client */


		/* Binding to socket or generate error and close socket connection if failed */
		if (bind(sockfd, &serv, sizeof(serv)) < 0) {
			close(sockfd);
			errorMessage("Error, Bind failed \n");
		} else {
			printf("Binding to Socket \n");
		}


		/* Get the output File name from client to store data after accepting connection */
		int i = 0;
		rc = recvfrom(sockfd, buffer, BUF_SIZE, 0, &client, &clientlen);

		printf("Got output file name from client %s \n", buffer);

		FILE *fr = fopen(buffer, "w");
		int bytesRecv = 0;

		bzero(buffer, BUF_SIZE);

		if (fr == NULL) {
			errorMessage("File cannot be opened file on server.\n");
		}

		frameServ.ack = -1;
		int randomError;
		unsigned long tempCheckSum;

		/* Getting data from client */
		while (1) {

			/* Get random number to induce error */
			randomError = generateRandom();

			memset(&frameClient, 0, sizeof(frameClient));

			/* Receive data from Client with SEQ number and checksum of frame */
			rc = recvfrom(sockfd, &frameClient, sizeof(frameClient), 0, &client,
					&clientlen);


			/* Calculate checksum of frame received to ensure no corrupt data */
		    tempCheckSum = hash(frameClient.packet.data,
									frameClient.packet.dataSize);


		    /* If checksum of frame received == checksum of frame sent
		     * AND if ACK number after receiving frame == SEQ number of frame sent
		     * Frame is duplicate and handled  */
			if ((frameServ.checkSum == frameClient.checkSum)
					&& (frameServ.ack == frameClient.seq)) {

				printf("---------------Duplicate frame [%s]---------------\n",frameClient.packet.data);

				sendto(sockfd, &(frameServ.ack), sizeof(frameServ.ack), 0, &client,
				sizeof(client));

				printf("----------------Duplicate ACK sent [%d]---------------\n",frameServ.ack);

			} else {

				printf("\n------------------------------Frame Received [%d]-----------------------------\n", (bytesRecv/10));


				if (rc < 0) {
					errorMessage("Error, reading from socket failed \n");
				}

				/* End of file */
				if (frameClient.fin == 1) {
					printf("End of receiving packets \n");
					break;
								}

				/* Closing connection after file transfer */
				if (rc == 0) {
					printf("Closing connection \n");
					break;
				}

				/* Handles condition when checksums do not match */
				if (frameClient.checkSum != tempCheckSum) {

					printf( "CheckSum of frame received [%ld] did not match the checksum of frame calculated [%ld] \n",
							frameClient.checkSum, tempCheckSum);

				/* Handles condition when SEQ != ACK */
				} else if (frameClient.seq != (~(frameServ.ack) & 1)) {
					printf(
							"SEQ of frame received [%d] did not match the ACK of frame sent [%ld] \n",
							frameClient.seq, frameServ.ack);

				} else {

					rc = frameClient.packet.dataSize;

					/* Write data to file if all conditions of error free transmission are met */
					int write_file = fwrite(frameClient.packet.data,
							sizeof(char), rc, fr);

					if (write_file < rc) {
						errorMessage("File write failed on server.\n");
					}

					bytesRecv += rc;

					frameServ.ack = frameClient.seq;
					frameServ.checkSum = tempCheckSum;


					printf("Checksum sent by client - [%ld] \n", frameClient.checkSum);
					printf("Frame Seq received - [%d] \n",frameClient.seq);
					printf("Frame data received - [%s] \n",frameClient.packet.data);
					printf("Checksum calculated by server - [%ld] \n", tempCheckSum);


				}

				/* Simulate Timeout by using sleep() function, 1 in 10 chance */
				if (randomError == SIMULATE_TIMEOUT_ERROR) {

					printf("---------------Sleeping to simulate timeout---------------\n");
					sleep(2);
				}

				/* Simulate ACK loss, 1 in 10 chance by not sending ACK
				 * to client within the time interval */
				if (randomError == SIMULATE_ACK_LOSS_ERROR) {

			         printf("---------------Simulating ACK loss error---------------\n");

			    } else {

			    printf("Frame Ack sent - [%d] \n\n", frameServ.ack);

			    /* Send ACK only if all conditons for error free transmission are met */
				sendto(sockfd, &(frameServ.ack), sizeof(frameServ.ack), 0, &client,
						sizeof(client));
			    }
			}
		}

		printf("Received total of [%d] bytes from client \n", bytesRecv);

		fclose(fr);

		/* Close connection after data received*/
		close(sockfd);
		printf("Closing socket \n");

	}

	return 0;
}

/* Print Error message and exit */
void errorMessage(const char *message) {

	perror(message);
	exit(1);

}

/* Calculate checksum */
unsigned long hash(unsigned char *str, int slen) {

	unsigned long hash = 5381;
	int c, i = 0;

	while (i < slen) {
		c = *str++;
		i++;
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}


/* Generate 1 in 10 chance */
int generateRandom() {
    return(rand() % 10);
}


