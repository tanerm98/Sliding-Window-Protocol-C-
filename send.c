#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#include "link_emulator/queue.h"
#include "aux.h"
#include <math.h>

#define HOST "127.0.0.1"
#define PORT 10000
#define packSize 1380

int main (int argc,char** argv) {

	init (HOST,PORT);
	msg t;
	myData tnr;	//temporary stores a package
	myACK mack;	//temporary stores an ACK
	//length of crt pack read from file & crt nr of packages in the window
	int len, crtPacks = 0;
	
	
	//setting the window dimension
	int BDP = atoi (argv [2]) * atoi (argv [3]) * 1000;
	int windowDim = BDP / (MSGSIZE * 8);
	printf("**********************Number of windows: %d.\n", windowDim);

	
	//opening input file and setting output file name
	char outFileName [256] = "recv_";
	strcat (outFileName, argv [1] );
	int inFile = open (argv [1], O_RDONLY);


	//size of file
	struct stat st;
	stat(argv[1], &st);
	size_t size = st.st_size;
	printf("**********************File size: %ld.\n", size);


	//number of packages to be sent
	int nrOfPacks;
	nrOfPacks = size / packSize + 1;
	if (size % packSize != 0) {
		nrOfPacks++;
	}
	printf("**********************Number of packs: %d.\n", nrOfPacks);
	
	
	//initializing a package array
	myData *packages = (myData*) calloc (nrOfPacks + 1, sizeof(myData));
	//keeping evidence of all the succesfuly sent packages
	int *sent = (int*) calloc(nrOfPacks + 1, sizeof(int));
	//storing the number of packages remaining to be sent
	int toSend = nrOfPacks;


	//sending in the available space the number of packages to be received;
	//this is the first package / header
	tnr.len = nrOfPacks;
	tnr.order = tnr.order2 = tnr.order3 = 0;
	sprintf (tnr.data, "%s", outFileName);
	tnr.checkSum = in_cksum((const unsigned short *)(&(tnr.len)), 15, 0);


	//loading the metadata package in the vector
	memcpy (&packages[0], &tnr, sizeof(myData));


	//loading all the packages in the vector
	int i = 1;
	while (1) {
		len = read (inFile, tnr.data, packSize);
		if (len <= 0) {
			break;
		}
		tnr.len = len;
		tnr.order = tnr.order2 = tnr.order3 = i;	
		tnr.checkSum = in_cksum((const unsigned short *)(tnr.data), tnr.len, 0);
		memcpy (&packages[i++], &tnr, sizeof(myData));
	}
	
	//sending all the packages one by one
	int rotation = 0, sentNr = 0, res;
	for (i = 0; i < nrOfPacks; i++) {
		//only sending the packages not already sent
		if (sent[i] == 1) {
			//the packages array is circled until there's no unsent one left
			if (i == nrOfPacks - 1) {
				rotation++;
				i = -1;
			}
			continue;
		}
		
		//keeps the sliding window permanently full for the best performance
		if (crtPacks < windowDim) {
			memcpy (t.payload, &packages[i], sizeof(myData));
			send_message (&t);
			crtPacks++;
			sentNr++;
		}
	
		//after the first filling of the sliding window, receiving ACKs starts
		if (sentNr >= windowDim) {
			res = recv_message_timeout(&t, 1);
			//succesful ACK or not, it is considered that the window has one
			//more empty frame.
			crtPacks--;
			if (res <= 0) {
				if (i == nrOfPacks - 1) {
					rotation++;
					i = -1;
				}
				continue;
			} else {
			
				mack = *((myACK*)t.payload);
				
				//"-2" is the order of the ACK for corrupt files
				if (mack.order == -2) {
					if (i == nrOfPacks - 1) {
						rotation++;
						i = -1;
					}
					continue;
				}
				
				//"-1" is the order of the ACK that marks the receiving of all
				//the packages and it permits the sender to shut down
				if (mack.order == -1) {
					printf("***Finished sending!***\n");
					close (inFile);
					return 0;
				}
				
				//verifies if the ACK was already received before for the same
				//package. If not, the number of remaining packages decrements.
				if (sent[mack.order] == 0) {
					sent[mack.order] = 1;
					toSend--;
					if (toSend == 0) {
						break;
					}
				}
				
				printf("[%s] Got ACK %d!\n", argv[0], mack.order);
			}
		}
		
		//cycling the array until it is empty
		if (i == nrOfPacks - 1) {
			rotation++;
			i = -1;
		}
	}

	//after sending everything succesfully, the final ACK is waited
	while (1) {

		res = recv_message_timeout(&t, 0);
		if (res > 0) {
			if ( (*((myACK*)t.payload)).order == -1) {
				printf("***Finished sending!***\n");
				break;
			}
		}
	}
	close (inFile);

	return 0;


}
