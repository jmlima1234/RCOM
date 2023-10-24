// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"


unsigned char *parseControlPacket(unsigned char* packet, long int *fileSize)
{
    unsigned char size_bytes = packet[2];
    unsigned char name_bytes = packet[4 + size_bytes];

    for (int i = 0; i < (int) size_bytes; i++)
        *fileSize |= packet[3 + size_bytes - i] << (8 * i);

    unsigned char *name = (unsigned char *) malloc(name_bytes);

    memcpy(name, packet + 3 + size_bytes + 2, name_bytes);

    return name;
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
        printf("Going for llwrite! \n");

        if(llwrite(fd, Control_packet_start, cpsize) == -1) exit(-1);
        printPacket(Control_packet_start, cpsize);

        long int bytesLeft = fileSize;
        
        unsigned char *file_data = (unsigned char *) malloc(sizeof(unsigned char) * fileSize);
        fread(file_data, sizeof(unsigned char), fileSize, file);
        
        while(bytesLeft > 0) {
            printf("Bytes left");
            printf(" %ld",bytesLeft);            

            int dataSize = 0;
            if(bytesLeft > MAX_PAYLOAD_SIZE) dataSize = MAX_PAYLOAD_SIZE;
            else dataSize = bytesLeft;

            unsigned char* data =(unsigned char*) malloc(dataSize);
            memcpy(data,file_data,dataSize);

            int packetSize;
            unsigned char *packet = getData(data,dataSize,&packetSize);

            if(llwrite(fd,packet,packetSize) == -1) exit(-1);
            printf("Packet written!");
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
            long int fileSize;
            unsigned char *name;

            printf("Waiting for a control packet...\n");
            while ((packetSize = llread(fd, packet)) < 0);

            //printPacket(packet, packetSize);
            printf("Print done! \n");
            
            printf("Parsing control packet...\n");
            name = parseControlPacket(packet, &fileSize);
            printf("Parsed! \n");
            
            FILE* newFile = fopen((char *) name, "wb+");
            printf("Receiving data packets...\n");
            while (1) { 
            	if(packetSize == 0)  break; 
                while ((packetSize = llread(fd, packet)) < 0);
                printf("Received! \n");     
                if (packet[0] == 1) {
                    unsigned char *buffer = (unsigned char *) malloc(packetSize);
                                        printf("Created buffer\n");
                    memcpy(buffer, packet + 3, packetSize - 3);
                                        printf("memcpy \n");
                    fwrite(buffer, sizeof(unsigned char), packetSize - 3, newFile);
                                        printf("fwrite \n");
                    free(buffer);
                                        printf("fwrite \n");
                  
                } else if (packet[0] == 3) {
                    printPacket(packet, packetSize);
                    long int fileSize2;
                    unsigned char *name2 = parseControlPacket(packet, &fileSize2);
                    if ((fileSize == fileSize2) && (name == name2)) {
                        fclose(newFile);
                        printf("File transfer completed.\n");
                        break;
                    }
                } else {
                    printf("Invalid packet received. Terminating.\n");
                    break;
                }
            }
            llclose(fd);
            break;
        }
        default:
        exit(-1);
        break;
    }
}