#ifndef _GPIO_H
#define _GPIO_H


#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <czmq.h>

//#define SET_GPIO_EVENT system("touch /tmp/gpio_event")
//#define CLR_GPIO_EVENT zsys_file_delete("/tmp/gpio_event")
//#define CHECK_GPIO_EVENT zsys_file_exists("/tmp/gpio_event")

extern bool gpio_global_event_set;
extern bool gpio_global_event_ack;

/** @brief starts the gpio monitor thread and takes control of configured gpiochips
 *  @param zmq_ctx a valid zmq context pointer
 *  @return returns a pointer to a connected PAIR socket that will notify parent thread of gpio events on success, returns NULL on failure
 */
void * gpio_init(void * zmq_ctx);

void gpio_deinit(void);

#endif 
