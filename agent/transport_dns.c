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
static struct in_addr server_addr;

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
int build_dns_query(const char* qname, unsigned char* buffer, size_t max_len_buffer, int16_t qtype) {
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

    return offset;
}

int dns_init(void) {
    current_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(current_sockfd == -1) {
        printf("[-/DNS] Error create socket.\n");
    }
    
    //timeout
    struct timeval tv = {
        .tv_sec = 2,
        .tv_usec = 0
    };
    setsockopt(current_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    if(inet_pton(AF_INET, SERVER_IP/*from config.h*/, &server_addr) <= 0) {
        printf("[-/ICMP] Error server_addr.\n");
        close(current_sockfd);
        return 1;
    }

    return 0;
}

static int dns_get_command(/*[OUT]*/char* command, /*[OUT]*/char* task_id, size_t max_len_command, size_t max_len_task_id, char* agent_id) {
    return 0;
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
