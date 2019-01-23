#ifdef WIN32
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "hidapi.h"
#include "joycon.h"

#define PORT "32064" // the port to connect to the server

unsigned short product_ids_size = 4;
unsigned short product_ids[] = {JOYCON_L_BT, JOYCON_R_BT, PRO_CONTROLLER, JOYCON_CHARGING_GRIP};

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int res;
	int res2;
	unsigned char *stick_data;
	unsigned char stick_hoz;
	unsigned char stick_vert;
	unsigned char a_button;
	unsigned char home_button;
	char command;
    unsigned char buf[2][0x400] = {0};
    hid_device *handle_l = 0, *handle_r = 0;
    const wchar_t *device_name = L"none";
    struct hid_device_info *devs, *dev_iter;
    bool charging_grip = false;
	int i;
	int hoz, vert, hoz_base, vert_base, is_neg;

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char bytes_sent;
	char s[INET6_ADDRSTRLEN];
    
    //setbuf(stdout, NULL); // turn off stdout buffering for test reasons
	if (argc != 2) {
		fprintf(stderr, "usage: <server name> \n");
		exit(1);
	}

    res = hid_init();
    if(res)
    {
        printf("Failed to open hid library! Exiting...\n");
        return -1;
    }

    // iterate thru all the valid product ids and try and initialize controllers
    for(i = 0; i < product_ids_size; i++)
    {
		// 0x057E is the vendor ID
        devs = hid_enumerate(0x057E, product_ids[i]);
        dev_iter = devs;
        while(dev_iter)
        {
            // Sometimes hid_enumerate still returns other product IDs
            if (dev_iter->product_id != product_ids[i]) break;
            
            // break out if the current handle is already used
            if((dev_iter->product_id == JOYCON_R_BT || dev_iter->interface_number == 0) && handle_r)
                break;
            else if((dev_iter->product_id == JOYCON_L_BT || dev_iter->interface_number == 1) && handle_l)
                break;
            
            device_print(dev_iter);
            
			
            if(!wcscmp(dev_iter->serial_number, L"000000000001"))
            {
                printf("Only bluetooth devices are supported \n");
				return -1;
            }
            
            
            // on windows this will be -1 for devices with one interface
            if(dev_iter->interface_number == 0 || dev_iter->interface_number == -1)
            {
                hid_device *handle = hid_open_path(dev_iter->path);
 
                if(handle == NULL)
                {
                    printf("Failed to open controller at %ls, continuing...\n", dev_iter->path);
                    dev_iter = dev_iter->next;
                    continue;
                }
                
                switch(dev_iter->product_id)
                {
                    case JOYCON_CHARGING_GRIP:
                        device_name = L"Joy-Con (R)";
                        charging_grip = true;

                        handle_r = handle;
                        break;
                    /*case PRO_CONTROLLER:
                        device_name = L"Pro Controller";

                        handle_l = handle;
                        break;*/
                    case JOYCON_L_BT:
                        device_name = L"Joy-Con (L)";
						printf("Here \n");
                        handle_l = handle;
                        break;
                    case JOYCON_R_BT:
                        device_name = L"Joy-Con (R)";

                        handle_r = handle;
                        break;
					case PRO_CONTROLLER:
						printf("Pro controller type not supported at the moment\n");
						hid_close(handle);
						hid_exit();
						return -1;
						
                }
                
                if(joycon_init(handle, device_name))
                {
                    hid_close(handle);
                    if(dev_iter->product_id != JOYCON_L_BT)
                        handle_r = NULL;
                    else
                        handle_l = NULL;
                }
            }
            // Only exists for left Joy-Con in the charging grip
            else if(dev_iter->interface_number == 1)
            {
                handle_l = hid_open_path(dev_iter->path);
                if(handle_l == NULL)
                {
                    printf("Failed to open controller at %ls, continuing...\n", dev_iter->path);
                    dev_iter = dev_iter->next;
                    continue;
                }
                
                if(joycon_init(handle_l, L"Joy-Con (L)"))
                    handle_l = NULL;
            }
            dev_iter = dev_iter->next;
        }
        hid_free_enumeration(devs);
    }
    
    if(!handle_l)
    {
        printf("Failed to get handle for left Joy-Con or Pro Controller, exiting...\n");
        return -1;
    }
    
    // Need both joycons
    if(!handle_r)
    {
        printf("Could not get handles for both Joy-Cons! Exiting...\n");
		joycon_deinit(handle_l, L"Joy-Con (L)");
		hid_close(handle_l);		
		hid_exit();
        return -1;
    }
    // controller init is complete at this point
	// set up connection to server
	#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed.\n");
		joycon_deinit(handle_l, L"Joy-Con (L)");
		joycon_deinit(handle_r, L"Joy-Con (R)");
		hid_close(handle_l);
		hid_close(handle_r);
		hid_exit();
		return 1;
	}
	#endif

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		#ifdef WIN32
		WSACleanup();
		#endif
		joycon_deinit(handle_l, L"Joy-Con (L)");
		joycon_deinit(handle_r, L"Joy-Con (R)");
		hid_close(handle_l);
		hid_close(handle_r);
		hid_exit();
		return 1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			#ifdef WIN32
			closesocket(sockfd);
			#else
			close(sockfd);
			#endif
			perror("client: connect");
			continue;
		}
	
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		#ifdef WIN32
		closesocket(sockfd);
		WSACleanup();
		#else
		close(sockfd);
		#endif
		joycon_deinit(handle_l, L"Joy-Con (L)");
		joycon_deinit(handle_r, L"Joy-Con (R)");
		hid_close(handle_l);
		hid_close(handle_r);
		hid_exit();
		return 1;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
		s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
	// done setting up connection to server

	//hid_set_nonblocking(handle, 1);

	// Vibration 
	memset(buf[0], 0x00, 0x400);
	buf[0][2] = 0x1;
	buf[0][1] = 5;
    buf[0][5] = 5;
	joycon_send_command(handle_l, 0x10, (uint8_t*)buf, 0x9);
	//usleep(10000);
	//joycon_send_command(handle_r, 0x10, (uint8_t*)buf, 0x9);
	#ifdef WIN32
	Sleep(50);
	#else
	usleep(50000);
	#endif
	joycon_send_command(handle_l, 0x10, (uint8_t*)buf, 0x9);
	//usleep(10000);
	//joycon_send_command(handle_r, 0x10, (uint8_t*)buf, 0x9);

	stick_data = buf[0] + 6;
	hoz_base = 0;
	vert_base = 0;	

	i = 0;
	while(true){
		memset(buf[0], 0x00, 0x400);
		memset(buf[1], 0x00, 0x400);
		command = 0;
		
		res = joycon_send_subcommand_timeout(handle_l, 0x1, 0x0, buf[0], 0, 3000);
		#ifdef WIN32
		Sleep(8);
		#else
		usleep(8000);
		#endif
		res2 = joycon_send_subcommand_timeout(handle_r, 0x1, 0x0, buf[1], 0, 3000);

		// joycon timed out, close handle and exit
		if(res == -1 || res == 0 || res2 == -1 || res2 == 0){ 
			printf("Joycon timed out... \n");
			#ifdef WIN32
			closesocket(sockfd);
			WSACleanup();
			#else
			close(sockfd);
			#endif
			joycon_deinit(handle_l, L"Joy-Con (L)");
			joycon_deinit(handle_r, L"Joy-Con (R)");
			hid_close(handle_l);
			hid_close(handle_r);
			hid_exit();
			return -1;
		}

		if(buf[0][2] == 0x8e){ // data only valid if it's 8e
			//hex_dump(buf[0], 0x3D);
			stick_hoz = ((stick_data[1] & 0x0F) << 4) | ((stick_data[0] & 0xF0) >> 4);
			stick_vert = stick_data[2];
				
			hoz = (-128 + (int)(unsigned int) stick_hoz) - hoz_base;
			vert = (-128 + (int)(unsigned int) stick_vert) - vert_base;
			printf("Horizontal stick: %d \n" , (int) hoz);
			printf("Vertical stick: %d \n" , (int) vert);
			// hoz: -80 to 80
			// vert: -60 to 80 offset 13/14

			if(i < 3){
				i++;
			}
			if (i == 3) {
				hoz_base = hoz;
				vert_base = vert;
				i++;
			}

			// Fill command to send for hoz stick
			is_neg = 0;
			if(hoz < 0){
				//printf("aa \n");
				hoz = hoz * -1;
				is_neg = 1;
			}
			hoz = hoz / 21;
			if(hoz > 3){
				hoz = 3;
			}
			command = command | (hoz & 0x03);
			command = command | (is_neg << 2);

			// Fill command to send for vert stick
			is_neg = 0;
			if(vert < 0){
				vert = vert * -1;
				is_neg = 1;
			}
			vert = vert / 21;
			if(vert > 3){
				vert = 3;
			}
			command = command | ((vert & 0x03) << 3);
			command = command | (is_neg << 5);		
		}

		if(buf[1][2] == 0x8e){
			/*printf("Right joycon \n");
			hex_dump(buf[1], 0x3D); */
			a_button = (buf[1][3] == 0x08)? 1: 0;
			home_button = (buf[1][4] == 0x10)? 1: 0;
			//printf("A button: %d \n" , a_button);
			//printf("Home button: %d \n" , home_button);
			command = command | (a_button << 6);
			command = command | (home_button << 7);
		}
		
		//send command
		bytes_sent = send(sockfd, &command, 1, 0);
		if (bytes_sent == -1) {
			perror("send");
			#ifdef WIN32
			closesocket(sockfd);
			WSACleanup();
			#else
			close(sockfd);
			#endif
			return 1;
		}

		if (home_button) {
			break;
		}

		#ifdef WIN32
		Sleep(8);
		#else
		usleep(8000);
		#endif
	}
	

    if(handle_l)
    {
        joycon_deinit(handle_l, L"Joy-Con (L)");
        hid_close(handle_l);
    }
    
    if(handle_r)
    {
        joycon_deinit(handle_r, device_name);
        hid_close(handle_r);
    }

    // Free the hidapi library
    res = hid_exit();

	// close connection
	#ifdef WIN32
	closesocket(sockfd);
	WSACleanup();
	#else
	close(sockfd);
	#endif

    return 0;
}
