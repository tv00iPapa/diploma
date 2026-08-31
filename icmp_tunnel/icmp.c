#include <stdio.h>
#include <inttypes.h>

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
	struct icmphdr *imcp = (struct icmphdr*)packet;
	
	//заполнение полей заголовка icmp
	icmp->type = ICMP_ECHO; //type 8(request)
	icmp->code = 0; //0 - без ососбого значения
	icmp->un.echo.id = htons(0x66); //идентификатор (можно писать все что угодно)
	icmp->un.echo.sequence = htons(1); //серийный номер (нужен для отправка нескольких запросов)
	
	//добавление данных(полезной нагрузки) после заголовка
































}
