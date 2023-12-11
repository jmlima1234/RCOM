// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"

const char *Appendix = "-received";

char *getNewFilename(const char *filename, const char *appendix, int fileNameSize)
{
    const char *dotPosition = strrchr(filename, '.');

    char *newFilename = (char *) malloc(fileNameSize);

    strncpy(newFilename, filename, (int)(dotPosition - filename));

    newFilename[(int)(dotPosition - filename)] = '\0';

    strcat(newFilename, appendix);

    strcat(newFilename, dotPosition);

    return newFilename;
}


char *parseControlPacket(unsigned char* packet, long int *fileSize, const char *appendix)
{
    unsigned char size_bytes = packet[2];
    unsigned char name_bytes = packet[4 + size_bytes];

    for (int i = size_bytes - 1; i >= 0; i--)
        *fileSize = (*fileSize << 8) | packet[3 + i];

    unsigned char *name = (unsigned char *) malloc(name_bytes * sizeof(unsigned char));

    memcpy(name, packet + 3 + size_bytes + 2, name_bytes);
    
    name[name_bytes] = '\0';

    char *newFileName = getNewFilename((const char *) name, appendix, name_bytes + strlen(appendix) + 1);

    return newFileName;
}

void printPacket(const unsigned char *packet, int packetSize) {
    printf("Received packet with size %d:\n", packetSize);
    printf("Contents: { ");
    for (int i = 0; i < packetSize; i++) {
        printf("%02X ", packet[i]);
    }
    printf("}\n\n");
}

unsigned char *getData(unsigned char *data, int dataSize, int *packetSize) {

    *packetSize = 3 + dataSize;

    unsigned char *start_packet = (unsigned char *) malloc(*packetSize);

    start_packet[0] = 1;
    start_packet[1] = (unsigned char) dataSize / 256;
    start_packet[2] = (unsigned char) dataSize % 256;

    memcpy(start_packet+3,data,dataSize);
    return start_packet;
}

unsigned char *getControl_packet(int control, const char *filename, long int fileSize, int *cpsize) {
    
    int fileSize_bytes = (int) ceil(log2f((float)fileSize)/8.0);
    int fileName_bytes = strlen(filename);

    *cpsize = 5 + fileSize_bytes + fileName_bytes;

    unsigned char *packet = malloc(*cpsize);

    int packet_pos = 0;
    packet[packet_pos++] = control;
    packet[packet_pos++] = 0;
    packet[packet_pos++] = (unsigned char) fileSize_bytes;

    for(int i = 0; i<fileSize_bytes; i++) {
        packet[3 + i] = (fileSize >> (8*i)) && 0xFF;
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
        {
        FILE *file = fopen(filename, "rb");
        if (file == NULL) { 
            printf("File Not Found!\n"); 
            exit(-1); 
        } 
  
        int previous = ftell(file);
        fseek(file,0L,SEEK_END);
        long int fileSize = ftell(file)-previous;
        fseek(file,previous,SEEK_SET);
	
	
        int cpsize;
        unsigned char *Control_packet_start = getControl_packet(2, filename, fileSize, &cpsize);


        if(llwrite(fd, Control_packet_start, cpsize) == -1) exit(-1);
        //printPacket(Control_packet_start, cpsize);

        long int bytesLeft = fileSize;
        
        unsigned char *file_data = (unsigned char *) malloc(sizeof(unsigned char) * fileSize);

        fread(file_data, sizeof(unsigned char), fileSize, file);
        
        while(bytesLeft > 0) {       
            int dataSize = 0;
            if(bytesLeft > MAX_PAYLOAD_SIZE) dataSize = MAX_PAYLOAD_SIZE;
            else dataSize = bytesLeft;

            unsigned char* data =(unsigned char*) malloc(dataSize);
            memcpy(data,file_data,dataSize);

            int packetSize;
            unsigned char *packet = getData(data,dataSize,&packetSize);

            if(llwrite(fd,packet,packetSize) == -1) exit(-1);

            file_data += dataSize;
            bytesLeft -= MAX_PAYLOAD_SIZE;
        }

        unsigned char *packet_end = getControl_packet(3, filename, fileSize, &cpsize);
        
        if(llwrite(fd, packet_end, cpsize) == -1) exit(-1);

        llclose(fd);

        break;
        }
        case LlRx:      
        {  
            unsigned char *packet = (unsigned char *) malloc(MAX_PAYLOAD_SIZE);
            int packetSize = 0;
            long int fileSize = 0;

            while ((packetSize = llread(fd, packet)) < 0);

            //printPacket(packet, packetSize);
            
            char *name = parseControlPacket(packet, &fileSize, Appendix);
            
            FILE* newFile = fopen((char *) filename, "wb+");

            while (1) { 
            	if(packetSize == 0)  break; 
                
                while ((packetSize = llread(fd, packet)) < 0);
                
                if (packet[0] == 1) {
                    unsigned char *buffer = (unsigned char *) malloc(packetSize - 3);

                    memcpy(buffer, packet + 3, packetSize - 3);

                    fwrite(buffer, sizeof(unsigned char), packetSize - 3, newFile);

                    free(buffer);

                  
                } else if (packet[0] == 3) {
                    //printPacket(packet, packetSize);
                    long int fileSize2;
                    name = parseControlPacket(packet, &fileSize2, Appendix);
                    if (fileSize == fileSize2) {
                        fclose(newFile);
                        printf("File transfer completed.\n");
                        break;
                    }
                } else {
                    printf("Invalid packet received. Terminating.\n");
                    break;
                }
            }
            printf("Going for llclose \n");
            llclose(fd);
            break;
        }
        default:
        exit(-1);
        break;
    }
}

