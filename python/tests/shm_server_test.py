import shm_server
import pack_message
import time


class SHMServer(shm_server.SHMServer):
    def __init__(self):
        super().__init__()

    def HandleMsg(self):
        # 将消息回显
        msg = pack_message.PackMessage()
        ret = self.Pop(msg)
        if ret:
            self.Push(msg=msg)

    def Run(self):
        while True:
            self.PollMsg()
            self.HandleMsg()
    

def TEST_shm_server():
    
    try:
        server = SHMServer()
        server.Start(server_name="shm_server_test")
        server.Run()
    except KeyboardInterrupt:
        server.Stop()
    
    
if __name__ == "__main__":
    TEST_shm_server()