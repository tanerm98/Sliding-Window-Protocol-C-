#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"
#include "aux.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main (int argc,char** argv) {

	msg r,t;
	myData tnr;
	myACK mack;
	int nrOfPacks = 8000;
	init (HOST,PORT);

	int outFile = -1;

	//temporary ordered storage for packages
	char **tempFile = (char**) calloc(7300, sizeof(char*));
	//stores which package was already received
	int *recvd = (int*) calloc(7300, sizeof(int));
	//stores the size of each received package
	int *ssize = (int*) calloc(7300, sizeof(int));
	
	
	//stores the number of received packages
	int received = 0;
	
	int checkSum;
	
	//receives packages, stores them and sends ACK
	while (received < nrOfPacks) {
		recv_message(&r);
		
		tnr = *((myData*)r.payload);
		
		//this is in case one of them is corrupted, then we know for sure the 
		//2 equal ones are correct
		if (tnr.order2 == tnr.order3) {
			tnr.order = tnr.order2;
		}
		
		//avoid receiving duplicate packages
		if (recvd[tnr.order] == 1) {
			continue;
		}
		
		//checks if it has received the metadata package
		if (tnr.order == 0) {
			//verifies if the message is corrupt or not
			checkSum = in_cksum((const unsigned short *)(&(tnr.len)), 15, 0);
			if (checkSum != tnr.checkSum) {
				//sends the ACK for corrupted packages
				mack.order = -2;
				memcpy (t.payload, &mack, sizeof(myACK));
				t.len = sizeof(myACK);
				send_message (&t);
				
				continue;
			}
			//sets the package as succesfuly received
			recvd[tnr.order] = 1;
			//sets the correct number of packages to be received. Until this,
			//the number is set higher than possible.
			nrOfPacks = tnr.len;
			printf ("[%s] Got file name!\n", argv[0]);
			
			//opens the output file
			outFile = open (tnr.data, O_CREAT | O_WRONLY, 0777);
			
		} else {
			//verifies if the message is corrupt or not
			checkSum = in_cksum((const unsigned short *)(tnr.data), tnr.len, 0);
			if (checkSum != tnr.checkSum) {
				//sends the ACK for corrupted packages
				mack.order = -2;
				memcpy (t.payload, &mack, sizeof(myACK));
				t.len = sizeof(myACK);
				send_message (&t);
				
				continue;
			}
			//sets the package as succesfuly received
			recvd[tnr.order] = 1;
			printf ("[%s] Got package %d! (%d)\n", argv[0], tnr.order, received);
		}
		
		//updates the number of succesfuly received packages
		received++;

		//temporary stores the package and its size
		tempFile[tnr.order] = (char*) calloc(tnr.len + 1, sizeof(char));
		ssize[tnr.order] = tnr.len;
		memcpy (tempFile[tnr.order], tnr.data, tnr.len);

		//sends ACK with the order of the received package
		mack.order = tnr.order;
		memcpy (t.payload, &mack, sizeof(myACK));
		t.len = sizeof(myACK);
		send_message (&t);
		
	}
	
	//writes the package in the output file
	int i;
	for (i = 1; i < nrOfPacks; i++) {
		write (outFile, tempFile[i], ssize[i]);
		//free(tempFile[i]);
	}

	free(tempFile);

	close(outFile);
	
	//sends the last ACK which permits the sender to shut down
	mack.order = -1;
	memcpy (t.payload, &mack, sizeof(myACK));
	t.len = sizeof(myACK);
	send_message (&t);
	
	return 0;
}
