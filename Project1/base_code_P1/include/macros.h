// STATE MACHINE

#define STATE_START 0
#define STATE_RCV 1
#define STATE_A_RCV 2
#define STATE_C_RCV 3
#define STATE_DATA 4
#define STATE_ESC_FOUND 5
#define STATE_BCC_OK 6
#define STATE_STOP 7

#define FLAG 0x7E
#define ESC 0x7D
#define AFTER_ESC 0x5D
#define AFTER_FLAG 0x5E
#define ADDRESS_S 0x03
#define ADDRESS_R 0x01

#define CONTROL_SET 0x03
#define CONTROL_UA 0x07
#define CONTROL_RR (N) ((N<< 7) || 0x05)
#define CONTROL_REJ (N) ((N << 7) || 0x01)
#define CONTROL_INF(N) (N << 6)
#define CONTROL_DISC 0x0B
