#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "hidapi.h"
#include "joycon.h"

unsigned short product_ids_size = 4;
unsigned short product_ids[] = {JOYCON_L_BT, JOYCON_R_BT, PRO_CONTROLLER, JOYCON_CHARGING_GRIP};



int main(){
    int res;
	unsigned char *stick_data;
	unsigned char stick_hoz;
	unsigned char stick_vert;
    unsigned char buf[2][0x400] = {0};
    hid_device *handle_l = 0, *handle_r = 0;
    const wchar_t *device_name = L"none";
    struct hid_device_info *devs, *dev_iter;
    bool charging_grip = false;
	int i;
    
    setbuf(stdout, NULL); // turn off stdout buffering for test reasons

    res = hid_init();
    if(res)
    {
        printf("Failed to open hid library! Exiting...\n");
        return -1;
    }

    // iterate thru all the valid product ids and try and initialize controllers
    for(int i = 0; i < product_ids_size; i++)
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
            
			/*
            if(!wcscmp(dev_iter->serial_number, L"000000000001"))
            {
                bluetooth = false;
            }
            else if(!bluetooth)
            {
                printf("Can't mix USB HID with Bluetooth HID, exiting...\n");
                return -1;
            }
			*/
            
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
                    case PRO_CONTROLLER:
                        device_name = L"Pro Controller";

                        handle_l = handle;
                        break;
                    case JOYCON_L_BT:
                        device_name = L"Joy-Con (L)";
						printf("Here \n");
                        handle_l = handle;
                        break;
                    case JOYCON_R_BT:
                        device_name = L"Joy-Con (R)";

                        handle_r = handle;
                        break;
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
    
    // Only missing one half by this point
    if(!handle_r && charging_grip)
    {
        printf("Could not get handles for both Joy-Con in grip! Exiting...\n");
        return -1;
    }
    
    // controller init is complete at this point
   
	//hid_set_nonblocking(handle, 1);

	// Vibration 
	memset(buf[0], 0x00, 0x400);
	buf[0][2] = 0x1;
	buf[0][1] = 5;
    buf[0][5] = 5;
	joycon_send_command(handle_l, 0x10, (uint8_t*)buf, 0x9);
	usleep(50000);
	joycon_send_command(handle_l, 0x10, (uint8_t*)buf, 0x9);

	stick_data = buf[0] + 6;

	for(i = 0; i <= 10000; i++){
		memset(buf[0], 0x00, 0x400);
		res = joycon_send_subcommand_timeout(handle_l, 0x1, 0x0, buf[0], 0, 3000);

		// joycon timed out, close handle and exit
		if(res == -1 || res == 0){ 
			printf("Joycon timed out... \n");
			joycon_deinit(handle_l, L"Joy-Con (L)");
			hid_close(handle_l);
			hid_exit();
			return -1;
		}
		

	
		if((i % 30) == 0){
			if(buf[0][2] == 0x8e){ // data only valid if it's 8e
				hex_dump(buf[0], 0x3D);
				stick_hoz = ((stick_data[1] & 0x0F) << 4) | ((stick_data[0] & 0xF0) >> 4);;
				stick_vert = stick_data[2];
				printf("Horizontal stick: %d \n" , (-128 + (int)(unsigned int)stick_hoz));
				printf("Vertical stick: %d \n" , (-128 + (int)(unsigned int)stick_vert));
				// hoz: -80 to 80
				// vert: -60 to 80 offset 13/14
			}
		}
		usleep(15000);
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

    return 0;
}
