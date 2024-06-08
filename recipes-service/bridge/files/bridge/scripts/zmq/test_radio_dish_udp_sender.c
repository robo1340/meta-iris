#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h> // for usleep
#define ZMQ_BUILD_DRAFT_API
#include <zmq.h>

volatile sig_atomic_t stop;

#define DEFAULT_URL "udp://10.255.255.255:1337"
#define MSG_MAXLEN 1500

void my_free (void *data, void *hint)
{
    free (data);
}

static uint16_t read_stdin(uint8_t * msg){
	uint16_t i;
	for (i=0; i<MSG_MAXLEN; i++){
		fread(&msg[i], sizeof(char), 1, stdin);	
		if (msg[i] == EOF){
			if (i > 0) {i--;}
			break;
		}
	}
	return i;
}

int main(int argc, char *argv[]) {
	char url[64];
	uint8_t * pay = (char*)malloc(MSG_MAXLEN);
	assert(pay != NULL);
	memset(pay, 0, MSG_MAXLEN);
	uint16_t msg_len;
	
	if (argc >= 3){ //arg 1 is the message, arg2 is the address
		snprintf(url, sizeof(url), "udp://%s:1337", argv[2]);
		printf("%s\n",url);
		strncpy((char*)pay, argv[1], MSG_MAXLEN);
		msg_len = strnlen((char*)pay, MSG_MAXLEN);
	} 
	else if (argc == 2){ //arg 1 is the message
		strncpy((char*)pay, argv[1], MSG_MAXLEN); 
		msg_len = strnlen((char*)pay, MSG_MAXLEN);
		strcpy(url, DEFAULT_URL);
	} else { //address is 10.255.255.255
		msg_len = read_stdin(pay);
		strcpy(url, DEFAULT_URL);
	}
	
    void *ctx = zmq_ctx_new();
	assert(ctx != NULL);
    void *radio = zmq_socket(ctx, ZMQ_RADIO);
	assert(radio != NULL);
	int linger = 1500;
	zmq_setsockopt(radio, ZMQ_LINGER, &linger, sizeof(linger));

    int rc = zmq_connect(radio, url);
    assert(rc > -1);

    char group[] = "default";

	zmq_msg_t msg;
	zmq_msg_init_data (&msg, pay, 255, my_free, NULL);
	zmq_msg_set_group(&msg, group);
	zmq_sendmsg(radio, &msg, 0); 

    printf("Sent\n");
    zmq_close(radio);
    zmq_ctx_term(ctx);
    return 0;
}