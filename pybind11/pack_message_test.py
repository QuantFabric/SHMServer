import pack_message



def TEST_EBusinessType():
    print("Test EBusinessType...")
    print("ESTOCK ", pack_message.EBusinessType.ESTOCK)
    print("ECREDIT ", pack_message.EBusinessType.ECREDIT)
    print("EFUTURE ", pack_message.EBusinessType.EFUTURE)
    print("ESPOT ", pack_message.EBusinessType.ESPOT)
    print("\n")

def TEST_EOrderType():
    print("Test EOrderType...")
    print("EFAK ", pack_message.EOrderType.EFAK)
    print("EFOK ", pack_message.EOrderType.EFOK)
    print("ELIMIT ", pack_message.EOrderType.ELIMIT)
    print("EMARKET ", pack_message.EOrderType.EMARKET)
    print("\n")

def TEST_EOrderDirection():
    print("Test EOrderDirection...")
    print("EBUY ", pack_message.EOrderDirection.EBUY)
    print("ESELL ", pack_message.EOrderDirection.ESELL)
    print("EREVERSE_REPO ", pack_message.EOrderDirection.EREVERSE_REPO)
    print("ESUBSCRIPTION ", pack_message.EOrderDirection.ESUBSCRIPTION)
    print("EALLOTMENT ", pack_message.EOrderDirection.EALLOTMENT)
    print("ECOLLATERAL_TRANSFER_IN ", pack_message.EOrderDirection.ECOLLATERAL_TRANSFER_IN)
    print("ECOLLATERAL_TRANSFER_OUT ", pack_message.EOrderDirection.ECOLLATERAL_TRANSFER_OUT)
    print("EMARGIN_BUY ", pack_message.EOrderDirection.EMARGIN_BUY)
    print("EREPAY_MARGIN_BY_SELL ", pack_message.EOrderDirection.EREPAY_MARGIN_BY_SELL)
    print("ESHORT_SELL ", pack_message.EOrderDirection.ESHORT_SELL)
    print("EREPAY_STOCK_BY_BUY ", pack_message.EOrderDirection.EREPAY_STOCK_BY_BUY)
    print("EREPAY_STOCK_DIRECT ", pack_message.EOrderDirection.EREPAY_STOCK_DIRECT)
    print("\n")

def TEST_EOrderOffset():
    print("Test EOrderOffset...")
    print("EOPEN ", pack_message.EOrderOffset.EOPEN)
    print("ECLOSE ", pack_message.EOrderOffset.ECLOSE)
    print("ECLOSE_TODAY ", pack_message.EOrderOffset.ECLOSE_TODAY)
    print("ECLOSE_YESTODAY ", pack_message.EOrderOffset.ECLOSE_YESTODAY)
    print("\n")

def TEST_ERiskStatusType():
    print("Test ERiskStatusType...")
    print("EPREPARE_CHECKED ", pack_message.ERiskStatusType.EPREPARE_CHECKED)
    print("ECHECKED_PASS ", pack_message.ERiskStatusType.ECHECKED_PASS)
    print("ECHECKED_NOPASS ", pack_message.ERiskStatusType.ECHECKED_NOPASS)
    print("ENOCHECKED ", pack_message.ERiskStatusType.ENOCHECKED)
    print("ECHECK_INIT ", pack_message.ERiskStatusType.ECHECK_INIT)
    print("\n")

def TEST_TOrderRequest():
    print("Test TOrderRequest...")
    order = pack_message.TOrderRequest()
    order.Colo = "ZX_F1"
    order.Broker = "9999"
    order.Product = "至尊宝一号"
    order.Account = "188795"
    order.Ticker = "600000.SH"
    order.ExchangeID = "SH"
    order.BusinessType = pack_message.EBusinessType.ESTOCK
    order.OrderType = pack_message.EOrderType.EMARKET
    order.Direction = pack_message.EOrderDirection.EBUY
    order.Offset = pack_message.EOrderOffset.EOPEN
    order.RiskStatus = pack_message.ERiskStatusType.ECHECK_INIT
    order.OrderToken = 1
    order.EngineID = 1
    order.UserReserved1 = 123
    order.UserReserved2 = 456
    order.Price = 20.20
    order.Volume = 1000
    order.RecvMarketTime = "2024-12-30 10:00:00.123456"
    order.SendTime = "2024-12-30 10:00:00.123678"
    order.RiskID = "9999"
    order.Trader = "天山老妖"
    order.ErrorID = 1
    order.ErrorMsg = "No Error"
    order.UpdateTime = "2024-12-30 10:00:00.123789"
    print("Colo", order.Colo)
    print("Broker", order.Broker)
    print("Product", order.Product)
    print("Account", order.Account)
    print("Ticker", order.Ticker)
    print("ExchangeID", order.ExchangeID)
    print("BusinessType", order.BusinessType)
    print("OrderType", order.OrderType)
    print("Direction", order.Direction)
    print("Offset", order.Offset)
    print("RiskStatus", order.RiskStatus)
    print("OrderToken", order.OrderToken)
    print("EngineID", order.EngineID)
    print("UserReserved1", order.UserReserved1)
    print("UserReserved2", order.UserReserved2)
    print("Price", order.Price)
    print("Volume", order.Volume)
    print("RecvMarketTime", order.RecvMarketTime)
    print("SendTime", order.SendTime)
    print("RiskID", order.RiskID)
    print("Trader", order.Trader)
    print("ErrorID", order.ErrorID)
    print("ErrorMsg", order.ErrorMsg)
    print("UpdateTime", order.UpdateTime)
    print("\n")

def TEST_TActionRequest():
    print("Test TEST_TActionRequest...")
    action = pack_message.TActionRequest()
    action.Colo = "ZX_F1"
    action.Account = "188795"
    action.OrderRef = "1234567890"
    action.ExchangeID = "SH"
    action.BusinessType = pack_message.EBusinessType.ESTOCK
    action.EngineID = 1
    action.RiskStatus = pack_message.ERiskStatusType.ECHECK_INIT
    action.Trader = "天山老妖"
    action.RiskID = "9999"
    action.ErrorID = 1
    action.ErrorMsg = "No Error"
    action.UpdateTime = "2024-12-30 10:00:00.123789"

    print("Colo", action.Colo)
    print("Account", action.Account)
    print("OrderRef", action.OrderRef)
    print("ExchangeID", action.ExchangeID)
    print("BusinessType", action.BusinessType)
    print("EngineID", action.EngineID)
    print("RiskStatus", action.RiskStatus)
    print("Trader", action.Trader)
    print("RiskID", action.RiskID)
    print("ErrorID", action.ErrorID)
    print("ErrorMsg", action.ErrorMsg)
    print("UpdateTime", action.UpdateTime)
    print("\n")

def TEST_EOrderStatusType():
    print("Test EOrderStatusType...")
    print("EORDER_SENDED", pack_message.EOrderStatusType.EORDER_SENDED)
    print("EBROKER_ACK", pack_message.EOrderStatusType.EBROKER_ACK)
    print("EEXCHANGE_ACK", pack_message.EOrderStatusType.EEXCHANGE_ACK)
    print("EPARTTRADED", pack_message.EOrderStatusType.EPARTTRADED)
    print("EALLTRADED", pack_message.EOrderStatusType.EALLTRADED)
    print("ECANCELLING", pack_message.EOrderStatusType.ECANCELLING)
    print("ECANCELLED", pack_message.EOrderStatusType.ECANCELLED)
    print("EPARTTRADED_CANCELLED", pack_message.EOrderStatusType.EPARTTRADED_CANCELLED)
    print("EBROKER_ERROR", pack_message.EOrderStatusType.EBROKER_ERROR)
    print("EEXCHANGE_ERROR", pack_message.EOrderStatusType.EEXCHANGE_ERROR)
    print("EACTION_ERROR", pack_message.EOrderStatusType.EACTION_ERROR)
    print("ERISK_ORDER_REJECTED", pack_message.EOrderStatusType.ERISK_ORDER_REJECTED)
    print("ERISK_ACTION_REJECTED", pack_message.EOrderStatusType.ERISK_ACTION_REJECTED)
    print("ERISK_CHECK_INIT", pack_message.EOrderStatusType.ERISK_CHECK_INIT)
    print("ERISK_CHECK_SELFMATCH", pack_message.EOrderStatusType.ERISK_CHECK_SELFMATCH)
    print("ERISK_CHECK_CANCELLIMIT", pack_message.EOrderStatusType.ERISK_CHECK_CANCELLIMIT)
    print("\n")

def TEST_EOrderSide():
    print("Test EOrderSide...")
    print("EOPEN_LONG", pack_message.EOrderSide.EOPEN_LONG)
    print("ECLOSE_TD_LONG", pack_message.EOrderSide.ECLOSE_TD_LONG)
    print("ECLOSE_YD_LONG", pack_message.EOrderSide.ECLOSE_YD_LONG)
    print("EOPEN_SHORT", pack_message.EOrderSide.EOPEN_SHORT)
    print("ECLOSE_TD_SHORT", pack_message.EOrderSide.ECLOSE_TD_SHORT)
    print("ECLOSE_YD_SHORT", pack_message.EOrderSide.ECLOSE_YD_SHORT)
    print("ECLOSE_LONG", pack_message.EOrderSide.ECLOSE_LONG)
    print("ECLOSE_SHORT", pack_message.EOrderSide.ECLOSE_SHORT)
    print("ESIDE_REVERSE_REPO", pack_message.EOrderSide.ESIDE_REVERSE_REPO)
    print("ESIDE_SUBSCRIPTION", pack_message.EOrderSide.ESIDE_SUBSCRIPTION)
    print("ESIDE_ALLOTMENT", pack_message.EOrderSide.ESIDE_ALLOTMENT)
    print("ESIDE_COLLATERAL_BUY", pack_message.EOrderSide.ESIDE_COLLATERAL_BUY)
    print("ESIDE_COLLATERAL_SELL", pack_message.EOrderSide.ESIDE_COLLATERAL_SELL)
    print("ESIDE_MARGIN_BUY", pack_message.EOrderSide.ESIDE_MARGIN_BUY)
    print("ESIDE_REPAY_MARGIN_BY_SELL", pack_message.EOrderSide.ESIDE_REPAY_MARGIN_BY_SELL)
    print("ESIDE_SHORT_SELL", pack_message.EOrderSide.ESIDE_SHORT_SELL)
    print("ESIDE_REPAY_STOCK_BY_BUY", pack_message.EOrderSide.ESIDE_REPAY_STOCK_BY_BUY)
    print("ESIDE_REPAY_STOCK_DIRECT", pack_message.EOrderSide.ESIDE_REPAY_STOCK_DIRECT)
    print("\n")

def TEST_TOrderStatus():
    print("Test TOrderStatus...")
    order = pack_message.TOrderStatus()
    order.Colo = "ZX_F1"
    order.Broker = "9999"
    order.Product = "至尊宝一号"
    order.Account = "188795"
    order.Ticker = "600000.SH"
    order.ExchangeID = "SH"
    order.BusinessType = pack_message.EBusinessType.ESTOCK
    order.OrderRef = "1234567890"
    order.OrderSysID = "A1234567890"
    order.OrderLocalID = "B1234567890"
    order.OrderToken = 1
    order.EngineID = 1
    order.UserReserved1 = 123
    order.UserReserved2 = 456
    order.OrderType = pack_message.EOrderType.ELIMIT
    order.OrderSide = pack_message.EOrderSide.EOPEN_LONG
    order.OrderStatus = pack_message.EOrderStatusType.EORDER_SENDED
    order.SendPrice = 20.20
    order.SendVolume = 1000
    order.TotalTradedVolume = 1000
    order.TradedAvgPrice = 20.20
    order.TradedVolume = 1000
    order.TradedPrice = 20.20
    order.CanceledVolume = 0
    order.Commission = 0.0
    order.RecvMarketTime = "2024-12-30 10:00:00.123456"
    order.SendTime = "2024-12-30 10:00:00.123678"
    order.InsertTime = "2024-12-30 10:00:00.123789"
    order.BrokerACKTime = "2024-12-30 10:00:00.123890"
    order.ExchangeACKTime = "2024-12-30 10:00:00.123901"
    order.RiskID = "9999"
    order.Trader = "天山老妖"
    order.ErrorID = 1
    order.ErrorMsg = "No Error"
    order.UpdateTime = "2024-12-30 10:00:00.124789"

    print("Colo", order.Colo)
    print("Broker", order.Broker)
    print("Product", order.Product)
    print("Account", order.Account)
    print("Ticker", order.Ticker)
    print("ExchangeID", order.ExchangeID)
    print("BusinessType", order.BusinessType)
    print("OrderRef", order.OrderRef)
    print("OrderSysID", order.OrderSysID)
    print("OrderLocalID", order.OrderLocalID)
    print("OrderToken", order.OrderToken)
    print("EngineID", order.EngineID)
    print("UserReserved1", order.UserReserved1)
    print("UserReserved2", order.UserReserved2)
    print("OrderType", order.OrderType)
    print("OrderSide", order.OrderSide)
    print("OrderStatus", order.OrderStatus)
    print("SendPrice", order.SendPrice)
    print("SendVolume", order.SendVolume)
    print("TotalTradedVolume", order.TotalTradedVolume)
    print("TradedAvgPrice", order.TradedAvgPrice)
    print("TradedVolume", order.TradedVolume)
    print("TradedPrice", order.TradedPrice)
    print("CanceledVolume", order.CanceledVolume)
    print("Commission", order.Commission)
    print("RecvMarketTime", order.RecvMarketTime)
    print("SendTime", order.SendTime)
    print("InsertTime", order.InsertTime)
    print("BrokerACKTime", order.BrokerACKTime)
    print("ExchangeACKTime", order.ExchangeACKTime)
    print("RiskID", order.RiskID)
    print("Trader", order.Trader)
    print("ErrorID", order.ErrorID)
    print("ErrorMsg", order.ErrorMsg)
    print("UpdateTime", order.UpdateTime)
    print("\n")
    
def TEST_TAccountFund():
    print("Test TAccountFund...")
    fund = pack_message.TAccountFund()
    fund.Colo = "ZX_F1"
    fund.Broker = "9999"
    fund.Product = "至尊宝一号"
    fund.Account = "188795"
    fund.BusinessType = pack_message.EBusinessType.ESTOCK
    fund.Deposit = 100000.0
    fund.Withdraw = 10000.0
    fund.CurrMargin = 10000.0
    fund.Commission = 0.0
    fund.CloseProfit = 1000.0
    fund.PositionProfit = 1000.0
    fund.Available = 1000.0
    fund.WithdrawQuota = 1000.0
    fund.ExchangeMargin = 1000.0
    fund.Balance = 1000.0
    fund.PreBalance = 1000.0
    fund.UpdateTime = "2024-12-30 10:00:00.123456"

    print("Colo", fund.Colo)
    print("Broker", fund.Broker)
    print("Product", fund.Product)
    print("Account", fund.Account)
    print("BusinessType", fund.BusinessType)
    print("Deposit", fund.Deposit)
    print("Withdraw", fund.Withdraw)
    print("CurrMargin", fund.CurrMargin)
    print("Commission", fund.Commission)
    print("CloseProfit", fund.CloseProfit)
    print("PositionProfit", fund.PositionProfit)
    print("Available", fund.Available)
    print("WithdrawQuota", fund.WithdrawQuota)
    print("ExchangeMargin", fund.ExchangeMargin)
    print("Balance", fund.Balance)
    print("PreBalance", fund.PreBalance)
    print("UpdateTime", fund.UpdateTime)
    print("\n")

def TEST_TFuturePosition():
    print("Test TFuturePosition...")
    position = pack_message.TFuturePosition()
    position.LongTdVolume = 10000
    position.LongYdVolume = 1000
    position.LongOpenVolume = 1000
    position.LongOpeningVolume = 500
    position.LongClosingTdVolume = 0
    position.LongClosingYdVolume = 0
    position.ShortTdVolume = 10000
    position.ShortYdVolume = 1000
    position.ShortOpenVolume = 1000
    position.ShortOpeningVolume = 0
    position.ShortClosingTdVolume = 0
    position.ShortClosingYdVolume = 0
    
    print("LongTdVolume", position.LongTdVolume)
    print("LongYdVolume", position.LongYdVolume)
    print("LongOpenVolume", position.LongOpenVolume)
    print("LongOpeningVolume", position.LongOpeningVolume)
    print("LongClosingTdVolume", position.LongClosingTdVolume)
    print("LongClosingYdVolume", position.LongClosingYdVolume)
    print("ShortTdVolume", position.ShortTdVolume)
    print("ShortYdVolume", position.ShortYdVolume)
    print("ShortOpenVolume", position.ShortOpenVolume)
    print("ShortOpeningVolume", position.ShortOpeningVolume)
    print("ShortClosingTdVolume", position.ShortClosingTdVolume)
    print("ShortClosingYdVolume", position.ShortClosingYdVolume)
    print("\n")

def TEST_TStockPosition():
    print("Test TStockPosition...")
    position = pack_message.TStockPosition()
    position.LongYdPosition = 10000
    position.LongPosition = 10000
    position.LongTdBuy = 10000
    position.LongTdSell = 10000
    position.MarginYdPosition = 10000
    position.MarginPosition = 10000
    position.MarginTdBuy = 10000
    position.MarginTdSell = 10000
    position.ShortYdPosition = 10000
    position.ShortPosition = 10000
    position.ShortTdSell = 10000
    position.ShortTdBuy = 10000
    position.ShortDirectRepaid = 10000
    position.SpecialPositionAvl = 10000

    print("LongYdPosition", position.LongYdPosition)
    print("LongPosition", position.LongPosition)
    print("LongTdBuy", position.LongTdBuy)
    print("LongTdSell", position.LongTdSell)
    print("MarginYdPosition", position.MarginYdPosition)
    print("MarginPosition", position.MarginPosition)
    print("MarginTdBuy", position.MarginTdBuy)
    print("MarginTdSell", position.MarginTdSell)
    print("ShortYdPosition", position.ShortYdPosition)
    print("ShortPosition", position.ShortPosition)
    print("ShortTdSell", position.ShortTdSell)
    print("ShortTdBuy", position.ShortTdBuy)
    print("ShortDirectRepaid", position.ShortDirectRepaid)
    print("SpecialPositionAvl", position.SpecialPositionAvl)
    print("\n")


def TEST_TAccountPosition():
    print("Test TAccountPosition...")
    position = pack_message.TAccountPosition()
    position.Colo = "ZX_F1"
    position.Broker = "9999"
    position.Product = "至尊宝一号"
    position.Account = "188795"
    position.Ticker = "600000.SH"
    position.ExchangeID = "SH"
    position.BusinessType = pack_message.EBusinessType.EFUTURE
    position.FuturePosition.LongTdVolume = 10000
    position.FuturePosition.LongYdVolume = 1000
    position.FuturePosition.LongOpenVolume = 1000
    position.FuturePosition.LongOpeningVolume = 500
    position.FuturePosition.LongClosingTdVolume = 0
    position.FuturePosition.LongClosingYdVolume = 0
    position.FuturePosition.ShortTdVolume = 10000
    position.FuturePosition.ShortYdVolume = 1000
    position.FuturePosition.ShortOpenVolume = 1000
    position.FuturePosition.ShortOpeningVolume = 0
    position.FuturePosition.ShortClosingTdVolume = 0
    position.FuturePosition.ShortClosingYdVolume = 0
    position.UpdateTime = "2024-12-30 10:00:00.123456"

    print("Colo", position.Colo)
    print("Broker", position.Broker)
    print("Product", position.Product)
    print("Account", position.Account)
    print("Ticker", position.Ticker)
    print("ExchangeID", position.ExchangeID)
    print("BusinessType", position.BusinessType)
    print("FuturePosition.LongTdVolume", position.FuturePosition.LongTdVolume)
    print("FuturePosition.LongYdVolume", position.FuturePosition.LongYdVolume)
    print("FuturePosition.LongOpenVolume", position.FuturePosition.LongOpenVolume)
    print("FuturePosition.LongOpeningVolume", position.FuturePosition.LongOpeningVolume)
    print("FuturePosition.LongClosingTdVolume", position.FuturePosition.LongClosingTdVolume)
    print("FuturePosition.LongClosingYdVolume", position.FuturePosition.LongClosingYdVolume)
    print("FuturePosition.ShortTdVolume", position.FuturePosition.ShortTdVolume)
    print("FuturePosition.ShortYdVolume", position.FuturePosition.ShortYdVolume)
    print("FuturePosition.ShortOpenVolume", position.FuturePosition.ShortOpenVolume)
    print("FuturePosition.ShortOpeningVolume", position.FuturePosition.ShortOpeningVolume)
    print("FuturePosition.ShortClosingTdVolume", position.FuturePosition.ShortClosingTdVolume)
    print("FuturePosition.ShortClosingYdVolume", position.FuturePosition.ShortClosingYdVolume)
    print("UpdateTime", position.UpdateTime)
    print("\n")


def TEST_EMessageType():
    print("Test EMessageType...")
    print("ETest", f"{int(pack_message.EMessageType.ETest):#X}")
    print("ELoginRequest", f"{int(pack_message.EMessageType.ELoginRequest):#X}")
    print("ELoginResponse",  f"{int(pack_message.EMessageType.ELoginResponse):#X}")
    print("ECommand", f"{int(pack_message.EMessageType.ECommand):#X}")
    print("EEventLog", f"{int(pack_message.EMessageType.EEventLog):#X}")
    print("EOrderStatus", f"{int(pack_message.EMessageType.EOrderStatus):#X}")
    print("EAccountFund", f"{int(pack_message.EMessageType.EAccountFund):#X}")
    print("EAccountPosition", f"{int(pack_message.EMessageType.EAccountPosition):#X}")
    print("EOrderRequest", f"{int(pack_message.EMessageType.EOrderRequest):#X}")
    print("EActionRequest", f"{int(pack_message.EMessageType.EActionRequest):#X}")
    print("ERiskReport", f"{int(pack_message.EMessageType.ERiskReport):#X}")
    print("EColoStatus", f"{int(pack_message.EMessageType.EColoStatus):#X}")
    print("EAppStatus", f"{int(pack_message.EMessageType.EAppStatus):#X}")
    print("EFastOrder", f"{int(pack_message.EMessageType.EFastOrder):#X}")
    print("EFutureMarketData", f"{int(pack_message.EMessageType.EFutureMarketData):#X}")
    print("EStockMarketData", f"{int(pack_message.EMessageType.EStockMarketData):#X}")
    print("ESpotMarketData", f"{int(pack_message.EMessageType.ESpotMarketData):#X}")
    print("\n")

def TEST_PackMessage():
    print("Test PackMessage...")
    msg = pack_message.PackMessage()
    msg.MessageType = pack_message.EMessageType.ETest
    msg.ChannelID = 1
    msg.TimeStamp = 1234567890
    
    order = pack_message.TOrderStatus()
    order.Colo = "ZX_F1"
    order.Broker = "9999"
    order.Product = "至尊宝一号"
    order.Account = "188795"
    order.Ticker = "600000.SH"
    order.ExchangeID = "SH"
    order.BusinessType = pack_message.EBusinessType.ESTOCK
    order.OrderRef = "1234567890"
    order.OrderSysID = "A1234567890"
    order.OrderLocalID = "B1234567890"
    order.OrderToken = 1
    order.EngineID = 1
    order.UserReserved1 = 123
    order.UserReserved2 = 456
    order.OrderType = pack_message.EOrderType.ELIMIT
    order.OrderSide = pack_message.EOrderSide.EOPEN_LONG
    order.OrderStatus = pack_message.EOrderStatusType.EORDER_SENDED
    order.SendPrice = 20.20
    order.SendVolume = 1000
    order.TotalTradedVolume = 1000
    order.TradedAvgPrice = 20.20
    order.TradedVolume = 1000
    order.TradedPrice = 20.20
    order.CanceledVolume = 0
    order.Commission = 0.0
    order.RecvMarketTime = "2024-12-30 10:00:00.123456"
    order.SendTime = "2024-12-30 10:00:00.123678"
    order.InsertTime = "2024-12-30 10:00:00.123789"
    order.BrokerACKTime = "2024-12-30 10:00:00.123890"
    order.ExchangeACKTime = "2024-12-30 10:00:00.123901"
    order.RiskID = "9999"
    order.Trader = "天山老妖"
    order.ErrorID = 1
    order.ErrorMsg = "No Error"
    order.UpdateTime = "2024-12-30 10:00:00.124789"
    
    
    print("MessageType", f"{msg.MessageType:#X}")
    print("ChannelID", msg.ChannelID)
    print("TimeStamp", msg.TimeStamp)
    print("OrderRef", msg.OrderStatus.OrderRef)
    print("OrderSysID", msg.OrderStatus.OrderSysID)
    print("OrderLocalID", msg.OrderStatus.OrderLocalID)
    print("OrderToken", msg.OrderStatus.OrderToken)
    print("EngineID", msg.OrderStatus.EngineID)
    print("UserReserved1", msg.OrderStatus.UserReserved1)
    print("UserReserved2", msg.OrderStatus.UserReserved2)
    print("OrderType", msg.OrderStatus.OrderType)
    print("OrderSide", msg.OrderStatus.OrderSide)
    print("OrderStatus", msg.OrderStatus.OrderStatus)
    print("SendPrice", msg.OrderStatus.SendPrice)
    print("SendVolume", msg.OrderStatus.SendVolume)
    print("TotalTradedVolume", msg.OrderStatus.TotalTradedVolume)
    print("TradedAvgPrice", msg.OrderStatus.TradedAvgPrice)
    print("TradedVolume", msg.OrderStatus.TradedVolume)
    print("TradedPrice", msg.OrderStatus.TradedPrice)
    print("CanceledVolume", msg.OrderStatus.CanceledVolume)
    print("Commission", msg.OrderStatus.Commission)
    print("RecvMarketTime", msg.OrderStatus.RecvMarketTime)
    print("SendTime", msg.OrderStatus.SendTime)
    print("InsertTime", msg.OrderStatus.InsertTime)
    print("BrokerACKTime", msg.OrderStatus.BrokerACKTime)
    print("ExchangeACKTime", msg.OrderStatus.ExchangeACKTime)
    print("RiskID", msg.OrderStatus.RiskID)
    print("Trader", msg.OrderStatus.Trader)
    print("ErrorID", msg.OrderStatus.ErrorID)
    print("ErrorMsg", msg.OrderStatus.ErrorMsg)
    print("UpdateTime", msg.OrderStatus.UpdateTime)
    print("\n")
    
    
if __name__ == "__main__":
    TEST_EBusinessType()
    TEST_EOrderType()
    TEST_EOrderDirection()
    TEST_EOrderOffset()
    TEST_ERiskStatusType()
    TEST_TOrderRequest()
    TEST_TActionRequest()
    TEST_EOrderStatusType()
    TEST_EOrderSide()
    TEST_TOrderStatus()
    TEST_TAccountFund()
    TEST_TFuturePosition()
    TEST_TStockPosition()
    TEST_TAccountPosition()
    TEST_EMessageType()
    TEST_PackMessage()
    
    