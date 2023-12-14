#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <regex.h>

#define MAX_LENGTH  500
#define FTP_PORT    21

/* Server Response Codes */
#define SERVER_READY_FOR_AUTH   220
#define SERVER_READY_FOR_PASS   331
#define LOGIN_SUCCESS           230
#define SERVER_PASSIVE_MODE     227
#define READY_FOR_DATA_TRANSFER 150
#define TRANSFER_COMPLETED      226
#define SERVER_GOODBYE          221

/* Regular Expressions for URL Parsing */
#define HOST      "%*[^/]//%[^/]"
#define HOST_AT   "%*[^/]//%*[^@]@%[^/]"
#define RESOURCE  "%*[^/]//%*[^/]/%s"
#define USER      "%*[^/]//%[^:/]"
#define PASSWORD      "%*[^/]//%*[^:]:%[^@\n$]"
#define RESPCODE  "%d"
#define PASSIVE   "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

/* Parser output */
struct URL {
    char host[MAX_LENGTH];      // 'ftp.up.pt'
    char resource[MAX_LENGTH];  // 'parrot/misc/canary/warrant-canary-0.txt'
    char file[MAX_LENGTH];      // 'warrant-canary-0.txt'
    char user[MAX_LENGTH];      // 'username'
    char password[MAX_LENGTH];  // 'password'
    char ip[MAX_LENGTH];        // 193.137.29.15
};

/* Response State Enumeration */
typedef enum {
    STATE_START,
    STATE_SINGLE_LINE,
    STATE_MULTI_LINE,
    STATE_END
} ResponseState;

int parseFTP(char *input, struct URL *url);
int create_socket(char *ip, int port);
int handleAuth(const int sockfd, const char *username, const char *password);
int enablePassiveMode(const int sockfd, char *ip, int *port);
int getResponse(const int sockfd, char *buffer);
int requestFile(const int sockfd, char *resource);
int downloadData(const int sockfdControl,const int sockfdData, char *filename);
int closeSockets(const int sockfdControl,const int sockfdData);
