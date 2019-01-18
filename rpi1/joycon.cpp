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



bool bluetooth = true;
uint8_t global_count = 0;

void hex_dump(unsigned char *buf, int len)
{
    for (int i = 0; i < len; i++)
        printf("%02x ", buf[i]);
    printf("\n");
}

void hid_exchange(hid_device *handle, unsigned char *buf, int len)
{
    if(!handle) return; //TODO: idk I just don't like this to be honest
    
#ifdef DEBUG_PRINT
    hex_dump(buf, len);
#endif
    
    hid_write(handle, buf, len);

    int res = hid_read(handle, buf, 0x400);
#ifdef DEBUG_PRINT
    if(res > 0)
        hex_dump(buf, res);
#endif
}

int hid_exchange_timeout(hid_device *handle, unsigned char *buf, int len, 
						int millis)
{
    if(!handle) return -1; 
    
#ifdef DEBUG_PRINT
    hex_dump(buf, len);
#endif
    
    hid_write(handle, buf, len);

    int res = hid_read_timeout(handle, buf, 0x400, millis);
#ifdef DEBUG_PRINT
    if(res > 0)
        hex_dump(buf, res);
#endif

	return res;
}

int hid_dual_exchange(hid_device *handle_l, hid_device *handle_r, 
					unsigned char *buf_l, unsigned char *buf_r, int len)
{
    int res = -1;
    
    if(handle_l && buf_l)
    {
        hid_set_nonblocking(handle_l, 1);
        res = hid_write(handle_l, buf_l, len);
        res = hid_read(handle_l, buf_l, 0x400);
#ifdef DEBUG_PRINT
        if(res > 0)
            hex_dump(buf_l, res);
#endif
        hid_set_nonblocking(handle_l, 0);
    }
    
    if(handle_r && buf_r)
    {
        hid_set_nonblocking(handle_r, 1);
        res = hid_write(handle_r, buf_r, len);
        //usleep(17000);
        do
        {
            res = hid_read(handle_r, buf_r, 0x400);
        }
        while(!res);
#ifdef DEBUG_PRINT
        if(res > 0)
            hex_dump(buf_r, res);
#endif
        hid_set_nonblocking(handle_r, 0);
    }
    
    return res;
}

void joycon_send_command(hid_device *handle, int command, uint8_t *data, int len)
{
    unsigned char buf[0x400];
    memset(buf, 0, 0x400);
    
    if(!bluetooth)
    {
        buf[0x00] = 0x80;
        buf[0x01] = 0x92;
        buf[0x03] = 0x31;
    }
    
    buf[bluetooth ? 0x0 : 0x8] = command;
    if(data != NULL && len != 0)
        memcpy(buf + (bluetooth ? 0x1 : 0x9), data, len);
    
    hid_exchange(handle, buf, len + (bluetooth ? 0x1 : 0x9));
    
    if(data)
        memcpy(data, buf, 0x40);
}

int joycon_send_command_timeout(hid_device *handle, int command, uint8_t *data, 
								int len, int millis)
{
	int res;
    unsigned char buf[0x400];
    memset(buf, 0, 0x400);
    
    if(!bluetooth)
    {
        buf[0x00] = 0x80;
        buf[0x01] = 0x92;
        buf[0x03] = 0x31;
    }
    
    buf[bluetooth ? 0x0 : 0x8] = command;
    if(data != NULL && len != 0)
        memcpy(buf + (bluetooth ? 0x1 : 0x9), data, len);
    
    res = hid_exchange_timeout(handle, buf, len + (bluetooth ? 0x1 : 0x9), 
								millis);
    
    if(data)
        memcpy(data, buf, 0x40);
	
	return res;
}

void joycon_send_subcommand(hid_device *handle, int command, int subcommand, uint8_t *data, int len)
{
    unsigned char buf[0x400];
    memset(buf, 0, 0x400);
    
    uint8_t rumble_base[9] = {(++global_count) & 0xF, 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40};
    memcpy(buf, rumble_base, 9);
    
    buf[9] = subcommand;
    if(data && len != 0)
        memcpy(buf + 10, data, len);
        
    joycon_send_command(handle, command, buf, 10 + len);
        
    if(data)
        memcpy(data, buf, 0x40); //TODO
}

int joycon_send_subcommand_timeout(hid_device *handle, int command, 
									int subcommand, uint8_t *data, int len, 
									int millis)
{
	int res;
    unsigned char buf[0x400];
    memset(buf, 0, 0x400);
    
    uint8_t rumble_base[9] = {(++global_count) & 0xF, 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40};
    memcpy(buf, rumble_base, 9);
    
    buf[9] = subcommand;
    if(data && len != 0)
        memcpy(buf + 10, data, len);
        
    res = joycon_send_command_timeout(handle, command, buf, 10 + len, millis);
        
    if(data)
        memcpy(data, buf, 0x40); //TODO

	return res;
}

void spi_write(hid_device *handle, uint32_t offs, uint8_t *data, uint8_t len)
{
    unsigned char buf[0x400];
    uint8_t *spi_write = (uint8_t*)calloc(1, 0x26 * sizeof(uint8_t));
    uint32_t* offset = (uint32_t*)(&spi_write[0]);
    uint8_t* length = (uint8_t*)(&spi_write[4]);
   
    *length = len;
    *offset = offs;
    memcpy(&spi_write[0x5], data, len);
   
    int max_write_count = 2000;
    int write_count = 0;
    do
    {
        //usleep(300000);
        write_count += 1;
        memcpy(buf, spi_write, 0x39);
        joycon_send_subcommand(handle, 0x1, 0x11, buf, 0x26);
    }
    while((buf[0x10 + (bluetooth ? 0 : 10)] != 0x11 && buf[0] != (bluetooth ? 0x21 : 0x81))
        && write_count < max_write_count);
	if(write_count > max_write_count)
        printf("ERROR: Write error or timeout\nSkipped writing of %dBytes at address 0x%05X...\n", 
            *length, *offset);
}

void spi_read(hid_device *handle, uint32_t offs, uint8_t *data, uint8_t len)
{
    unsigned char buf[0x400];
    uint8_t *spi_read_cmd = (uint8_t*)calloc(1, 0x26 * sizeof(uint8_t));
    uint32_t* offset = (uint32_t*)(&spi_read_cmd[0]);
    uint8_t* length = (uint8_t*)(&spi_read_cmd[4]);
   
    *length = len;
    *offset = offs;
   
    int max_read_count = 2000;
	int read_count = 0;
    do
    {
        //usleep(300000);
		read_count += 1;
        memcpy(buf, spi_read_cmd, 0x36);
        joycon_send_subcommand(handle, 0x1, 0x10, buf, 0x26);
    }
    while(*(uint32_t*)&buf[0xF + (bluetooth ? 0 : 10)] != *offset && read_count < max_read_count);
	if(read_count > max_read_count)
        printf("ERROR: Read error or timeout\nSkipped reading of %dBytes at address 0x%05X...\n", 
            *length, *offset);

    
    memcpy(data, &buf[0x14 + (bluetooth ? 0 : 10)], len);
}

void spi_flash_dump(hid_device *handle, char *out_path)
{
    unsigned char buf[0x400];
    uint8_t *spi_read_cmd = (uint8_t*)calloc(1, 0x26 * sizeof(uint8_t));
    int safe_length = 0x10; // 512KB fits into 32768 * 16B packets
    int fast_rate_length = 0x1D; // Max SPI data that fit into a packet is 29B. Needs removal of last 3 bytes from the dump.
    
    int length = fast_rate_length;
    
    FILE *dump = fopen(out_path, "wb");
    if(dump == NULL)
    {
        printf("Failed to open dump file %s, aborting...\n", out_path);
        return;
    }
    
    uint32_t* offset = (uint32_t*)(&spi_read_cmd[0x0]);
    for(*offset = 0x0; *offset < 0x80000; *offset += length)
    {
        // HACK/TODO: hid_exchange loves to return data from the wrong addr, or 0x30 (NACK?) packets
        // so let's make sure our returned data is okay before writing

        //Set length of requested data
        spi_read_cmd[0x4] = length;

        int max_read_count = 2000;
        int read_count = 0;
        while(1)
        {
            read_count += 1;
            memcpy(buf, spi_read_cmd, 0x26);
            joycon_send_subcommand(handle, 0x1, 0x10, buf, 0x26);
            
            // sanity-check our data, loop if it's not good
            if((buf[0] == (bluetooth ? 0x21 : 0x81)
                && *(uint32_t*)&buf[0xF + (bluetooth ? 0 : 10)] == *offset) 
                || read_count > max_read_count)
                break;
        }

        if(read_count > max_read_count)
        {
			printf("\n\nERROR: Read error or timeout.\nSkipped dumping of %dB at address 0x%05X...\n\n", 
                length, *offset);
            return;
        }
        fwrite(buf + (0x14 + (bluetooth ? 0 : 10)) * sizeof(char), length, 1, dump);
        
        if((*offset & 0xFF) == 0) // less spam
            printf("\rDumped 0x%05X of 0x80000", *offset);
    }
    printf("\rDumped 0x80000 of 0x80000\n");
    fclose(dump);
}

int joycon_init(hid_device *handle, const wchar_t *name)
{
    unsigned char buf[0x400];
    unsigned char sn_buffer[14] = {0x00};
    memset(buf, 0, 0x400);
    
    
    if(!bluetooth)
    {
        // Get MAC Left
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x01;
        hid_exchange(handle, buf, 0x2);
        
        if(buf[2] == 0x3)
        {
            printf("%ls disconnected!\n", name);
            return -1;
        }
        else
        {
            printf("Found %ls, MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", name,
                   buf[9], buf[8], buf[7], buf[6], buf[5], buf[4]);
        }
            
        // Do handshaking
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x02;
        hid_exchange(handle, buf, 0x2);
        
        printf("Switching baudrate...\n");
        
        // Switch baudrate to 3Mbit
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x03;
        hid_exchange(handle, buf, 0x2);
        
        // Do handshaking again at new baudrate so the firmware pulls pin 3 low?
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x02;
        hid_exchange(handle, buf, 0x2);
        
        // Only talk HID from now on
        memset(buf, 0x00, 0x400);
        buf[0] = 0x80;
        buf[1] = 0x04;
        hid_exchange(handle, buf, 0x2);
    }

    // Enable vibration
    memset(buf, 0x00, 0x400);
    buf[0] = 0x01; // Enabled
    joycon_send_subcommand(handle, 0x1, 0x48, buf, 1);
    
    // Enable IMU data
    memset(buf, 0x00, 0x400);
    buf[0] = 0x01; // Enabled
    joycon_send_subcommand(handle, 0x1, 0x40, buf, 1);
    
    // Increase data rate for Bluetooth
    if (bluetooth)
    {
       memset(buf, 0x00, 0x400);
       buf[0] = 0x31; // Enabled
       joycon_send_subcommand(handle, 0x1, 0x3, buf, 1);
    }
    
    //Read device's S/N
    spi_read(handle, 0x6002, sn_buffer, 0xE);
    
    printf("Successfully initialized %ls with S/N: %c%c%c%c%c%c%c%c%c%c%c%c%c%c!\n", 
        name, sn_buffer[0], sn_buffer[1], sn_buffer[2], sn_buffer[3], 
        sn_buffer[4], sn_buffer[5], sn_buffer[6], sn_buffer[7], sn_buffer[8], 
        sn_buffer[9], sn_buffer[10], sn_buffer[11], sn_buffer[12], 
        sn_buffer[13]);
    
    return 0;
}

void joycon_deinit(hid_device *handle, const wchar_t *name)
{
    unsigned char buf[0x400];
    memset(buf, 0x00, 0x400);

    //Let the Joy-Con talk BT again 
    if(!bluetooth)
    {   
        buf[0] = 0x80;
        buf[1] = 0x05;
        hid_exchange(handle, buf, 0x2);
    }
    
    printf("Deinitialized %ls\n", name);
}

void device_print(struct hid_device_info *dev)
{
    printf("USB device info:\n  vid: 0x%04hX pid: 0x%04hX\n  path: %s\n  MAC: %ls\n  interface_number: %d\n",
        dev->vendor_id, dev->product_id, dev->path, dev->serial_number, dev->interface_number);
    printf("  Manufacturer: %ls\n", dev->manufacturer_string);
    printf("  Product:      %ls\n\n", dev->product_string);
}

