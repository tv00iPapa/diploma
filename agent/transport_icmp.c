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
        printf("[-/ICMP] Error create socket.\n");
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
    char packet[64]; //buf for icmp-packet
    memset(packet, 0, sizeof(packet));

    //наложение структуры заголовка icmp пакета на буфер
    struct icmphdr* icmp_header = (struct icmphdr*)packet;

    //заполнение полей
    icmp_header->type = ICMP_ECHO; //type 8
    icmp_header->code = 0; //0 - без особого значения
    icmp_header->un.echo.id = htons(0x666); //идентификатор
    icmp_header->un.echo.sequence = htons(1); //серийный номер

    //данные
    char* data_dest = packet + sizeof(struct icmphdr);
    snprintf(data_dest, 30, "GET_TASK|%s|", agent_id);
    int packet_len = sizeof(struct icmphdr) + strlen(data_dest);

    //контрольная сумма
    icmp_header->checksum = checksum((uint8_t*)packet, packet_len);

    //отправка ICMP пакета
    struct sockaddr_in dest = {
        .sin_family = AF_INET, 
    };
    inet_pton(AF_INET, SERVER_IP, &dest.sin_addr);

    //отправка этого пакета
    if(sendto(current_sockfd, packet, packet_len, 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        printf("[-/ICMP] Error send icmp-packet.\n");
        return 0;
    } else {
        printf("[+/ICMP] Send icmp-packet for get task.\n");
    }

    //получение ответа от сервера
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    memset(packet, 0, sizeof(packet));

    int recv_len = recvfrom(current_sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&from, &from_len);
    
    if(recv_len <= 0) {
        printf("No tasks.\n");
        return 1;
    } else {
        //анализ
        //данные приходят в сырой сокет с ip заголовком, который тоже надо обработать
        struct iphdr* ip = (struct iphdr*)packet;

        int ip_hdr_len = ip->ihl * 4; //длина ip заголовка (ед. изм. = 4 байта)
        
        //icmp заголовок
        struct icmphdr* icmp_hdr = (struct icmphdr*)(packet + ip_hdr_len);

        sleep(3);
        if(icmp_hdr->type == ICMP_ECHOREPLY) {
            printf("[+/ICMP] Recevied task from server (%s).\n", inet_ntoa(from.sin_addr));
            
            int data_from_len = recv_len - ip_hdr_len - sizeof(struct icmphdr);
            if(data_from_len > 0) {
                char* data_from = packet + ip_hdr_len + sizeof(struct icmphdr);

                //парсинг полученных данных
                if(data_from[0] == 'T' && data_from[1] == 'A' && data_from[2] == 'S' &&
                        data_from[3] == 'K' && data_from[4] == '|') {
                    data_from += 5;
                    char* pos = strchr(data_from, '|');
                    if(pos != NULL) {
                        //длина до разделителя
                        size_t len = data_from - pos;
                        if(len > max_len_task_id) {
                            printf("[-/ICMP] Error. Len task_id invalid.\n");
                            return 1;
                        }
                        char task_id_tmp[50] = {0};
                        memcpy(task_id_tmp, data_from, len);
                        data_from += (len + 1);

                        //само задание
                        pos = strchr(data_from, '|');
                        if(pos != NULL) {
                            len = data_from - pos;
                            if(len > max_len_command) {
                                printf("[-/ICMP] Error. Len task_command invalid.\n");
                                return 1;
                            }
                            char task[100] = {0};

                            printf("[+/ICMP] Получена задача (%s): %s\n", task_id_tmp, task);

                            strcpy(task_id, task_id_tmp);
                            strcpy(command, task);                                
                        } else {
                            printf("[-/ICMP] Invalid task.\n");
                        }
                    } else {
                        printf("[-/ICMP] Invalid task_id.\n");
                    }
                } else {
                    printf("[-/ICMP] Invalid data(header).\n");
                    return 1;
                }
            } else {
                printf("[-/ICMP] The data is missing.\n");
                return 1;
            }
        }
    }

    return 0;
}

/**
 * @return В случае положительного исхода - 0
*/
static int icmp_send_result(char* task_id, char* result) {
    return 0;
}

void icmp_cleanup(void) {
    close(current_sockfd);
}

//export module
TransportModule icmp_transport = {
    .name = "ICMP",
    .init = icmp_init,
    .get_command = icmp_get_command,
    .send_result = icmp_send_result,
    .cleanup = icmp_cleanup
};
