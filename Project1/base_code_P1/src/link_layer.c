// Link layer protocol implementation

#include "../include/link_layer.h"

int alarmCount = 0;
int alarmEnabled = FALSE;

struct termios oldtio;
struct termios newtio;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("--- Alarm #%d ---\n", alarmCount);
}

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    unsigned char byte;
    int state = STATE_START;
    int timeout = connectionParameters.timeout;
    int alarmCount = connectionParameters.nRetransmissions;

    int fd = openSerialPort(connectionParameters.serialPort, &oldtio, &newtio);
    if (fd < 0) return -1;
    
    switch(connectionParameters.role) {
        case LlTx:
        unsigned char buf[5] = {0}; //set up buffer

        buf[0] = FLAG;
        buf[1] = ADDRESS_S;
        buf[2] = CONTROL_SET;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;
        
        (void)signal(SIGALRM, alarmHandler);

        while (state != STATE_STOP && alarmCount <= timeout){

            if (alarmEnabled == FALSE){
                int bytes = write(fd, buf, 5);
                sleep(1);
                printf("SET sent!");

                alarm(0);
                alarmEnabled = TRUE;
                printf("Alarm!")
            }

            if (read(fd, byte, 1) > 0){
                switch (state){

                case STATE_START:
                if (byte == FLAG) state = STATE_FLAG_RCV;
                break;

                case STATE_FLAG_RCV:
                if (byte == ADDRESS_R)
                    state = STATE_A_RCV;
                else if (byte == FLAG)
                    state = STATE_FLAG_RCV;
                else
                    state = STATE_START;
                break;

                case STATE_A_RCV:
                if (byte == CONTROL_UA)
                    state = STATE_C_RCV;
                else if (byte == FLAG)
                    state = STATE_FLAG_RCV;
                else
                    state = STATE_START;
                break;

                case STATE_C_RCV:
                if (byte == (ADDRESS_R ^ CONTROL_UA))
                    state = STATE_BCC_OK;
                else if (byte == FLAG)
                    state = STATE_FLAG_RCV;
                else
                    state = STATE_START;
                break;

                case STATE_BCC_OK:
                if (byte == FLAG)
                {
                    state = STATE_STOP;
                    printf("Received UA!\n");
                }
                else
                    state = STATE_START;
                break;

            default:
                printf("ERROR: No state! \n");
                break;
            }
        }
        }
        break;
        
        case LlRx:
        unsigned char buf[5] = {0}; 
        buf[0] = FLAG;
        buf[1] = ADDRESS_R;
        buf[2] = CONTROL_UA;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;
        
        while (state != STATE_STOP){

        if(read(fd, byte, 1) > 0) {
            switch(state) {
                case STATE_START:
                
                if (byte == FLAG) state = STATE_FLAG_RCV;
                
                break;
                case STATE_FLAG_RCV:
                
                if (byte == ADDRESS_S) state = STATE_A_RCV;
                else if (byte == FLAG) state = STATE_FLAG_RCV;
                else state = STATE_START;

                break;
                case STATE_A_RCV:
                
                if (byte == CONTROL_SET) state = STATE_C_RCV;
                else if (byte == FLAG) state = STATE_FLAG_RCV;
                else state = STATE_START;

                break;
                case STATE_C_RCV:

                if (byte == (ADDRESS_S ^ CONTROL_SET)) state = STATE_BCC_OK;
                else if (byte == FLAG) state = STATE_FLAG_RCV;
                else state = STATE_START;

                break;
            case STATE_BCC_OK:
                if (byte == FLAG)
                {
                    state = STATE_STOP;
                    printf("Received SET UP!\n");
                }
                else state = STATE_START;
                break;
            default:
                printf("ERROR: No state with such name\n");
                break;

            }
        }
        }

        write(fd,buf,5);
        sleep(1);
        break;

        default:
            return -1;
            break;
    }

    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
