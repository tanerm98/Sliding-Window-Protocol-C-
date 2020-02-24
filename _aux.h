
//structure used for sending a package
typedef struct {
	//stores the precalculated checkSum of section <len> + <data>
	unsigned short checkSum;
	
	//the order of the packages is the most important when finding out which 
	//package was succesfuly sent or not. So, knowing that only one byte can 
	//be corrupted, at least 2 of these are equal. Therefore, the correct order 
	//of the packages will always be known.
	int order;
	int order2;
	int order3;	
	
	//the length of the package
	int len;
	
	//the actuala package
	char data[1380];
} myData;


//structure for sending ACK. Only contains the order number of the package for 
//which the ACK is done.
typedef struct {
	int order;
} myACK;


//function that calculates the checksum of a package.
unsigned short in_cksum(const unsigned short *addr, int len, unsigned short csum);
