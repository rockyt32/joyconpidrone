
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include "utils.h"


#define PORT "32064"  // the port users will be connecting to
#define BACKLOG 50     // how many pending connections queue will hold
#define POWER 0.5
#define THROTTLE_BASE 1150

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, client_fd;  // listen on sock_fd, new connection on new_fd
	int serial_fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char numbytes;
	char hostname[128];
	char command;
	char home_button, a_button, vert_down, hoz_left, vert_value, hoz_value;
	msp_rc rc_data;	

    /* setup server */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	gethostname(hostname, 128);

	printf("Hostname: %s \n", hostname);
	
	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {

		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}


		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		close(sockfd);
		return 1;
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		close(sockfd);
		return 1;
	}

	printf("server: waiting for connections...\n");

	sin_size = sizeof their_addr;
	client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
	if (client_fd == -1) {
		perror("accept");
		close(sockfd);
		return 1;
	}

	inet_ntop(their_addr.ss_family,
		get_in_addr((struct sockaddr *)&their_addr),
		s, sizeof s);
	printf("server: got connection from %s\n", s);

	close(sockfd);  
	/* server setup complete */

	/* set up serial port */
	serial_fd = serial_open("/dev/ttyUSB0", 115200); 
	if(serial_fd == -1){
		close(client_fd);
		return 1;
	}
	sleep(1);
	printf("Serial connection established \n");
	/* set up rc data and arm drone */
	rc_data.roll = 1500;
	rc_data.pitch = 1500;
	rc_data.yaw = 1500;
	rc_data.throttle = 1000;
	rc_data.aux1 = 1000;
	rc_data.aux2 = 1000;
	rc_data.aux3 = 1500;
	rc_data.aux4 = 1500;
	msp_set_raw_rc(serial_fd, &rc_data);
	usleep(5000);
	msp_set_raw_rc(serial_fd, &rc_data);
	sleep(1);
	printf("Armed \n");

	numbytes = 1;
	command = 0;
	while (numbytes > 0) {
		numbytes = recv(client_fd, &command, 1, 0);
		if (numbytes == 0) {
			rc_data.aux3 = 2000;
			printf("Connection closed\n");
		}
		else if (numbytes == -1) {
			perror("recv");
			close(client_fd);
			rc_data.roll = 1500;
			rc_data.pitch = 1500;
			rc_data.yaw = 1500;
			rc_data.throttle = 1000;
			rc_data.aux1 = 1500;
			rc_data.aux2 = 1500;
			rc_data.aux3 = 2000;
			rc_data.aux4 = 1500;
			msp_set_raw_rc(serial_fd, &rc_data);
			close(serial_fd);
			return 1;
		}
		else {
			home_button = (command >> 7) & 0x01;
			if(home_button){ //done
				break;
			}
			
			a_button = (command >> 6) & 0x01;
			vert_down = (command >> 5) & 0x01;
			hoz_left = (command >> 2) & 0x01;
			vert_value = (command >> 3) & 0x03;
			hoz_value = command & 0x03;

			rc_data.roll = 1500;
			rc_data.pitch = 1500;
			rc_data.yaw = 1500;
			rc_data.throttle = THROTTLE_BASE;
			if(a_button){
				// set forward going value
				rc_data.pitch = 1500 - (400 * POWER);
			}
			if(vert_down){
				rc_data.throttle = THROTTLE_BASE - (50 * vert_value * POWER);
			} 
			else {
				rc_data.throttle = THROTTLE_BASE + (200 * vert_value * POWER);
			}
			if(hoz_left){
				rc_data.yaw = 1500 - (166 * hoz_value * POWER);
			} 
			else{
				rc_data.yaw = 1500 + (166 * hoz_value * POWER);
			}

			// put componenents together and send in msp
			if(msp_set_raw_rc(serial_fd, &rc_data) < 0){
				close(client_fd);
				rc_data.roll = 1500;
				rc_data.pitch = 1500;
				rc_data.yaw = 1500;
				rc_data.throttle = 1000;
				rc_data.aux1 = 1500;
				rc_data.aux2 = 1500;
				rc_data.aux3 = 2000;
				rc_data.aux4 = 1500;
				msp_set_raw_rc(serial_fd, &rc_data);
				close(serial_fd);
				return 1;
			}
			/*printf("Throttle: %d \n", rc_data.throttle);
			printf("Yaw: %d \n", rc_data.yaw);
			printf("Pitch: %d \n", rc_data.pitch); */
		}
		
	}

	printf("Done \n");
	

	// clean up and disarm drone
	close(client_fd);
	rc_data.roll = 1500;
	rc_data.pitch = 1500;
	rc_data.yaw = 1500;
	rc_data.throttle = 1000;
	rc_data.aux1 = 1500;
	rc_data.aux2 = 1500;
	rc_data.aux4 = 1500;
	if(msp_set_raw_rc(serial_fd, &rc_data) < 0){
		close(serial_fd);
		return 1;
	}
	usleep(5000);
	msp_set_raw_rc(serial_fd, &rc_data);
	usleep(10000);
	close(serial_fd);


	return 0;
}
