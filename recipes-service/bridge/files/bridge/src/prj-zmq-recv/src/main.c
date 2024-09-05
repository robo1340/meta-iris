#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h> // for usleep
#define ZMQ_BUILD_DRAFT_API
#include <zmq.h>
#include <stdint.h>
#include <stdbool.h>

#define URL "udp://*:1337"
#define BROADCAST_GROUP "broadcast"

//  Signal handling
//
//  Create a self-pipe and call s_catch_signals(pipe's writefd) in your application
//  at startup, and then exit your main loop if your pipe contains any data.
//  Works especially well with zmq_poll.
#define S_NOTIFY_MSG " "
#define S_ERROR_MSG "Error while writing to self-pipe.\n"
static int s_fd;
static void s_signal_handler (int signal_value);
static void s_catch_signals (int fd);
static void s_setup_pipe(int * pipefds);
int pipefds[2];

#define HEADER "=========================\n"

static bool handle_msg(zmq_msg_t * msg, void * pub){
	static char group[32];
	static char cmd[2000];
        if ((strcmp(zmq_msg_group(msg),"location")!=0)&&
            (strcmp(zmq_msg_group(msg),"newMessage")!=0)&&
			(strcmp(zmq_msg_group(msg),"waypoint")!=0))		
	{	
		snprintf(cmd, sizeof(cmd), "wall -n \""HEADER"message received on chat group \"%s\"\n%s\"", zmq_msg_group(msg), (char*)zmq_msg_data(msg));
		system(cmd);
	}
	
	strncpy(group, zmq_msg_group(msg), sizeof(group));
	
	zmq_send(pub, group, sizeof(group), ZMQ_SNDMORE);
	zmq_send(pub, zmq_msg_data(msg), zmq_msg_size(msg), 0);
	
	return true;
}


int main(int argc, char *argv[]){
    void * ctx = zmq_ctx_new();
    void * dish = zmq_socket(ctx, ZMQ_DISH);

    int rc = zmq_bind(dish, URL);
    assert(rc > -1);
	
	void * pub = zmq_socket(ctx, ZMQ_PUB);
	assert(pub != NULL);
	zmq_bind(pub, "ipc:///tmp/chat.ipc");
	
	zmq_join(dish, BROADCAST_GROUP);
	zmq_join(dish, "location");
	zmq_join(dish, "newMessage");
	zmq_join(dish, "waypoint");
	zmq_join(dish, "test");
	zmq_join(dish, "group1");
	zmq_join(dish, "group2");
	zmq_join(dish, "group3");
	if (argc > 1){
		int i;
		for (i=1; i<argc; i++){
			zmq_join(dish, argv[i]);
		}
	}
    
    printf("...\n" );
	s_setup_pipe(pipefds);
	
	zmq_pollitem_t items [] = {
        { 0,   pipefds[0], ZMQ_POLLIN, 0},
		{dish, 0,          ZMQ_POLLIN, 0}
    };
	
	while(1) {
        rc = zmq_poll(items, 2, 1000); //poll only the first two sockets
        if (rc<0){ //a zmq_poll error occurred 
            if (errno == EINTR) {continue;}
            printf("zmq poll error occured %d, %s\n",errno,strerror(errno));
            break; 
        }
		
		
		if (items[0].revents & ZMQ_POLLIN) { //signal pipe fd triggered zmq_poll
			char buffer[1];
			read(pipefds[0], buffer, 1); //clear notifying byte
			printf("interrupt received, exiting\n");
			break;
		}
		if (items[1].revents & ZMQ_POLLIN){ //message received
			zmq_msg_t recv_msg;
			zmq_msg_init (&recv_msg);
			zmq_recvmsg (dish, &recv_msg, 0);
			if (!handle_msg(&recv_msg, pub)){
				printf("ERROR: handle_msg() failed\n");
			}
			zmq_msg_close (&recv_msg);
		}
    }
	zmq_close(dish);
	zmq_close(pub);
	zmq_ctx_term(ctx);
	return 0;
}

static void s_setup_pipe(int * pipefds) {
	int rc = pipe(pipefds);
	if (rc != 0) {
		perror("Creating self-pipe");
		exit(1);
	}
	for (int i=0; i<2; i++){
        int flags = fcntl(pipefds[i], F_GETFL, 0);
        if (flags < 0) {
            perror ("fcntl(F_GETFL)");
            exit(1);
        }
        rc = fcntl (pipefds[i], F_SETFL, flags | O_NONBLOCK);
        if (rc != 0) {
            perror ("fcntl(F_SETFL)");
            exit(1);
        }
	}
   	s_catch_signals (pipefds[1]); //catch ctrl+c so the program will be able to cleanup and exit cleanly
}
	
//  Signal handling
//
//  Create a self-pipe and call s_catch_signals(pipe's writefd) in your application
//  at startup, and then exit your main loop if your pipe contains any data.
//  Works especially well with zmq_poll.
static void s_signal_handler (int signal_value) {
    int rc = write (s_fd, S_NOTIFY_MSG, sizeof(S_NOTIFY_MSG));
    if (rc != sizeof(S_NOTIFY_MSG)) {
        write (STDOUT_FILENO, S_ERROR_MSG, sizeof(S_ERROR_MSG)-1);
        exit(1);
    }
}

static void s_catch_signals (int fd) {
    s_fd = fd;

    struct sigaction action;
    action.sa_handler = s_signal_handler;
    //  Doesn't matter if SA_RESTART set because self-pipe will wake up zmq_poll
    //  But setting to 0 will allow zmq_read to be interrupted.
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}
