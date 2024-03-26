#ifndef _GPIO_H
#define _GPIO_H


#include <stdint.h>
#include <stdbool.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/** @brief starts the gpio monitor thread and takes control of configured gpiochips
 */
bool gpio_init(void);

//returns true when IRQ is asserted
bool gpio_poll(void);

//returns true when IRQ is asserted
bool gpio_get(void);

void gpio_deinit(void);

#endif 
