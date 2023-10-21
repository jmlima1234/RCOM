// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"


unsigned char *getData(unsigned char *data, int dataSize, int *packetSize) {

    packetSize = 3 + dataSize;

    unsigned char *start_packet = (unsigned char *) malloc(packetSize);

    start_packet[0] = 1;
    start_packet[1] = (unsigned char) dataSize / 256;
    start_packet[2] = (unsigned char) dataSize % 256;

    memcpy(start_packet+3,data,dataSize);
    return start_packet;
}

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
  
        int previous = ftell(file);
        fseek(file,0L,SEEK_END);
        long int fileSize = ftell(file)-previous;
        fseek(file,previous,SEEK_SET);

        unsigned int cpsize;
        unsigned char *Control_packet_start = getControl_packet(2, filename, fileSize, &cpsize);
  
        if(llwrite(fd, Control_packet_start, cpsize) == -1) exit(-1);

        long int bytesLeft = fileSize;
        
        unsigned char *file_data = (unsigned char *) malloc(sizeof(unsigned char) * fileSize);
        fread(data, sizeof(unsigned char), fileSize, file);
        
        while(bytesLeft > 0) {
            int dataSize = 0;
            if(bytesLeft > MAX_PAYLOAD_SIZE) dataSize = MAX_PAYLOAD_SIZE;
            else dataSize = bytesLeft;

            unsigned char* data =(unsigned char*) malloc(dataSize);
            memcpy(data,fileDate,dataSize);

            int packetSize;
            unsigned char *packet = getData(data,dataSize,&packetSize);

            if(llwrite(fd,packet,packetSize) == -1) exit(-1);

            file_data += dataSize;
            bytesLeft -= dataSize;
        }

        unsigned char *packet_end = getControl_packet(3, filename, fileSize, &cpsize);
        if(llwrite(fd, packet_end, cpsize) == -1) exit(-1);

        llclose(fd);

        break;

        case LlRx:

        llclose(fd);
        break;

        default:
        exit(-1);
        break;
    }
}
