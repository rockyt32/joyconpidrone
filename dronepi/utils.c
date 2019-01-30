#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include "utils.h"


int serial_open(char *device, int baudrate){
	int fd;
	speed_t baud;
	struct termios options;

	fd = open(device, O_RDWR | O_NOCTTY | O_SYNC); //O_NONBLOCK

	if(fd < 0){
		perror("open");
		return -1;
	}

	switch (baudrate) {
	case      0:	baud = B0; break;
	case      50:	baud = B50; break;
    case      75:	baud = B75; break;
    case     110:	baud = B110; break;
	case     134:	baud = B134; break;
    case     150:	baud = B150; break;
    case     200:	baud = B200; break;
    case     300:	baud = B300; break;
    case    1200:	baud = B1200; break;
	case    1800:	baud = B1800; break;
    case    2400:	baud = B2400; break;
    case    4800:	baud = B4800; break;
    case    9600:	baud = B9600; break;
    case   19200:	baud = B19200; break;
    case   38400:	baud = B38400; break;
    case   57600:	baud = B57600; break;
    case  115200:	baud = B115200; break;
    case  230400:	baud = B230400; break;
	default: fprintf(stderr, "Invalid baud rate \n");
			 return -1;
	}

	fcntl(fd, F_SETFL, 0);

	tcgetattr(fd, &options);

	cfsetispeed(&options, baud);
	cfsetospeed(&options, baud);

	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB; /* no parity */
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CRTSCTS;

	/* raw input */
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	/* no sofware flow control */
	options.c_iflag &= ~(IXON | IXOFF | IXANY);

	/* raw output */
	options.c_oflag &= ~OPOST;

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 10; /* 1 second read timeout */

	if(tcsetattr(fd, TCSANOW, &options) != 0){
		perror("tcsetattr");
		return -1;
	}

	return fd;
}

int msp_set_raw_rc(int fd, msp_rc *rc_data){
	uint8_t checksum = 0;
	uint8_t command[32];

	if(fd < 0) {
		return -1;
	}

	command[0] = '$'; // $
	command[1] = 'M'; // M
	command[2] = '<'; // <
	command[3] = 16; /* Sending 16 bytes of data */
	command[4] = 200;
	checksum = command[3] ^ command[4];
	command[5] = (rc_data->roll) & (0x00FF);
	checksum ^= command [5];
	command[6] = ((rc_data->roll) >> 8) & (0x00FF);
	checksum ^= command [6];
	command[7] = (rc_data->pitch) & (0x00FF);
	checksum ^= command [7];
	command[8] = ((rc_data->pitch) >> 8) & (0x00FF);
	checksum ^= command [8];
	command[9] = (rc_data->throttle) & (0x00FF);
	checksum ^= command [9];
	command[10] = ((rc_data->throttle) >> 8) & (0x00FF);
	checksum ^= command [10];
	command[11] = (rc_data->yaw) & (0x00FF);
	checksum ^= command [11];
	command[12] = ((rc_data->yaw) >> 8) & (0x00FF);
	checksum ^= command [12];
	command[13] = (rc_data->aux1) & (0x00FF);
	checksum ^= command [13];
	command[14] = ((rc_data->aux1) >> 8) & (0x00FF);
	checksum ^= command [14];
	command[15] = (rc_data->aux2) & (0x00FF);
	checksum ^= command [15];
	command[16] = ((rc_data->aux2) >> 8) & (0x00FF);
	checksum ^= command [16];
	command[17] = (rc_data->aux3) & (0x00FF);
	checksum ^= command [17];
	command[18] = ((rc_data->aux3) >> 8) & (0x00FF);
	checksum ^= command [18];
	command[19] = (rc_data->aux4) & (0x00FF);
	checksum ^= command [19];
	command[20] = ((rc_data->aux4) >> 8) & (0x00FF);
	checksum ^= command [20];
	command[21] = checksum;

	if(write(fd, command, 22) < 0){
		perror("write");
		return -1;
	}
	
	return 0;
}
