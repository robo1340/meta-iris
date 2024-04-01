#include "serial.h"

// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
//#include <sys/ioctl.h>

//#define SERIAL_DEBUG

int serial_open(char * path){
	#ifdef SERIAL_DEBUG
	printf("serial_open()\n");
	#endif
	
	//int to_return = open(path, O_RDWR);
	int to_return = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (to_return < 0){return to_return;}

	struct termios tty; // Create new termios struct, we call it 'tty' for convention

	if(tcgetattr(to_return, &tty) != 0) { // Read in existing settings, and handle any error
		printf("ERROR: %i from tcgetattr: %s\n", errno, strerror(errno));
		return -1;
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size 
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	tty.c_cc[VTIME] = 0;
	//tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	cfsetispeed(&tty, B115200); // Set in/out baud rate to be 115200
	cfsetospeed(&tty, B115200);

	if (tcsetattr(to_return, TCSANOW, &tty) != 0) { // Save tty settings, also checking for error
		printf("ERROR: %i from tcsetattr: %s\n", errno, strerror(errno));
		return -1;
	}
	
	return to_return;
}

int serial_write(int port, uint8_t * buf, size_t len){
	#ifdef SERIAL_DEBUG
	printf("serial_write()\n");
	#endif
	int rc = write(port, buf, len);
	return rc;
}

int serial_read(int port, uint8_t * buf, size_t len){
	#ifdef SERIAL_DEBUG
	printf("serial_read()\n");
	#endif
	return read(port, buf, len);
}

void serial_close(int port){
	#ifdef SERIAL_DEBUG
	printf("serial_close()\n");
	#endif
	close(port);
}