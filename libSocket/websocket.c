//
//  websocket.c
//  libSocket
//
//  Copyright © 2021 bobles. All rights reserved.
//
#include <netdb.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <errno.h>
#include "websocket.h"
#define index1MK(a,b) (a<<b*8) >> (32-(b*8))
#define indexMK(a,b) (a>>b*8) 

struct websock_packet decodeWS(char *bytes) { // PLEASE null-terminate the bytes, thanks!
	uint8_t opc = bytes[0];
	uint8_t fin = opc >> 7; // gets first bit, X... ....
	uint8_t reserved = (opc << 1) >> 5; // removes first bit, gets next 3, .XXX ....
	uint8_t pmc = (opc << 1) >> 2; // per-message compressed, remove first bit, get next bit, .X.. ....
	uint8_t opcode = (opc << 4) >> 8; // real opcode, last 4 bytes. .... XXXX
	uint8_t inb = bytes[1];
	uint8_t isMasked = inb >> 7; // gets first bit, X... ....
	uint8_t lenb = (inb << 1) >> 8;
	int len = 0;
	uint8_t startAt = 2;
	if (lenb >= 125) {
		len = 125;
	} else if (lenb == 126) {
		startAt = 4; // If this happens, i know len will be different but that doesn't actually matter at all.
		len = lenb;
	} else if (lenb == 127) {
		startAt = 10; //
		len = lenb;
	}
	char decoded[256]; // pls free it btw.
	if (isMasked) {
		startAt = startAt + 4;
		uint32_t maskKey = bytes[2];
		memcpy(&maskKey, bytes+2, 4); // yay it works.
		for (int i = startAt; i < strlen(bytes); i++) {
			decoded[i-startAt] = bytes[i] ^ (uint8_t)indexMK(maskKey,(i-startAt % 4));
		}
	} else {
		for (int i = startAt; i < strlen(bytes); i++) {
			decoded[i-startAt] = bytes[i];
		}
	}
	decoded[(strlen(bytes)-startAt)]=0x00;//null term
	struct websock_packet returnValue;
	returnValue.data=strdup(decoded);
	returnValue.fin=fin;
	returnValue.isMasked=isMasked;
	returnValue.len=len;
	returnValue.opcode=opcode;
	returnValue.pmc=pmc;
	returnValue.reserved=reserved;
	return returnValue;
}
char *encodeWS(struct websock_packet packetData) {
	// OK im gonna assume this is text data, for now - actually no. i'll make the packet... :|
	packetData.len = strlen(packetData.data);
	char bytes[16+strlen(packetData.data)];
	// ok time to compress these into a single byte!
	uint8_t byte1 = 0;
	byte1 |= packetData.fin << 7;
	byte1 |= ((packetData.reserved << 8) >> 4);
	byte1 |= ((packetData.pmc << 8) >> 2);
	byte1 |= ((packetData.opcode << 8) >> 8); // compressed into a byte.
	bytes[0]=byte1;
	uint8_t byte2 = 0; // next byte!
	byte2 |= ((packetData.isMasked << 8) >> 1);
	byte2 |= ((packetData.len << 1) >> 1);
	bytes[1]=byte2;
	// ok now on to the thingy
	// must make a random mask!
	// then aalso maask dataa.
	bytes[2] = rand() & 0xFF;
	bytes[3] = rand() & 0xFF;
	bytes[4] = rand() & 0xFF;
	bytes[5] = rand() & 0xFF;
	for (uint8_t i=0; i<=strlen(packetData.data)-1; i++) {
		if (packetData.data[i] != 0) { //websocket data doesn't mask the null-terminator.
			bytes[i+6]=packetData.data[i] ^ (uint8_t)(bytes[(i % 4)+2]);
		}
	}
	bytes[6+strlen(packetData.data)]=0; // null-terminate it
	return strdup(bytes);
}

#define copyStrWithoutTerm(a,b,c) memcpy(&a+b, strdup(c), strlen(c)-1)

void *wscon(void *argptr) {
	struct websocket *connection = (struct websocket*)argptr;
	int byten = 0;
	struct websock_packet *queue= connection->queue;
	int socketfd = connection->socketfd;
    
	while (connection->isActive) {
		if (queue[0].data != NULL) {
		    printf(queue[0].data);
			byten=0;
			char *req = encodeWS(queue[0]);
			queue[0].data=NULL;
			printf("%i\n",strlen(req));
				int pb;
				if (connection -> isSecure) {
//				//openssl write pls	int pb = write(socketfd, req+byten, strlen(req)-byten);

				} else {
					pb = write(socketfd, req+byten, strlen(req)-byten);

				}
//
				int pbe = errno;
				if (pbe == 32) {
					break;
				}
				byten=byten+pb;
		    queue[0].opcode=123;
		} else {
			if (connection->isSecure) {
				//open ssl read
			} else if (queue[0].opcode==123) {
				byten=0;
				char *currentBuffer=malloc(1024);
// ok this will check for a response.
				byten = read(socketfd,currentBuffer, 1024);
				write(STDOUT_FILENO, currentBuffer, byten);
			    queue[0].opcode=122;
			}
		}
		printf("DOING thING");
		sleep(1);
	}
    printf("cn inactived");
    printf("%i", connection->isActive);
	return NULL;
}

struct websocket *start_connection(char *ipaddr, uint8_t isSecure, char* host, int port, char *path) {
	signal(SIGPIPE, SIG_IGN);

	struct websocket connection;
	connection.ipaddr = inet_addr(ipaddr);
	connection.isSecure=isSecure;
	connection.port=port;
	connection.isActive=1;
	connection.host=host;
	connection.path=path;
	//request_h = 'GET /devtools/page/D518EAC3ECC20D705EF2EB42F7CF00BD HTTP/1.1\nHost: 127.0.0.1:9222\nUpgrade: websocket\nConnection: Upgrade\nSec-WebSocket-Key: dGhlIeNhbXBsZSBccub2jZQ==\nSec-WebSocket-Version: 13\nOrigin: 127.0.0.1:9222\r\n\r\n'
	char *handshake = malloc(1024);
	snprintf(handshake, 1024, "GET %s HTTP/1.1\r\nHost: %s:%hhu\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\nOrigin: %s:%hhu\r\n\r\n", path, host, port, "feedface", host, port); // a mess to generate handshake for the uh thingy idk tbh
	int socketfd;
	char *buffer = malloc(1024);
	struct sockaddr_in s_in;
	struct protoent *pent;
	pent = getprotobyname("tcp");
	s_in.sin_addr.s_addr = connection.ipaddr;
	s_in.sin_family = AF_INET;
	s_in.sin_port = htons(port);
	struct timeval timeout;
	timeout.tv_sec=1;
	timeout.tv_usec=0;
	struct in_addr iiadrn;
	iiadrn.s_addr=connection.ipaddr;
	socketfd = socket(AF_INET, SOCK_STREAM, pent->p_proto);
	connection.socketfd=socketfd;
	int flags = fcntl(socketfd, F_GETFL,0) | O_NDELAY;
	int con = connect(socketfd, (struct sockaddr*)&s_in, sizeof(s_in));
	fd_set set;
	FD_ZERO(&set);
	FD_SET(socketfd, &set);
	int e4=1;
	char *rees = inet_ntoa(iiadrn);
	int ernn = errno;
	flags &= (~O_NONBLOCK);
	int ern = errno;
	int byten = 0;
	if (isSecure) {
		while (byten < strlen(handshake)) {
			int pb = write(socketfd, handshake+byten, strlen(handshake)-byten); // openssl this later.
			int pbe = errno;
			if (pbe == 32) {
				break;
			}
			byten=byten+pb;
		}
	} else {
		while (byten < strlen(handshake)) {
			int pb = write(socketfd, handshake+byten, strlen(handshake)-byten);
			int pbe = errno;
			if (pbe == 32) {
				break;
			}
			byten=byten+pb;
		}
	}
	byten = read(socketfd,buffer, 1024);
	write(STDOUT_FILENO, buffer, byten);
	byten=0;
	struct websock_packet initReq;
	initReq.data = "{\"id\": 1, \"method\": \"Page.reload\"}";
	initReq.fin=1;
	initReq.isMasked=1;
	initReq.opcode=1;
	initReq.pmc=0;
	initReq.reserved=0;
	initReq.len=strlen(initReq.data)-1;
	printf(decodeWS(encodeWS(initReq)).data);
	connection.queue[0]=initReq;
	pthread_t thread;
	pthread_create(&thread, NULL, wscon, (void*)&connection);
	pthread_detach(thread);
	return &connection;
}





















































