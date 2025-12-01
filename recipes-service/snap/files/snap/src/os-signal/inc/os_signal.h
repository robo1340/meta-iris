#ifndef _OS_SIGNAL_H
#define _OS_SIGNAL_H

#include <stdint.h>
#include <signal.h>

extern int os_interrupted;

//  Signal handling ------------------------------------------------
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if s_interrupted is ever 1. Works especially well with 
//  zmq_poll.
void os_catch_signals(void);

#endif 
