import sqlite3

class TaskManager:
    
    db_path = "tasks.db"

    #intialization database(create table tasks)
    def __init__(self, path):
        self.db_path = path
        con = sqlite3.connect(self.db_path)
        cur = con.cursor()
        
        cur.execute("""
            CREATE TABLE IF NOT EXISTS tasks(
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                command TEXT NOT NULL,
                agent_id TEXT,
                status TEXT NOT NULL DEFAULT 'pending' CHECK( status IN ('pending', 'assigned', 'completed', 'failed') ),
                result TEXT,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        """) #create database
        
        con.close()

    #return connection for database
    def get_connection(self):
        return sqlite3.connect(self.db_path) #new connection

    def add_task(self, con, command, agent_id=None):
        cur = con.cursor()

        cur.execute("""
            INSERT INTO tasks (command, agent_id) VALUES
            (?, ?)
        """, (command, agent_id))

        con.commit()
    
    #возвращает команду в виде строки либо None
    def get_next_task(self, con, agent_id):
        cur = con.cursor()

        cur.execute("""
            UPDATE tasks
            SET status = 'assigned', agent_id = ?, updated_at = CURRENT_TIMESTAMP
            WHERE id = (
                SELECT id FROM tasks WHERE status = 'pending' ORDER BY id LIMIT 1
            )
            RETURNING command
        """, (agent_id,))

        res = cur.fetchone()
        con.commit()
        return res[0] if res else None

    def complete_task(self, con, task_id, result):
        cur = con.cursor()

        cur.execute("""
            UPDATE tasks
            SET status = 'completed', result = ?, updated_at = CURRENT_TIMESTAMP
            WHERE id = ?
        """, (result, task_id))
        
        con.commit()
        if cur.rowcount > 0:
            return True
        else:
            return False

    def fail_task(self, con, task_id, error_msg):
        cur = con.cursor()

        cur.execute("""
            UPDATE tasks
            SET status = 'failed', result = ?, updated_at = CURRENT_TIMESTAMP
            WHERE id = ?
        """, (error_msg, task_id))

        con.commit()

    def list_tasks(self, con, status=None):
        cur = con.cursor()
        
        if status is None:
            cur.execute("""
                SELECT id, command, status FROM tasks
            """)
        else:
            cur.execute("""
                SELECT id, command, status FROM tasks WHERE status = ?
            """, (status,))

        return cur.fetchall()

    def get_task_result(self, con, task_id):
        cur = con.cursor()

        cur.execute("""
            SELECT result FROM tasks WHERE id = ?
        """, (task_id,))

        return cur.fetchone()
