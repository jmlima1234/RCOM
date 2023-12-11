#include "../include/application.h"

int parseFTP(char *input, struct URL *url) {

    regex_t compare;
    regcomp(&compare, "/", 0);
    if(regexec(&compare, input, 0, NULL, 0)) return -1;

    regcomp(&compare, "@", 0);
    if(regexec(&compare, input, 0, NULL, 0)){
        sscanf(input, HOST_AT, url->host);
        sscanf(input, USER, url->host);
        sscanf(input, PASSWORD, url->host);
    }
    else {
        sscanf(input, HOST, url->host);
        strcpy(url->user, "anonymous");
        strcpy(url->password, "password");
    }

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
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    } 
    

}