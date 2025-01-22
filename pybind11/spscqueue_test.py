import spscqueue
import pack_message
import datetime

def print_msg(msg):
    print(f"MessageType {msg.MessageType:#X}")
    print("Colo", msg.OrderRequest.Colo)
    print("Broker", msg.OrderRequest.Broker)
    print("ExchangeID", msg.OrderRequest.ExchangeID)
    print("Account", msg.OrderRequest.Account) 
    print("Ticker", msg.OrderRequest.Ticker)
    print("BusinessType", msg.OrderRequest.BusinessType)
    print("OrderType", msg.OrderRequest.OrderType)
    print("Price", msg.OrderRequest.Price)
    print("Volume", msg.OrderRequest.Volume)
    print("Direction", msg.OrderRequest.Direction)
    print("Offset", msg.OrderRequest.Offset)

def TEST_spscqueue():
    queue = spscqueue.SPSCQueue()
    
    msg = pack_message.PackMessage()
    msg.MessageType = pack_message.EMessageType.EOrderRequest
    msg.OrderRequest.Colo = "ZX_F1"
    msg.OrderRequest.Broker = "ZX"
    msg.OrderRequest.Account = "188795"
    msg.OrderRequest.ExchangeID = "SHFE"
    msg.OrderRequest.Ticker = "ag2502"
    msg.OrderRequest.BusinessType = pack_message.EBusinessType.EFUTURE
    msg.OrderRequest.OrderType = pack_message.EOrderType.ELIMIT
    msg.OrderRequest.Price = 112.0
    msg.OrderRequest.Volume = 1
    msg.OrderRequest.Direction = pack_message.EOrderDirection.EBUY
    msg.OrderRequest.Offset = pack_message.EOrderOffset.EOPEN
    msg.OrderRequest.RiskStatus = pack_message.ERiskStatusType.EPREPARE_CHECKED
    msg.OrderRequest.RecvMarketTime = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
    msg.OrderRequest.SendTime = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
    msg.ChannelID = 1
                        
    ret = queue.Push(msg)
    if ret:
        print("Push success")
    ret = queue.Pop(msg)
    if ret:
        print_msg(msg)


if __name__ == "__main__":
    TEST_spscqueue()