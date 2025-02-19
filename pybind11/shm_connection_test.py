import shm_connection
import pack_message
import time


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

def TEST_shm_connection():
    client = shm_connection.SHMConnection("188795")
    client.Start(server_name="shm_server_test")
    while not client.IsConnected():
        client.HandleMsg()
        time.sleep(1)
        
    order = pack_message.PackMessage()
    order.MessageType = pack_message.EMessageType.EOrderRequest
    order.OrderRequest.Colo = "ZX_F1"
    order.OrderRequest.Broker = "ZX"
    order.OrderRequest.ExchangeID = "SHFE"
    order.OrderRequest.Account = "188796"
    order.OrderRequest.Ticker = "ag2501"
    order.OrderRequest.BusinessType = pack_message.EBusinessType.EFUTURE
    order.OrderRequest.OrderType = pack_message.EOrderType.ELIMIT
    order.OrderRequest.Price = 112.0
    order.OrderRequest.Volume = 10
    order.OrderRequest.Direction = pack_message.EOrderDirection.EBUY
    order.OrderRequest.Offset = pack_message.EOrderOffset.EOPEN
    client.Push(order)
    
    while True:
        client.HandleMsg()
        msg = pack_message.PackMessage()
        ret = client.Pop(msg)
        if ret:
            print_msg(msg)
            client.Push(order)
            
        if not client.IsConnected():
            break
        time.sleep(1)


if __name__ == "__main__":
    TEST_shm_connection()