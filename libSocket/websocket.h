//
//  websocket.h
//  libSocket
//
//  Copyright © 2021 bobles. All rights reserved.
//

#ifndef websocket_h
#define websocket_h
#include <arpa/inet.h>

struct websock_packet { // represents a socket dataframe
	uint8_t fin;
	uint8_t reserved;
	uint8_t pmc;
	uint8_t opcode;
	uint8_t isMasked;
	uint8_t len;
	char* data;
};
struct websocket { // represents a socket connection
	in_addr_t ipaddr;
	int port;
	uint8_t isSecure;
	uint64_t timeout;
	uint8_t isActive; // set to 0 to destroy
	char *path; // for handshake. nvm
	//char *msgToSend; // set to not "" to send something waait no that's what queue is for delete this pls
	char *host; // for handshake
	pthread_t thread;
	char *dataBuffer; // for reading from it.
	int socketfd; // internal use.
	struct websock_packet queue[256]; // why does queue have to be last? because for some reason
	// if not it wants to overwrite the other stuff, so i have decided to fix that by hack
	
};
#include <stdio.h>
struct websock_packet decodeWS(char *bytes);
char *encodeWS(struct websock_packet packetData);
struct websocket *start_connection(char *ipaddr, uint8_t isSecure, char* host, int port, char *path);
void sendMessage(struct websocket web, char *message);
#endif /* websocket_h */
