#include "transport.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>

#define SERVER_IP "127.0.0.1"

static int current_sockfd = -1; //RAW_SOCKET

/**
 * @brief Функция для расчета контрольной суммы icmp пакета
 *
 * @param data Указатель на данные (сам пакет)
 * @param len Длина данных (пакета)
 * @return Значение контрольной суммы
*/
int checksum(uint8_t* data, int len) {
    uint32_t summ = 0;
    uint16_t* p = (uint16_t*)data;

    while(len > 1) {
        summ += *p++;
        len -= 2;
    }

    if(len == 1) {
        uint16_t tmp = 0;
        *(uint8_t*)&tmp = *(uint8_t*)p;
        summ += tmp;
    }

    while(summ >> 16) {
        summ = (summ & 0xffff) + (summ >> 16);
    }

    return (uint16_t)(~summ);
}

/**
 * @brief Функция для инициализации сокета
 *
 * @param void Отсутствие аргументов
 *
 * @return В случае положительного исхода - 0 
*/
static int icmp_init(void) {
    current_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(current_sockfd < 0) {
        printf("[-/ICMP] Error create socket.");
        return 1;
    }

    return 0;
}

/**
 * @brief Функция позволяет получить команду агенту от сервера
 *
 * @param command Строка содержащая полученную команду для агента,
 * выходной аргумент
 * @param task_id Номер задачи, выходной аргумент
 * @param max_len_command Размер буфера под команду
 * @param max_len_task_id Размер буфера под номер задачи
 * @param agent_id id агента
 *
 * @return В случае положительного исхода - 0
*/
static int icmp_get_command(/*[OUT]*/ char* command,/*[OUT]*/ char* task_id, size_t max_len_command, size_t max_len_task_id, char* agent_id) {
    return 0;
}

int main() {
    //сырой сокет домена AF_INET протокола IPPROTO_ICMP
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sock < 0) {
        printf("[-/ICMP] Error create socket.");
        return 1;
    }

//==========ICMP packet======================================
    char packet[64]; //буфер для создания icmp пакета
    memset(packet, 0, sizeof(packet));

    //наложение структуры заголовка icmp пакета на буфер
    struct icmphdr* icmp = (struct icmphdr*)packet;

    //заполнение полей
    icmp->type = ICMP_ECHO; //type 8
    icmp->code = 0; //0 - без особого значения
    icmp->un.echo.id = htons(0x666); //идентификатор
    icmp->un.echo.sequence = htons(1); //серийный номер
                                                  
    //данные
    char* payload = packet + sizeof(struct icmphdr);
    strcpy(payload, "HELLO");

    int packet_len = sizeof(struct icmphdr) + strlen("HELLO");
    icmp->checksum = checksum(packet, packet_len);

//=====отправка ICMP пакета=================================
    struct sockaddr_in dest = {
        .sin_family = AF_INET,
    };
    inet_pton(AF_INET, SERVER_IP, &dest.sin_addr);    

    //отправка этого пакета
    if(sendto(sock, packet, packet_len, 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        printf("[-/ICMP] Error send icmp-packet.");
    } else {
        printf("[+/ICMP] Send icmp-packet to agent success.");
    }

    close(sock);
    return 0;
}
