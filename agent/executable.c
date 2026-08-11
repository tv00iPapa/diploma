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
    char buf[4096] = {0};
    fgets(buf, sizeof(buf) - 1, fp);

    if(strlen(buf) >= max_result_len) {
        printf("[-] Error len command.\n");
        exit(EXIT_FAILURE);        
    }

    strncpy(result, buf, strlen(buf));
    pclose(fp);
}
