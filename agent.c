#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//for network
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

void error_exit(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

//get task from c2 for agent
void get_task_from_c2() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[2048] = {0};

    //создание потокового сетевого сокета
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        error_exit("socket");
    }

    //настройка структуры адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0){
        error_exit("inet_pton");
    }

    //установка TCP-соединения с сервером
    if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        error_exit("connect");
        close(sockfd);
    }
    printf("[+] Успешное подключение к C2 %s:%d\n", SERVER_IP, SERVER_PORT);

    //формирование и отрпавка HTTP GET запроса
    const char* http_request = 
        "GET /get_task/test_agent_1 HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n\r\n";

    if(send(sockfd, http_request, strlen(http_request), 0) == -1) {
        error_exit("send");
        close(sockfd);
    }
    printf("[*] HTTP GET запрос отправлен.\n");

    //чтение ответа от сервера
    ssize_t bytes_read = read(sockfd, buffer, sizeof(buffer));
    if(bytes_read == -1) {
        error_exit("read");
        close(sockfd);
    }

    printf("[*] Ответ от сервера:\n%s\n", buffer);

    close(sockfd);
}   

int main() {
    printf("[*] Запуск агента...\n");
    get_task_from_c2();
    return 0;
}
