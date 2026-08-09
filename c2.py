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

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8080)
