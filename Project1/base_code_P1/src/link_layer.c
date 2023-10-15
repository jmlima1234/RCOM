// Link layer protocol implementation

#include "../include/link_layer.h"

int alarmCount = 0;
int alarmEnabled = FALSE;
int baudRate = 0;
int nRetransmissions = 0;
int timeout = 0;
int tramaT = 0;

int fd = -1;

struct termios oldtio;
struct termios newtio;

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount--;

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
    timeout = connectionParameters.timeout;
    nRetransmissions = connectionParameters.nRetransmissions;
    alarmCount = nRetransmissions;


    fd = openSerialPort(connectionParameters.serialPort, &oldtio, &newtio);
    if (fd < 0) return -1;

    /* Será preciso isto?
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        return -1;
    }
    */ 
    switch(connectionParameters.role) {
        case LlTx:
        unsigned char buf[5] = {0}; //set up buffer

        buf[0] = FLAG;
        buf[1] = ADDRESS_S;
        buf[2] = CONTROL_SET;
        buf[3] = buf[1] ^ buf[2];
        buf[4] = FLAG;
        
        (void)signal(SIGALRM, alarmHandler);

        while (state != STATE_STOP && alarmCount != 0){

            if (alarmEnabled == FALSE){
                int bytes = write(fd, buf, 5);
                sleep(1);
                printf("SET sent!");

                alarm(timeout);
                alarmEnabled = TRUE;
                printf("Alarm!");
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
                    alarm(0);
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
int llwrite(const unsigned char *buf, int bufSize){
    int frameSize = bufSize + 6; //{FLAG, A, C, BCC1} + Data size + {BCC2, FLAG}
    unsigned char *frame = malloc(frameSize);

    frame[0] = FLAG;
    frame[1] = ADDRESS_S;
    frame[2] = CONTROL_INF(tramaT);
    frame[3] = frame[1] ^ frame[2];

    memcpy(frame + 4, buf, bufSize); //passar a data para o frame

    unsigned char BCC2 = buf[0];
    for(int i = 1; i < bufSize; i++) BCC2 ^= buf[i]; // calcular o bcc2

    for(int i = 4; i < frameSize - 2; i++) { //byte stuffing
        if(frame[i] == FLAG) {
            unsigned char byte1 = 0x7D;
            unsigned char byte2 = 0x5E;

            frameSize++;
            frame = realloc(frame,frameSize);
            memmove(frame + i + 1, frame + i, frameSize - i - 1);

            frame[i++] = byte1;
            frame[i] = byte2;
        }
        else if (frame[i] == ESC) {
            unsigned char byte1 = 0x7D;
            unsigned char byte2 = 0x5D;

            frameSize++;
            frame = realloc(frame,frameSize);
            memmove(frame + i + 1, frame + i, frameSize - i - 1);

            frame[i++] = byte1;
            frame[i] = byte2;
        }
    }

    frame[frameSize-2] = BCC2;
    frame[frameSize-1] = FLAG;
    

    int state = STATE_START;
    alarmCount = nRetransmissions;
    unsigned char Control;

    (void)signal(SIGALRM, alarmHandler);
    
    while (state != STATE_STOP && alarmCount != 0){
        if(alarmEnabled == FALSE) {

            write(fd,frame,frameSize);
            alarm(timeout);
            alarmEnabled = TRUE;
        }    

        //check control
        Control = check_control();

        if (C == C_RR(0) || C == C_RR(1))
        {
            state = STOP;
            tramaT = (tramaT + 1) % 2;
        }
    }
    
    if (state != STATE_STOP) return -1;
    
    free(frame);
    return frameSize;
}

int check_control() {
    unsigned char byte, control;
    int state = STATE_START;
    while(state != STATE_STOP && alarmEnabled == TRUE) {
        if(read(fd,byte,1) > 0) {
            switch(state) {
                case STATE_START:
                    if(byte == FLAG) state = STATE_RCV;
                    else state = STATE_START;
                    break;
                case STATE_RCV:
                    if(byte == ADDRESS_R) state=STATE_A_RCV;
                    else state = STATE_START;
                    break;
                case STATE_A_RCV:
                    if(byte == CONTROL_UA || byte= CONTROL_DISC || byte = CONTROL_RR(0) || byte = CONTROL_RR(1) || byte = CONTROL_REJ(0) || byte = CONTROL_REJ(1)) {
                        control = byte;
                        state = STATE_C_RCV;
                    }
                    else state = STATE_START;
                    break;
                case STATE_C_RCV:
                    if(byte == ADDRESS_R ^ control) state = STATE_BCC_OK;
                    else if (byte == FLAG) state = STATE_RCV;
                    else state = STATE_START;
                    break;
                case STATE_BCC_OK:
                    if(byte == FLAG) {
                        alarm(0);
                        state = STATE_STOP;
                    }
                    else state = STATE_START;
                    break;
                default:
                    break;
            }       
        }
    }
    return control;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char byte;
    int state = STATE_START;
    int i = 0; //variável para ir vendo os bytes do packet

    while(state != STATE_STOP) {
        if(read(fd,byte,1) > 0) {
            switch(state) {
                case STATE_START:
                    if(byte == FLAG) state = STATE_RCV;
                    break;
                case STATE_RCV:
                    if(byte == ADDRESS_S) state = STATE_A_RCV;
                    else if(byte != FLAG) state = STATE_START;
                    break;
                case STATE_A_RCV:
                
            }   
        }
    }
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
