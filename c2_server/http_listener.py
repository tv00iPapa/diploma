import time

def run_http(stop_event, task_manager):
    print("[+/HTTP] http-слушаетль работает")
    while not stop_event.is_set():
        time.sleep(1)
    print("[+/HTTP] http-слушатель прекратил работу.")
