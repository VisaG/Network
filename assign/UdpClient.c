/*
 Author: Visalakshi Gopalakrishnan
 Student ID:00000976426
 COEN 233 - Computer Networks.

 PART-2 Client Side:Reliable Transfer over an Unreliable Channel with Bit Errors that can also loose packets
 -Program is based Stop and Wait (S&W) reliable protocol
 -Program takes 4 command line argument - input file, output file, IP Address, port number
 -Client connects to server using IP Address (localhost) and arbitrary port number 8090
 -Upon connection sends the filename of output file
 -Sends data in chunks of 10 bytes to write to output file to server
 -Before sending frame- calculates checksum A
 -Add appropriate SEQ number and checksum to frame before sending
 -Waits on ACK from server
 -If ACK != SEQ, resends same frame to server
 -If ACK not received within the stipulated time intervel, resend frame
 -If ACK = SEQ, transmission successful, sends the next frame

 -Induce errors -
 -Use result of a random function to decide to send next or prior frame incase of ACK != SEQ
 -Decide whether to send the right checksum or just zero to fake the packet error and loss of a packet effect.

 -Closes the connection when data is transferred successfully. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

#define WRITE_LENGTH 10
#define SIMULATE_CHECKSUM_ERROR 3

extern int errno;

typedef struct {
	char data[WRITE_LENGTH];
	int dataSize;
} Packet;

typedef struct {
	unsigned long checkSum; 	/* CheckSum of the frame */
	int seq; 					/* Sequence of the frame 0 or 1 */
	int ack;					/* Acknowledgment of the frame recd. 0 or 1*/
	int fin;					/* INdicate end of the File */
	Packet packet;
} Frame;

/* Error message function */
void errorMessage(const char *);

/* Function to calculate checkSum of frame */
unsigned long hash(unsigned char *str, int slen);

/* Generate random number to induce error */
int generateRandom();

int main(int argc, char *argv[]) {

	int sockfd;
	struct sockaddr_in client_serv, from_serv;
	struct hostent *server;
	Frame frameClient, frameServ;
	struct timeval time;

	/* argv[1]-input file, argv[2]-Output file, argv[3]-IP Address, argv[4] port number */
	if (argc < 5) {
		errorMessage("Usage: client server-name file-name \n");
	}


	/* argv[3] = localhost*/
	server = gethostbyname(argv[3]);
	if (server == NULL) {
		errorMessage("Error, look-up host IP address failed \n");
	}


	/* Passive open. Wait for connection or generate error message if failed */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		errorMessage("Error, socket opening failed \n");
	} else {
		printf("Initialing Socket \n");
	}


	/* Build address structure to bind to socket */
	memset(&client_serv, 0, sizeof(client_serv));
	memset(&from_serv, 0, sizeof(from_serv));
	client_serv.sin_family = AF_INET;
	memcpy(&client_serv.sin_addr.s_addr, server->h_addr, server->h_length);
	client_serv.sin_port = atoi(argv[4]);

	time.tv_sec = 1;
	time.tv_usec = 0;

	/* Set time interval or generate error message if failed */
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time)) < 0) {
		perror("Error");
	}


	/* Sending Output File name or generate error message if failed */
	if (sendto(sockfd, argv[2], strlen(argv[2]), 0, &client_serv,
			sizeof(client_serv)) < 0) {
		errorMessage("Sending Output file name failed");
	} else {
		printf("File name sent - %s \n", argv[2]);
	}


	/* Opening Input File name */
	char* file_name = argv[1];
	char sdbuf[WRITE_LENGTH];

	printf("Input file name is : %s \n \n", file_name);

	FILE *fs = fopen(file_name, "r");
	if (fs == NULL) {

		errorMessage("ERROR: File %s not found.\n");
		exit(1);
	}


	bzero(sdbuf, WRITE_LENGTH);
	int fileBlk, rc;
	int bytesSent = 0;
	memset(&frameClient, 0, sizeof(frameClient));



	/* Read data from the file to be transfered, make a packet and send it to server */
	while ((fileBlk = fread(sdbuf, sizeof(char), WRITE_LENGTH, fs)) > 0) {

		memcpy(&(frameClient.packet.data), sdbuf, fileBlk); /* Frame created */
		frameClient.checkSum = hash(frameClient.packet.data, fileBlk); /* Checksum of Frame calculated */
		frameClient.packet.dataSize = fileBlk; /* Size of Frame added */

		char duplicateFlag = 0;
		unsigned long tempChecksum = frameClient.checkSum;


		/* Simulate checksum error, 1 in 10 chance */
		if (generateRandom() == SIMULATE_CHECKSUM_ERROR) {
			frameClient.checkSum = 0; /* Set checksum of Frame = 0 */
			printf("Simulating checksum error \n");
		}

		printf("\n------------------------------Frame Sent [%d]------------------------------\n", (bytesSent / 10));

		while (1) {

			/* Send frame with SEQ and checksum or generate error if failed */
			if (sendto(sockfd, &frameClient, sizeof(frameClient), 0,
					&client_serv, sizeof(client_serv)) < 0) {

				errorMessage("ERROR: Failed to send file.\n");
				exit(1);
			}


			/* Handle ACK of duplicate frame received by client due to timeout condition */
			if (duplicateFlag) {

				/* Receives ACK of duplicate frame */
				rc = recvfrom(sockfd, &(frameServ.ack), sizeof(frameServ.ack),
						0, &from_serv, &from_serv);

				printf("Waiting on ACK from server after sending duplicate frame \n");

				if (rc > 0) {
					printf("Frame ACK received after sending duplicate frame - [%d] \n",
							frameServ.ack);
				} else {
					duplicateFlag = 0;
				}
			}


			printf("Checksum of data calculated by Client - [%ld] \n",
					frameClient.checkSum);
			printf("Frame SEQ generated by Client- [%d] \n", frameClient.seq);
			printf("Data sent by Client- [%s] \n", frameClient.packet.data);
			printf("Waiting on ACK from server\n");


			/* Receives ACK from server */
			rc = recvfrom(sockfd, &(frameServ.ack), sizeof(frameServ.ack), 0,
					&from_serv, &from_serv);


			/* Handle condition of ACK not received. i.e: ACK lost or Timed out before ACK was received */
			/* ETIMEOUT = Timeout error or EAGAIN = Block on write */
			if (rc < 0 && (errno == ETIMEDOUT || errno == EAGAIN)) {
				if (duplicateFlag == 1) {
					printf("---------------ACK Loss accounted---------------\n");
					printf("Frame ACK received - [%d] \n", frameServ.ack);
					duplicateFlag = 0;
					break;
				}

				printf("---------------Timed Out waiting on receive---------------\n");
				duplicateFlag = 1;
				sleep(1);
				continue;

			} else {

				printf("Frame ACK received - [%d] \n", frameServ.ack);

				/* Handle condition of ACK != SEQ. */
				if (frameServ.ack != frameClient.seq) {

					printf("ACK of frame received [%d] did not match the SEQ of frame generated [%d] \n",
							frameServ.ack, frameClient.seq);

					/* Handle condition of ACK != SEQ. due to checksum error */
					if (frameClient.checkSum == 0)
						frameClient.checkSum = tempChecksum;
					continue;
				}

				/* All conditions satisfied */
				printf(
						"Ack sent by server matched - [%d] with SEQ generated by Client [%d] \n\n",
						frameServ.ack, frameClient.seq);

				break;
			}

		}

		bytesSent += fileBlk; /* Count of bytes sent */

		memset(&frameClient.packet, 0, sizeof(frameClient.packet));
		frameClient.seq = ~(frameClient.seq) & 1; /* SEQ number 1 or 0 */

		/* Reset induced checksum error to correct checksum */
		if (frameClient.checkSum == 0)
			frameClient.checkSum = tempChecksum;

	}

	printf("Input File %s from Client was sent with number of [%d] bytes  \n",
			file_name, bytesSent);

	frameClient.fin = 1; /* Set fin to indicate end of file */

	/* Send information to indicate end of file */
	sendto(sockfd, &frameClient, sizeof(frameClient), 0, &client_serv,
			sizeof(client_serv));

	close(sockfd);
	fclose(fs);

	printf("after close \n");
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
	return (rand() % 10);
}
