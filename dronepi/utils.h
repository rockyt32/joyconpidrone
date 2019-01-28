#if !defined(UTILS_H)
#define UTILS_H

#define MSP_SET_RAW_RC 200

typedef struct {
	uint16_t roll;
	uint16_t pitch;
	uint16_t yaw;
	uint16_t throttle;
	uint16_t aux1;
	uint16_t aux2;
	uint16_t aux3;
	uint16_t aux4;
} msp_rc;

int serial_open(char *device, int baudrate);


int msp_set_raw_rc(int fd, msp_rc *rc_data);

#endif
