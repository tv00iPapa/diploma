import sqlite3

class TaskManager:
    
    db_path = "tasks.db"

    def __init__(self, path):
        self.db_path = path
        self.con = sqlite3.connect(self.db_path)
        cur = self.con.cursor()
        
        cur.execute("""
            CREATE TABLE IF NOT EXISTS tasks(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                command TEXT NOT NULL,
                agent_id TEXT,
                status TEXT NOT NULL DEFAULT 'pending' CHECK( status IN ('pending', 'assigned', 'complite', 'failed') ),
                result TEXT,
                craeted_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                update_at DATETIME
            )
        """) #create database
        
        self.con.close()

    def get_connection(self):
        return sqlite3.connect(self.db_path) #new connection

    def add_task(self, con, command, agent_id=None):
        cur = con.cursor()

        cur.execute("""
            INSERT INTO tasks (command, agent_id) VALUES
            (?, ?)
        """, (command, agent_id))

        con.commit()

cl = TaskManager("tasks.db")
con = cl.get_connection()
cl.add_task(con, "whoami")
cl.add_task(con, "id")
res = con.execute("SELECT * FROM tasks")
con.close()
print(res.fetchall())

