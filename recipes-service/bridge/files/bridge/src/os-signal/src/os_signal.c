#include "os_signal.h"

int os_interrupted = 0;
static void os_signal_handler (int signal_value){
    os_interrupted = 1;
}

void os_catch_signals (void){
    struct sigaction action;
    action.sa_handler = os_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}