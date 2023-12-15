#include "../include/application.h"

int parseFTP(char *input, struct URL *url) {

    regex_t compare;
    regcomp(&compare, "/", 0);
    if(regexec(&compare, input, 0, NULL, 0)) return -1;

    regcomp(&compare, "@", 0);
    if(regexec(&compare, input, 0, NULL, 0) != 0){
        sscanf(input, HOST, url->host);
        strcpy(url->user, "anonymous");
        strcpy(url->password, "password");
    }
    else {
        sscanf(input, HOST_AT, url->host);
        sscanf(input, USER, url->user);
        sscanf(input, PASSWORD, url->password);
    }

    sscanf(input, RESOURCE, url->resource);
    strcpy(url->file, strrchr(input, '/') + 1);

    //esta parte foi tirada do ficheiro getip.c
    struct hostent *h;
    if (strlen(url->host) == 0) return -1;
    if ((h = gethostbyname(url->host)) == NULL) {
        printf("Invalid hostname '%s'\n", url->host);
        exit(-1);
    }
    strcpy(url->ip, inet_ntoa(*((struct in_addr *) h->h_addr)));

    return !(strlen(url->host) && strlen(url->user) && 
           strlen(url->password) && strlen(url->resource) && strlen(url->file));
}

int create_socket(char *ip, int port) {

    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    return sockfd;
}


int handleAuth(const int sockfd, const char *username, const char *password) {
    char cmdUser[5 + strlen(username) + 1];
    char cmdPass[5 + strlen(password) + 1];
    char response[MAX_LENGTH];

    sprintf(cmdUser, "USER %s\n", username);
    sprintf(cmdPass, "PASS %s\n", password);

    write(sockfd, cmdUser, strlen(cmdUser));
    if (getResponse(sockfd, response) != SERVER_READY_FOR_PASS) {
        printf("Unknown user '%s'. Abort.\n", username);
        exit(-1);
    }

    write(sockfd, cmdPass, strlen(cmdPass));
    return getResponse(sockfd, response);
}

int enablePassiveMode(const int sockfd, char *ip, int *port) {
    char response[MAX_LENGTH];
    int p1, p2, p3, p4, portHigh, portLow;

    write(sockfd, "PASV\n", 5);
    if (getResponse(sockfd, response) != SERVER_PASSIVE_MODE) return -1;

    sscanf(response, PASSIVE, &p1, &p2, &p3, &p4, &portHigh, &portLow);
    *port = portHigh * 256 + portLow;
    sprintf(ip, "%d.%d.%d.%d", p1, p2, p3, p4);

    return SERVER_PASSIVE_MODE;
}

int getResponse(const int sockfd, char *buffer) {
    char byte;
    int idx = 0, code;
    ResponseState currentState = STATE_START;

    memset(buffer, 0, MAX_LENGTH);
    while (currentState != STATE_END) {
        read(sockfd, &byte, 1);
        switch (currentState) {
            case STATE_START:
                if (byte == ' ') currentState = STATE_SINGLE_LINE;
                else if (byte == '-') currentState = STATE_MULTI_LINE;
                else if (byte == '\n') currentState = STATE_END;
                else buffer[idx++] = byte;
                break;
            case STATE_SINGLE_LINE:
                if (byte == '\n') currentState = STATE_END;
                else buffer[idx++] = byte;
                break;
            case STATE_MULTI_LINE:
                if (byte == '\n') {
                    memset(buffer, 0, MAX_LENGTH);
                    currentState = STATE_START;
                    idx = 0;
                } else buffer[idx++] = byte;
                break;
            case STATE_END:
                break;
            default:
                break;
        }
    }

    sscanf(buffer, RESPCODE, &code);
    return code;
}

int requestFile(const int sockfd, char *resource) {
    char requestCmd[5 + strlen(resource) + 1], response[MAX_LENGTH];
    sprintf(requestCmd, "RETR %s\n", resource);
    
    write(sockfd, requestCmd, strlen(requestCmd));
    return getResponse(sockfd, response);
}

int downloadData(const int sockfdControl, const int sockfdData, char *filename) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error opening file '%s'\n", filename);
        exit(-1);
    }

    char buffer[MAX_LENGTH];
    int bytesRead;
    while ((bytesRead = read(sockfdData, buffer, MAX_LENGTH)) > 0) {
        if (fwrite(buffer, 1, bytesRead, file) != bytesRead) {
            printf("Error writing to file '%s'.\n", filename);
            fclose(file);
            exit(-1);
        }
    }
    fclose(file);
    
    return getResponse(sockfdControl, buffer);
}


int closeSockets(const int sockfdControl, int sockfdData) {
    char response[MAX_LENGTH];
    write(sockfdControl, "QUIT\n", 5);
    if (getResponse(sockfdControl, response) != SERVER_GOODBYE) return -1;
    return close(sockfdControl) || close(sockfdData);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    } 
    
    struct URL application;
    memset(&application, 0, sizeof(application));

    if (parseFTP(argv[1], &application) != 0) {
        printf("Parse error. Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }
    printf("Hostname: %s\nPath: %s\nFilename: %s\nUsername: %s\nPassword: %s\nIP Address: %s\n", application.host, application.resource,application.file,application.user,application.password,application.ip);


    int sockfdControl = create_socket(application.ip, FTP_PORT);
    char response[MAX_LENGTH];
    if (sockfdControl < 0 || getResponse(sockfdControl, response) != SERVER_READY_FOR_AUTH) {
        printf("Socket to '%s' and port %d failed\n", application.ip, FTP_PORT);
        exit(-1);
    }
    if (handleAuth(sockfdControl, application.user, application.password) != LOGIN_SUCCESS) {
        printf("Authentication failed for user '%s' with password '%s'.\n", application.user,application.password);
        exit(-1);
    }

    char dataIP[MAX_LENGTH];
    int dataPort;
    if(enablePassiveMode(sockfdControl, dataIP, &dataPort)!= SERVER_PASSIVE_MODE){
        printf("Passive mode failed\n");
        exit(-1);
    }

    int sockfdData = create_socket(dataIP, dataPort);
    if(sockfdData < 0) {
        printf("Socket to '%s:%d' failed\n", dataIP, dataPort);
        exit(-1);
    }

    if (requestFile(sockfdControl, application.resource) != READY_FOR_DATA_TRANSFER) {
        printf("Resource '%s' unavailable.\n", application.resource);
        exit(-1);
    }

    if (downloadData(sockfdControl, sockfdData, application.file) != TRANSFER_COMPLETED) {
        printf("Failed to download file '%s'.\n", application.file);
        exit(-1);
    }

    if(closeSockets(sockfdControl, sockfdData)!= 0){
        printf("Sockets close error.\n");
        exit(-1);
    }

    return 0;

}