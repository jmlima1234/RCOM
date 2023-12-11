#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#define HOST      "%*[^/]//%[^/]"
#define HOST_AT   "%*[^/]//%*[^@]@%[^/]"
#define RESOURCE  "%*[^/]//%*[^/]/%s"
#define USER      "%*[^/]//%[^:/]"
#define PASSWORD      "%*[^/]//%*[^:]:%[^@\n$]"
#define RESPCODE  "%d"
#define PASSIVE   "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"


struct hostent {
    char *h_name;    // Official name of the host.
    char **h_aliases;    // A NULL-terminated array of alternate names for the host.
    int h_addrtype;    // The type of address being returned; usually AF_INET.
    int h_length;    // The length of the address in bytes.
    char **h_addr_list;    // A zero-terminated array of network addresses for the host.
    // Host addresses are in Network Byte Order.
};