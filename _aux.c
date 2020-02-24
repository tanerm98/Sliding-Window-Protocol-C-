#include <stdio.h>

unsigned short in_cksum(const unsigned short *addr, int len, unsigned short csum)
{
	int nleft = len;
	const unsigned short *w = addr;
	unsigned short answer;
	int sum = csum;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1)
		sum += *(unsigned char *)w;

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);

}
