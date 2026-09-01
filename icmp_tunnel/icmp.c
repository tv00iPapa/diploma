#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>


/**
 * @brief Функция для расчета контрольной суммы icmp пакета.
 *
 * @param data Указатель на данные, которые передаются в функцию.
 * @param len Длина данных, переданных в функцию.
 *
 * @return Значение контрольной суммы пакета.
 *
 */
int checksum(uint8_t* data, int len) {
	uint32_t sum = 0;
	uint16_t* p = (uint16_t*)data;

	while(len > 1) {
		sum += *p++;
		len -= 2;
	}

	if(len == 1) {
		uint16_t tmp = 0;
		*(uint8_t*)&tmp = *(uint8_t*)p;
		sum += tmp;
	}

	while(sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return (uint16_t)(~sum);
}

int main(int argc, char* argv[0]) {
	if(argc < 2) {
		printf("Usage: %s <target_ip>\n", argv[0]);
		return 1;
	}

	//сырой сокет домена ipv4 для протокола icmp
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(sock < 0) {
		perror("socket");
		return 1;
	}

//=====================ICMP-packet===============================
	char packet[64]; //буфер пакетов
	memset(packet, 0, sizeof(packet)); //инициализация
	
	//наложение структуры icmp на область памяти packet
	struct icmphdr *icmp = (struct icmphdr*)packet;
	
	//заполнение полей заголовка icmp
	icmp->type = ICMP_ECHO; //type 8(request)
	icmp->code = 0; //0 - без ососбого значения
	icmp->un.echo.id = htons(0x66); //идентификатор (можно писать все что угодно)
	icmp->un.echo.sequence = htons(1); //серийный номер (нужен для отправка нескольких запросов)
	
	//добавление данных(полезной нагрузки) после заголовка
    	//то есть берем адрес начала формирующегося пакета (адрес находится в packet)
    	//и далее отступаем на размер структуры , таким образом получаем адрес,
    	//который находится сразу после заголовка icmp
    	char *payload = packet + sizeof(struct icmphdr);
    	strcpy(payload, "HELLO");

    	//вычисление длины всего пакета и генерация контрольной суммы
    	int packet_len = sizeof(struct icmphdr) + sizeof("HELLO");
    	icmp->checksum = checksum(packet, packet_len);

//===================отправка ICMP пакета==========================
    	//установление структуры адреса назначения
    	struct sockaddr_in dest = {
        	.sin_family = AF_INET,
    	};

    	//преобразование ip адреса в сетевой порядок байт и сразу заполнение поля sin_addr
    	inet_pton(AF_INET, argv[1], &dest.sin_addr);

    	//отправка патека на адрес назначения
    	if(sendto(sock, packet, packet_len, 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        	perror("sendto");
    	} else {
        	printf("[+] ICMP sent to %s\n", argv[1]);
    	}

    	close(sock);
    	return 0;
}
