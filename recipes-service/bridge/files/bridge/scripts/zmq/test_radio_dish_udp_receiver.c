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

#define URL "udp://*:1337"

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

//main loop entry point for the republisher app
int main(int argc, char *argv[]){
    void * ctx = zmq_ctx_new();
    void * dish = zmq_socket(ctx, ZMQ_DISH);

    int rc = zmq_bind(dish, URL);
    assert(rc > -1);

    char group[] = "default";
    zmq_join(dish, group);

    printf( "Waiting for dish message\n" );
	s_setup_pipe(pipefds);
	
	zmq_pollitem_t items [] = {
        { 0, pipefds[0], ZMQ_POLLIN, 0 },
		{dish, 0, ZMQ_POLLIN, 0}
    };
	
	while(1) {
        rc = zmq_poll(items, 2, 500); //poll only the first two sockets
        if (rc<0){ //a zmq_poll error occurred 
            if (errno == EINTR) {continue;}
            printf("zmq poll error occured %d, %s\n",errno,strerror(errno));
            break; 
        }
		
		//signal pipe fd triggered zmq_poll
		if (items[0].revents & ZMQ_POLLIN) {
			char buffer[1];
			read(pipefds[0], buffer, 1); //clear notifying byte
			printf("interrupt received, exiting\n");
			break;
		}
		
		
		//if the subscriber socket had an event occur
		if (items[1].revents & ZMQ_POLLIN){
			zmq_msg_t recv_msg;
			zmq_msg_init (&recv_msg);
			zmq_recvmsg (dish, &recv_msg, 0);
			printf("%s", (char*)zmq_msg_data(&recv_msg));
			printf("\n");

			zmq_msg_close (&recv_msg);
		}
		
    }
	
	zmq_close(dish);
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