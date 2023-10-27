// Link layer protocol implementation

#include "../include/link_layer.h"

int alarmCount = 0;
int alarmEnabled = FALSE;
int baudRate = 0;
int nRetransmissions = 0;
int timeout = 0;
unsigned char tramaTx = 0;
unsigned char tramaRx = 0;
LinkLayer connection;


struct termios oldtio;
struct termios newtio;

void printFrame(const unsigned char *packet, int packetSize) {
    printf("Received packet with size %d:\n", packetSize);
    printf("Contents: { ");
    for (int i = 0; i < packetSize; i++) {
        printf("%02X ", packet[i]);
    }
    printf("}\n\n");
}

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount--;

    printf("--- Alarm #%d ---\n", alarmCount);
}

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int sendFrame(int fd, unsigned char Address, unsigned char control) {
    unsigned char frame[5] = {FLAG, Address, control, Address ^ control, FLAG};
    return write(fd,&frame,5);

}



unsigned char check_control(int fd, unsigned char Address) {

    unsigned char byte, control = 0xFF;
    int state = STATE_START;
    unsigned char previous_alarm_status = alarmEnabled;
    alarmEnabled = TRUE;

    while(state != STATE_STOP && alarmEnabled == TRUE) {

        if(read(fd,&byte,1) > 0) {
            switch(state) {
                case STATE_START:
                    if(byte == FLAG) state = STATE_RCV;
                    else state = STATE_START;
                    break;
                case STATE_RCV:
                    if(byte == Address) state=STATE_A_RCV;
                    else state = STATE_START;
                    break;
                case STATE_A_RCV:
                    if(byte == CONTROL_SET || byte == CONTROL_UA || byte == CONTROL_DISC || byte == CONTROL_RR(0) || byte == CONTROL_RR(1) || byte == CONTROL_REJ(0) || byte == CONTROL_REJ(1)) {

                        control = byte;
                        state = STATE_C_RCV;

                    }
                    else if (byte == FLAG) state = STATE_RCV;
                    else state = STATE_START;
                    break;
                case STATE_C_RCV:
                    if(byte == (Address ^ control)) state = STATE_BCC_OK;
                    else if (byte == FLAG) state = STATE_RCV;
                    else state = STATE_START;
                    break;
                case STATE_BCC_OK:
                    if(byte == FLAG) {
                        if (previous_alarm_status == TRUE) alarm(0);
                        alarmEnabled = FALSE;
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

int openSerialPort(const char *serialPort, int baudRate)
{
    int fd = open(serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0) return -1;


    if (tcgetattr(fd, &oldtio) == -1) return -1;


    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 5; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Read without blocking
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) return -1;


    return fd;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    connection = connectionParameters;
    connection.role = connectionParameters.role;
    baudRate = connectionParameters.baudRate;
    timeout = connectionParameters.timeout;
    nRetransmissions = connectionParameters.nRetransmissions;
    alarmCount = nRetransmissions;


    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0) return -1;
    


    int state = STATE_START;
    unsigned char byte;   
    switch(connectionParameters.role) {
        case LlTx:
        (void)signal(SIGALRM, alarmHandler);

        while (1){

            if(alarmCount == 0) return -1;

            if (alarmEnabled == FALSE){
                sendFrame(fd,ADDRESS_S,CONTROL_SET);
                
                alarm(timeout);
                alarmEnabled = TRUE;
            }

            if (check_control(fd,ADDRESS_R) == CONTROL_UA) {
                break;
            }

            }
            break;
        
        case LlRx:

 	    while (state != STATE_STOP){

            if(read(fd, &byte, 1) > 0) {
                switch(state) {
                case STATE_START:
                
                if (byte == FLAG) state = STATE_RCV;
                
                break;
                case STATE_RCV:
                
                if (byte == ADDRESS_S) state = STATE_A_RCV;
                else if (byte == FLAG) state = STATE_RCV;
                else state = STATE_START;

                break;
                case STATE_A_RCV:
                
                if (byte == CONTROL_SET) state = STATE_C_RCV;
                else if (byte == FLAG) state = STATE_RCV;
                else state = STATE_START;

                break;
                case STATE_C_RCV:

                if (byte == (ADDRESS_S ^ CONTROL_SET)) state = STATE_BCC_OK;
                else if (byte == FLAG) state = STATE_RCV;
                else state = STATE_START;

                break;
                case STATE_BCC_OK:
                if (byte == FLAG)
                {
                    state = STATE_STOP;
                    sendFrame(fd,ADDRESS_R,CONTROL_UA);
                }
                else state = STATE_START;
                break;

                default:
                return -1;
    	        }
    	    }
    	}
    }
    return fd;
}
////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, const unsigned char *buf, int bufSize){
    //printf("Packet to send:\n\n");
    //printf("{ ");
    //for (int i = 0; i < bufSize; i++)
        //printf("%02X ", buf[i]);
    //printf("}\n\n");
    
    int frameSize = bufSize + 6; //{FLAG, A, C, BCC1} + Data size + {BCC2, FLAG}
    unsigned char *frame = malloc(frameSize);

    frame[0] = FLAG;
    frame[1] = ADDRESS_S;
    frame[2] = CONTROL_INF(tramaTx);
    frame[3] = frame[1] ^ frame[2];

    memcpy(frame + 4, buf, bufSize); //passar a data para o frame
            
    unsigned char BCC2 = buf[0];
    for(int i = 1; i < bufSize; i++) BCC2 ^= buf[i]; // calcular o bcc2

    for(int i = 4; i < frameSize - 2; i++) { //byte stuffing
        if(frame[i] == FLAG) {
            unsigned char byte1 = 0x7D;
            unsigned char byte2 = 0x5E;

	    frame = realloc(frame,++frameSize);
            memmove(frame + i + 1, frame + i, frameSize - i - 1);

            frame[i++] = byte1;
            frame[i] = byte2;
        }
        else if (frame[i] == ESC) {
            unsigned char byte1 = 0x7D;
            unsigned char byte2 = 0x5D;

	    frame = realloc(frame,++frameSize);
            memmove(frame + i + 1, frame + i, frameSize - i - 1);

            frame[i++] = byte1;
            frame[i] = byte2;
        }
    }

    frame[frameSize-2] = BCC2;
    frame[frameSize-1] = FLAG;
    
    //printf("Frame after-stuffing:\n\n");
    //printf("{ ");
    //for (int i = 0; i < frameSize; i++)
        //printf("%02X ", frame[i]);
    //printf("} %d\n\n", frameSize);
    
    int state = STATE_START;
    alarmCount = nRetransmissions;
    unsigned char Control;
    
    //printf("Alarm count - %d  \n ", alarmCount);

    (void)signal(SIGALRM, alarmHandler);
    
    while (state != STATE_STOP && alarmCount != 0){
        if(alarmEnabled == FALSE) {

            write(fd,frame,frameSize);
            alarm(timeout);
            alarmEnabled = TRUE;
        }    

        //check control
        Control = check_control(fd,ADDRESS_R);

        if (Control == CONTROL_RR(0) || Control == CONTROL_RR(1))
        {
            state = STATE_STOP;
            tramaTx = (tramaTx+1) % 2;
        }

        else if(Control == CONTROL_REJ(0) || Control == CONTROL_REJ(1)) {
            alarm(0);
            alarmEnabled = FALSE;
        }
    }
    
    if (state != STATE_STOP) return -1;
    
    free(frame);
    return frameSize;
}


////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(int fd, unsigned char *packet) {

    int state = STATE_START;
    unsigned char byte;
    int packetIndex = 0;
    unsigned char control = 0; // Variável para armazenar o byte de controle
    unsigned char bcc2 = 0;    // Variável para calcular BCC2
    unsigned char acc = 0;     // Variável para verificar BCC2

    while (state != STATE_STOP) {
        if (read(fd, &byte, 1) > 0) {
            switch (state) {
                case STATE_START:
                    if (byte == FLAG) {
                        state = STATE_RCV;
                    }

                    break;
                case STATE_RCV:
                    if (byte == ADDRESS_S) {
                        state = STATE_A_RCV;
                    } else if (byte != FLAG) {
                        state = STATE_START;
                    }

                    break;
                case STATE_A_RCV:

                    if (byte == CONTROL_INF(0) || byte == CONTROL_INF(1)) {
                        control = byte; // Armazena o byte de controle
                        state = STATE_C_RCV;
                    } 
                    else if (byte == CONTROL_DISC) {
                        // Enviar mensagem de desconexão
                        // Após o envio, retorne 0 para indicar desconexão
                        sendFrame(fd, ADDRESS_R, CONTROL_DISC);
                        return 0;
                    } 
                    else if (byte == FLAG) {
                        state = STATE_RCV;
                    }
                    else state = STATE_START;

                    break;
                
                case STATE_C_RCV:

                    if (byte == (ADDRESS_S ^ control)) {
                        state = STATE_DATA;
                    } else if (byte == FLAG) {
                        state = STATE_RCV;
                    }
                    else{
                        state = STATE_START;
                    }
                    break;
                case STATE_DATA:

                    if (byte == ESC) state = STATE_ESC_FOUND;
                    else if (byte == FLAG) {
                        bcc2 = packet[packetIndex - 1];
                        packetIndex--;
                        acc = packet[0];

                        for (unsigned int i = 1; i < packetIndex; i++)
                        {
                            acc ^= packet[i];
                        }
                        
                        // Desempacotamento bem-sucedido
                        if (bcc2 == acc) {

                            state = STATE_STOP;
                            sendFrame(fd, ADDRESS_R, CONTROL_RR(tramaRx));
                            tramaRx = (tramaRx +1) %2;
                            
                            return packetIndex;
                        } else {
                            // Erro de verificação do BCC2, envie REJ
                            sendFrame(fd,ADDRESS_R,CONTROL_REJ(tramaRx));
                            return -1; // Retorna erro
                        }
                    } 
                    else {
                        // Adicione o byte ao pacote desempacotado
                        packet[packetIndex++] = byte;
                    }
                    break;
                case STATE_ESC_FOUND:
                    printf("ESC found \n");     
                    state = STATE_DATA;
                    if (byte == AFTER_ESC) {
                        packet[packetIndex++] = ESC;
                    }
                    else if(byte == AFTER_FLAG) {
                        packet[packetIndex++] = FLAG;
                    }
                    else {
                        packet[packetIndex++] = ESC;
                        packet[packetIndex++] = byte;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd)
{
    switch(connection.role) {
        case LlTx:
            alarmCount = nRetransmissions;
            
            (void)signal(SIGALRM, alarmHandler);
            while(1) {
                
                if(alarmCount == 0) return -1;

                if(alarmEnabled == FALSE){
                    sendFrame(fd, ADDRESS_S,CONTROL_DISC);
                    alarm(timeout);
                    alarmEnabled = TRUE;
                }

                if(check_control(fd,ADDRESS_R) == CONTROL_DISC) {
                    break;
                }
            }
            sendFrame(fd,ADDRESS_S, CONTROL_UA);
            break;
        
        case LlRx:
            alarmCount = nRetransmissions;
            
            while(check_control(fd,ADDRESS_S) != CONTROL_DISC);

            (void)signal(SIGALRM, alarmHandler);
            while(1) {
                
                if(alarmCount == 0) return -1;

                if(alarmEnabled == FALSE){
                    sendFrame(fd, ADDRESS_R,CONTROL_DISC);
                    alarm(timeout);
                    alarmEnabled = TRUE;
                }

                if(check_control(fd,ADDRESS_S) == CONTROL_UA) {
                    break;
                }
            }
            break; 
        default:
            return -1;
            break;       
    }
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) return -1;

    return close(fd);
}

