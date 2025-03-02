import shm_data_connection
import pack_message
import time
import datetime


class MarketClient(object):
    def __init__(self, client_name, server_name):
        self.client_name = client_name
        self.server_name = server_name
        self.shm_connection = shm_data_connection.SHMConnection(self.client_name)
        self.msg = pack_message.PackMessage()
        
    def start(self):
        self.shm_connection.Start(self.server_name)
        print(f"Client {self.client_name} connected to {self.server_name}")

        
    def run(self):
        print(f"Client {self.client_name} Satrt recv data from {self.server_name}")
        # 收取行情数据
        while True:
            self.shm_connection.HandleMsg()
            if self.shm_connection.Pop(self.msg):
                if self.msg.MessageType == pack_message.EMessageType.EFutureMarketData:
                    self.print_msg(self.msg)

    @staticmethod
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


if __name__ == "__main__":
    client_name = "188795"
    server_name = "MarketServer"
    client = MarketClient(client_name, server_name)
    client.start()
    client.run()