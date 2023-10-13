// STATE MACHINE

#define STATE_START 0
#define STATE_FLAG_RCV 1
#define STATE_A_RCV 2
#define STATE_C_RCV 3
#define STATE_BCC_OK 4
#define STATE_STOP 5

#define FLAG 0x7E
#define ADDRESS_S 0x03
#define ADDRESS_R 0x01
#define CONTROL_SET 0x03
#define CONTROL_UA 0x07