#include "executable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void execute_command(const char* command, char* result, int max_result_len) {
    FILE* fp;

    fp = popen(command, "r");

    if(fp == NULL) {
        printf("[-] Error popen.\n");
        exit(EXIT_FAILURE);
    }
    char temp[128] = {0};
    char buf[4096] = {0};
    int total_len_buf = 0;

    while(fgets(temp, sizeof(temp) - 1, fp) != NULL) {
        total_len_buf += strlen(temp);
        if(total_len_buf < 4096) {
            strncat(buf, temp, strlen(temp));
        }
    }

    if(strlen(buf) >= max_result_len) {
        printf("[-] Error len command.\n");
        exit(EXIT_FAILURE);        
    }

    strncpy(result, buf, strlen(buf));
    
    pclose(fp);
}
