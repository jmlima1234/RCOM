// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"



unsigned char getControl_packet(int control, const char *filename, long int fileSize, int *cpsize) {
    
    int fileSize_bytes = (int) ceil(log2f((float) fileSize) / 8.0);
    int fileName_bytes = strlen(filename);

    cpsize = 5 + fileSize_bytes + fileName_bytes;

    unsigned char *packet = malloc(cpsize);

    int packet_pos = 0;
    packet[packet_pos++] = control;
    packet[packet_pos++] = 0;
    packet[packet_pos++] = (unsigned char) fileSize_bytes;

    for(int i = 0; i<fileSize_bytes; i++) {
        packet[2 + fileSize_bytes-i] = (fileSize >> (8*i)) && 0xFF;
        packet_pos++;
    }
    packet[packet_pos++] = 1;
    packet[packet_pos] = fileName_bytes;

    memcpy(packet + packet_pos, filename, fileName_bytes);

    return packet;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout, const char *filename){
    LinkLayer Received;

    Received.serialPort = serialPort;
    Received.baudRate = baudRate;
    Received.nRetransmissions = nTries;
    Received.role = strcmp(role, "tx") ? LlRx : LlTx;
    Received.timeout = timeout;

    int fd = llopen(Received);
    if(fd < 0) exit(-1);
    
    switch(Received.role) {
        case LlTx:

        FILE *file = fopen(filename, 'rb');
        if (file == NULL) { 
            printf("File Not Found!\n"); 
            exit(-1); 
        } 
  
        fseek(file, 0L, SEEK_END); 
        long int fileSize = ftell(file);

        int cpsize;
        unsigned char *Control_packet_start = getControl_packet(2, filename, fileSize, &cpsize);
  
        if(llwrite(fd, Control_packet_start, cpsize) == -1) exit(-1);

        break;

        case LlRx:
        break;

        default:
        return -1;
        break;
    }
}
