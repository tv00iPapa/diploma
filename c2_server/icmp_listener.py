import socket
import struct

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
    print("[+/ICMP] icmp-слушатель работает");

    s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_ICMP)
    while not stop_event.is_set():
        pass

    print("[+/ICMP] icmp-слушатель прекратил работу.")
