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
#define BROADCAST_GROUP "broadcast"

void my_free (void *data, void *hint)
{
    free (data);
}

static uint16_t read_stdin(uint8_t * msg){
	uint16_t i;
	for (i=0; i<MSG_MAXLEN; i++){
		fread(&msg[i], sizeof(char), 1, stdin);	
		if ((int8_t)msg[i] == EOF){
			if (i > 0) {i--;}
			break;
		}
	}
	return i;
}

static void print_usage(void){
	printf("radio chat program supporting file redirects\n");
	printf("usage: send <msg> <group> <address>\n");
	printf("examples\n");
	printf("send \"hello world\" group1 10.255.255.255\n");
	printf("send \"hello world\" group2 \n");
	printf("send < hello.txt\n");
}

int main(int argc, char *argv[]) {
	char url[64];
	char group[64];
	uint8_t * pay = malloc(MSG_MAXLEN);
	assert(pay != NULL);
	memset(pay, 0, MSG_MAXLEN);
	uint16_t msg_len;

	if (argc >= 4){ //arg 1 is the msg, arg 2 is the group, arg3 is the address
		snprintf(url, sizeof(url), "udp://%s:1337", argv[3]);
		printf("%s\n",url);
	        strncpy(group, argv[2], sizeof(group));	
		strncpy((char*)pay, argv[1], MSG_MAXLEN);
		msg_len = strnlen((char*)pay, MSG_MAXLEN);
        }
	if (argc >= 3){ //arg 1 is the message, arg2 is the group
	        strncpy(group, argv[2], sizeof(group));	
		strncpy((char*)pay, argv[1], MSG_MAXLEN);
		msg_len = strnlen((char*)pay, MSG_MAXLEN);
		strcpy(url, DEFAULT_URL);
	} 
	else if (argc == 2){ //arg 1 is the message
		strncpy((char*)pay, argv[1], MSG_MAXLEN); 
		msg_len = strnlen((char*)pay, MSG_MAXLEN);
		strcpy(url, DEFAULT_URL);
	        strcpy(group, BROADCAST_GROUP);	
	} else { //address is 10.255.255.255
		print_usage();
		return 0;
		msg_len = read_stdin(pay);
		strcpy(url, DEFAULT_URL);
	        strcpy(group, BROADCAST_GROUP);	
	}
	
    void *ctx = zmq_ctx_new();
	assert(ctx != NULL);
    void *radio = zmq_socket(ctx, ZMQ_RADIO);
	assert(radio != NULL);
	int linger = 1500;
	zmq_setsockopt(radio, ZMQ_LINGER, &linger, sizeof(linger));

    int rc = zmq_connect(radio, url);
    assert(rc > -1);

	zmq_msg_t msg;
	zmq_msg_init_data (&msg, pay, 255, my_free, NULL);
	zmq_msg_set_group(&msg, group);
	zmq_sendmsg(radio, &msg, 0); 

    printf("Sent\n");
    zmq_close(radio);
    zmq_ctx_term(ctx);
    return 0;
}
