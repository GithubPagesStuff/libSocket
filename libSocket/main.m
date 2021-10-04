//
//  main.m
//  libSocket
//
//  Copyright © 2021 bobles. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "websocket.h"
int main(int argc, const char * argv[]) {
	
	struct websock_packet defaultPacket;
	defaultPacket.fin=1;
	defaultPacket.reserved=0;
	defaultPacket.pmc=0;
	defaultPacket.isMasked=1;
	defaultPacket.len=2;
	defaultPacket.opcode=1;
	defaultPacket.data="{}";
//	printf(decodeWS(encodeWS(defaultPacket)).data);
	struct websocket *connection = (start_connection("127.0.0.1", 0, "127.0.0.1", 9222, "/devtools/page/867B88D0A1CB742AC967021943A57881"));
	while (0==0) {
		char *input=malloc(256);

		//gets(input);
//		printf(input);
		struct websock_packet initReq;
		initReq.data = strdup("{\"id\": 1, \"method\": \"Page.reload\"}");
		initReq.fin=1;
		initReq.isMasked=1;
		initReq.opcode=1;
		initReq.pmc=0;
		initReq.reserved=0;
		initReq.len=strlen(initReq.data)-1;
		connection->queue[0]=initReq;
		sleep(0.1);
		free(input);
	}
	
	
	return 0;
}
