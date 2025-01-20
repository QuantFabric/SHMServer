from multiprocessing import Process, Queue, Manager
import shm_connection
import shm_server
import pack_message
import time

class SHMServer(shm_server.SHMServer):
    def __init__(self, order_queue, report_queue):
        super().__init__()
        self.order_queue = order_queue
        self.report_queue = report_queue

    def HandleMsg(self):
        # 订单数据处理
        while (not self.order_queue.empty()):
            msg = order_queue.get()
            self.Push(msg)
        # 回报数据处理
        msg = pack_message.PackMessage()
        ret = self.Pop(msg)
        if ret:
            self.report_queue.put(msg)
        

def recv_market_data(client_name, server_name, market_queue):
    connection = shm_connection.SHMConnection(client_name)
    connection.Start(server_name)
    msg = pack_message.PackMessage()
    while True:
        connection.HandleMsg()
        ret = connection.Pop(msg)
        if ret:
            market_queue.put(msg)
            
            
def shm_server_service(server_name, order_queue, report_queue):
    server = SHMServer(order_queue, report_queue)
    server.Start(server_name)
    while True:
        server.HandleMsg()
            
            
def print_msg(msg):
    print(f"MessageType {msg.MessageType:#X}")
    print("Colo", msg.FutureMarketData.Colo)
    print("Broker", msg.FutureMarketData.Broker)
    print("ExchangeID", msg.FutureMarketData.ExchangeID)
    print("TradingDay", msg.FutureMarketData.TradingDay)
    print("ActionDay", msg.FutureMarketData.ActionDay)
    print("UpdateTime", msg.FutureMarketData.UpdateTime)
    print("MillSec", msg.FutureMarketData.MillSec)
    print("LastPrice", msg.FutureMarketData.LastPrice)
    print("Volume", msg.FutureMarketData.Volume)
    print("Turnover", msg.FutureMarketData.Turnover)
    print("OpenPrice", msg.FutureMarketData.OpenPrice)
    print("ClosePrice", msg.FutureMarketData.ClosePrice)
    print("PreClosePrice", msg.FutureMarketData.PreClosePrice)
    print("SettlementPrice", msg.FutureMarketData.SettlementPrice)
    print("PreSettlementPrice", msg.FutureMarketData.PreSettlementPrice)
    print("OpenInterest", msg.FutureMarketData.OpenInterest)
    print("PreOpenInterest", msg.FutureMarketData.PreOpenInterest)
    print("HighestPrice", msg.FutureMarketData.HighestPrice)
    print("LowestPrice", msg.FutureMarketData.LowestPrice)
    print("UpperLimitPrice", msg.FutureMarketData.UpperLimitPrice)
    print("LowerLimitPrice", msg.FutureMarketData.LowerLimitPrice)
    print("BidPrice1", msg.FutureMarketData.BidPrice1)
    print("BidVolume1", msg.FutureMarketData.BidVolume1)
    print("AskPrice1", msg.FutureMarketData.AskPrice1)
    print("AskVolume1", msg.FutureMarketData.AskVolume1)
    print("RecvLocalTime", msg.FutureMarketData.RecvLocalTime)
    print("CurrentTime", datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f"))
        
        
def strategy_server(market_queue, report_queue, order_queue):
    while True:
        # 行情数据处理
        while (not market_queue.empty()):
            msg = market_queue.get()
            if msg is None:
                continue
            print("recv market data")
            if msg.MessageType == pack_message.EMessageType.EFutureMarketData:
                print_msg(msg)
            
        # 处理回报数据
        while (not report_queue.empty()):
            msg = report_queue.get()
            if msg is None:
                continue
            print("recv report")
        
        # 处理订单数据
        # msg = pack_message.PackMessage()
        # order_queue.put(msg)
        # print("send order data", msg)

    
            
if __name__ == "__main__":
    
    # 行情数据队列
    market_queue = Manager().Queue()
    # 订单数据队列
    order_queue = Manager().Queue()
    # 回报数据队列
    report_queue = Manager().Queue()

    job_list = []
    # 行情数据进程
    client_name = "QuantServer"
    server_name = "MarketServer"
    job = Process(target=recv_market_data, args=(client_name, server_name, market_queue)) 
    job_list.append(job)
    job.start()
    
    # SHMServer进程
    server_name = "QuantServer"
    job = Process(target=shm_server_service, args=(server_name, order_queue, report_queue))
    job_list.append(job)
    job.start()
    
    # 策略进程
    job = Process(target=strategy_server, args=(market_queue, report_queue, order_queue))
    job_list.append(job)
    job.start()
    
    # 等待所有进程结束
    for job in job_list:
        job.join()