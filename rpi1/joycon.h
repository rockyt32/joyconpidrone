#if !defined(JOYCON_H)
#define JOYCON_H

#define JOYCON_L_BT (0x2006)
#define JOYCON_R_BT (0x2007)
#define PRO_CONTROLLER (0x2009)
#define JOYCON_CHARGING_GRIP (0x200e)
//#define DEBUG_PRINT 
//Uncomment to enable debug printing

void hex_dump(unsigned char *buf, int len);

void hid_exchange(hid_device *handle, unsigned char *buf, int len);

int hid_dual_exchange(hid_device *handle_l, hid_device *handle_r, 
					unsigned char *buf_l, unsigned char *buf_r, int len);

void joycon_send_command(hid_device *handle, int command, uint8_t *data, 
						int len);

void joycon_send_subcommand(hid_device *handle, int command, int subcommand, 
							uint8_t *data, int len);

void spi_write(hid_device *handle, uint32_t offs, uint8_t *data, uint8_t len);

void spi_read(hid_device *handle, uint32_t offs, uint8_t *data, uint8_t len);

void spi_flash_dump(hid_device *handle, char *out_path);

int joycon_init(hid_device *handle, const wchar_t *name);

void joycon_deinit(hid_device *handle, const wchar_t *name);

void device_print(struct hid_device_info *dev);

#endif
