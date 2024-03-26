#ifndef _SERIAL_H
#define _SERIAL_H

#include <stdint.h>
#include <stdlib.h>

int serial_open(char * path);

int serial_write(int port, uint8_t * buf, size_t len);

int serial_read(int port, uint8_t * buf, size_t len);

void serial_close(int port);

#endif 






