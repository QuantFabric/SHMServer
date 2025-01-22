#include "Common.hpp"
#include "SHMConnection.hpp"
#include "PackMessage.hpp"
#include <time.h>
#include <sched.h>
#include <vector>
#include <string>
#include <thread>
#include <stdio.h>

struct ClientConf : public SHMIPC::CommonConf
{
    static const bool Performance = false;
};


void PrintFutureMarketData(const Message::PackMessage& msg)
{
    printf("MessageType: %#X\n", msg.MessageType);
    printf("Colo: %s\n", msg.FutureMarketData.Colo);
    printf("Broker: %s\n", msg.FutureMarketData.Broker);
    printf("ExchangeID: %s\n", msg.FutureMarketData.ExchangeID);
    printf("TradingDay: %s\n", msg.FutureMarketData.TradingDay);
    printf("ActionDay: %s\n", msg.FutureMarketData.ActionDay);
    printf("UpdateTime:%s\n", msg.FutureMarketData.UpdateTime);
    printf("MillSec:%03d\n", msg.FutureMarketData.MillSec);
    printf("LastPrice:%.3f\n", msg.FutureMarketData.LastPrice); 
    printf("Volume: %d\n", msg.FutureMarketData.Volume);
    printf("Turnover: %.3f\n", msg.FutureMarketData.Turnover);
    printf("OpenPrice: %.3f\n", msg.FutureMarketData.OpenPrice);
    printf("ClosePrice: %.3f\n", msg.FutureMarketData.ClosePrice);
    printf("PreClosePrice: %.3f\n", msg.FutureMarketData.PreClosePrice);
    printf("SettlementPrice: %.3f\n", msg.FutureMarketData.SettlementPrice);
    printf("PreSettlementPrice: %.3f\n", msg.FutureMarketData.PreSettlementPrice);
    printf("OpenInterest: %d\n", msg.FutureMarketData.OpenInterest);
    printf("PreOpenInterest: %d\n", msg.FutureMarketData.PreOpenInterest);
    printf("HighestPrice: %.3d\n", msg.FutureMarketData.HighestPrice);
    printf("LowestPrice: %.3f\n", msg.FutureMarketData.LowestPrice);
    printf("UpperLimitPrice: %.3f\n", msg.FutureMarketData.UpperLimitPrice);
    printf("LowerLimitPrice: %.3f\n", msg.FutureMarketData.LowerLimitPrice);
    printf("BidPrice1: %.3f\n", msg.FutureMarketData.BidPrice1);
    printf("BidVolume1: %d\n", msg.FutureMarketData.BidVolume1);
    printf("AskPrice1: %.3f\n", msg.FutureMarketData.AskPrice1);
    printf("AskVolume1: %d\n", msg.FutureMarketData.AskVolume1);
    printf("RecvLocalTime: %s\n", msg.FutureMarketData.RecvLocalTime);
}

void PrintOrderRequest(const Message::PackMessage& msg)
{
    printf("MessageType: %#X\n", msg.MessageType);
    printf("Colo: %s\n", msg.OrderRequest.Colo);
    printf("Broker: %s\n", msg.OrderRequest.Broker);
    printf("Product: %s\n", msg.OrderRequest.Product);
    printf("Account: %s\n", msg.OrderRequest.Account);
    printf("Ticker: %s\n", msg.OrderRequest.Ticker);
    printf("ExchangeID: %s\n", msg.OrderRequest.ExchangeID);
    printf("BusinessType: %d\n", msg.OrderRequest.BusinessType);
    printf("OrderType: %d\n", msg.OrderRequest.OrderType);
    printf("Direction: %d\n", msg.OrderRequest.Direction);
    printf("Offset: %d\n", msg.OrderRequest.Offset);
    printf("RiskStatus: %d\n", msg.OrderRequest.RiskStatus);
    printf("OrderToken: %d\n", msg.OrderRequest.OrderToken);
    printf("EngineID: %d\n", msg.OrderRequest.EngineID);
    printf("UserReserved1: %d\n", msg.OrderRequest.UserReserved1);
    printf("UserReserved2: %d\n", msg.OrderRequest.UserReserved2);
    printf("Price: %.3f\n", msg.OrderRequest.Price);
    printf("Volume: %d\n", msg.OrderRequest.Volume);
    printf("RecvMarketTime: %s\n", msg.OrderRequest.RecvMarketTime);
    printf("SendTime: %s\n", msg.OrderRequest.SendTime);
    printf("RiskID: %s\n", msg.OrderRequest.RiskID);
    printf("Trader: %s\n", msg.OrderRequest.Trader);
    printf("ErrorID: %d\n", msg.OrderRequest.ErrorID);
    printf("ErrorMsg: %s\n", msg.OrderRequest.ErrorMsg);
    printf("UpdateTime: %s\n", msg.OrderRequest.UpdateTime);
}

void PrintOderStatus(const Message::PackMessage& msg)
{
    printf("MessageType: %#X\n", msg.MessageType);
    printf("Colo: %s\n", msg.OrderStatus.Colo);
    printf("Broker: %s\n", msg.OrderStatus.Broker);
    printf("Product: %s\n", msg.OrderStatus.Product);
    printf("Account: %s\n", msg.OrderStatus.Account);
    printf("Ticker: %s\n", msg.OrderStatus.Ticker);
    printf("ExchangeID: %s\n", msg.OrderStatus.ExchangeID);
    printf("BusinessType: %d\n", msg.OrderStatus.BusinessType);
    printf("OrderRef: %s\n", msg.OrderStatus.OrderRef);
    printf("OrderSysID: %s\n", msg.OrderStatus.OrderSysID);
    printf("OrderLocalID: %s\n", msg.OrderStatus.OrderLocalID);
    printf("OrderToken: %d\n", msg.OrderStatus.OrderToken);
    printf("EngineID: %d\n", msg.OrderStatus.EngineID);
    printf("UserReserved1: %d\n", msg.OrderStatus.UserReserved1);
    printf("UserReserved2: %d\n", msg.OrderStatus.UserReserved2);
    printf("OrderType: %d\n", msg.OrderStatus.OrderType);
    printf("OrderSide: %d\n", msg.OrderStatus.OrderSide);
    printf("OrderStatus: %d\n", msg.OrderStatus.OrderStatus);
    printf("SendPrice: %.3f\n", msg.OrderStatus.SendPrice);
    printf("SendVolume: %d\n", msg.OrderStatus.SendVolume);
    printf("TotalTradedVolume: %d\n", msg.OrderStatus.TotalTradedVolume);
    printf("TradedAvgPrice: %.3f\n", msg.OrderStatus.TradedAvgPrice);
    printf("TradedVolume: %d\n", msg.OrderStatus.TradedVolume);
    printf("TradedPrice: %.3f\n", msg.OrderStatus.TradedPrice);
    printf("CanceledVolume: %d\n", msg.OrderStatus.CanceledVolume);
    printf("Commission: %.3f\n", msg.OrderStatus.Commission);
    printf("RecvMarketTime: %s\n", msg.OrderStatus.RecvMarketTime);
    printf("SendTime: %s\n", msg.OrderStatus.SendTime);
    printf("InsertTime: %s\n", msg.OrderStatus.InsertTime);
    printf("BrokerACKTime: %s\n", msg.OrderStatus.BrokerACKTime);
    printf("ExchangeACKTime: %s\n", msg.OrderStatus.ExchangeACKTime);
    printf("RiskID: %s\n", msg.OrderStatus.RiskID);
    printf("Trader: %s\n", msg.OrderStatus.Trader);
    printf("ErrorID: %d\n", msg.OrderStatus.ErrorID);
    printf("ErrorMsg: %s\n", msg.OrderStatus.ErrorMsg);
    printf("UpdateTime: %s\n", msg.OrderStatus.UpdateTime);
}

void PrintAccountFund(const Message::PackMessage& msg)
{
    printf("MessageType: %#X\n", msg.MessageType);
    printf("Colo: %s\n", msg.AccountFund.Colo);
    printf("Broker: %s\n", msg.AccountFund.Broker);
    printf("Product: %s\n", msg.AccountFund.Product);
    printf("Account: %s\n", msg.AccountFund.Account);
    printf("BusinessType: %d\n", msg.AccountFund.BusinessType);
    printf("Deposit: %.3f\n", msg.AccountFund.Deposit);
    printf("Withdraw: %.3f\n", msg.AccountFund.Withdraw);
    printf("CurrMargin: %.3f\n", msg.AccountFund.CurrMargin);
    printf("Commission: %.3f\n", msg.AccountFund.Commission);
    printf("CloseProfit: %.3f\n", msg.AccountFund.CloseProfit);
    printf("PositionProfit: %.3f\n", msg.AccountFund.PositionProfit);
    printf("Available: %.3f\n", msg.AccountFund.Available);
    printf("WithdrawQuota: %.3f\n", msg.AccountFund.WithdrawQuota);
    printf("ExchangeMargin: %.3f\n", msg.AccountFund.ExchangeMargin);
    printf("Balance: %.3f\n", msg.AccountFund.Balance);
    printf("PreBalance: %.3f\n", msg.AccountFund.PreBalance);
    printf("UpdateTime: %s\n", msg.AccountFund.UpdateTime);
}


void PrintAccountPosition(const Message::PackMessage& msg)
{
    printf("MessageType: %#X\n", msg.MessageType);
    printf("Colo: %s\n", msg.AccountPosition.Colo);
    printf("Broker: %s\n", msg.AccountPosition.Broker);
    printf("Product: %s\n", msg.AccountPosition.Product);
    printf("Account: %s\n", msg.AccountPosition.Account);
    printf("Ticker: %s\n", msg.AccountPosition.Ticker);
    printf("ExchangeID: %s\n", msg.AccountPosition.ExchangeID);
    printf("BusinessType: %d\n", msg.AccountPosition.BusinessType);
    if(msg.AccountPosition.BusinessType == Message::EBusinessType::EFUTURE)
    {
        printf("LongTdVolume: %d\n", msg.AccountPosition.FuturePosition.LongTdVolume);
        printf("LongYdVolume: %d\n", msg.AccountPosition.FuturePosition.LongYdVolume);
        printf("LongOpenVolume: %d\n", msg.AccountPosition.FuturePosition.LongOpenVolume);
        printf("LongOpeningVolume: %d\n", msg.AccountPosition.FuturePosition.LongOpeningVolume);
        printf("LongClosingTdVolume: %d\n", msg.AccountPosition.FuturePosition.LongClosingTdVolume);
        printf("LongClosingYdVolume: %d\n", msg.AccountPosition.FuturePosition.LongClosingYdVolume);
        printf("ShortTdVolume: %d\n", msg.AccountPosition.FuturePosition.ShortTdVolume);
        printf("ShortYdVolume: %d\n", msg.AccountPosition.FuturePosition.ShortYdVolume);
        printf("ShortOpenVolume: %d\n", msg.AccountPosition.FuturePosition.ShortOpenVolume);
        printf("ShortOpeningVolume: %d\n", msg.AccountPosition.FuturePosition.ShortOpeningVolume);
        printf("ShortClosingTdVolume: %d\n", msg.AccountPosition.FuturePosition.ShortClosingTdVolume);
        printf("ShortClosingYdVolume: %d\n", msg.AccountPosition.FuturePosition.ShortClosingYdVolume);
    }
    else if(msg.AccountPosition.BusinessType == Message::EBusinessType::ESTOCK)
    {
        printf("LongYdPosition: %d\n", msg.AccountPosition.StockPosition.LongYdPosition);
        printf("LongPosition: %d\n", msg.AccountPosition.StockPosition.LongPosition);
        printf("LongTdBuy: %d\n", msg.AccountPosition.StockPosition.LongTdBuy);
        printf("LongTdSell: %d\n", msg.AccountPosition.StockPosition.LongTdSell);
        printf("MarginYdPosition: %d\n", msg.AccountPosition.StockPosition.MarginYdPosition);
        printf("MarginPosition: %d\n", msg.AccountPosition.StockPosition.MarginPosition);
        printf("MarginTdBuy: %d\n", msg.AccountPosition.StockPosition.MarginTdBuy);
        printf("MarginTdSell: %d\n", msg.AccountPosition.StockPosition.MarginTdSell);
        printf("ShortYdPosition: %d\n", msg.AccountPosition.StockPosition.ShortYdPosition);
        printf("ShortPosition: %d\n", msg.AccountPosition.StockPosition.ShortPosition);
        printf("ShortTdSell: %d\n", msg.AccountPosition.StockPosition.ShortTdSell);
        printf("ShortTdBuy: %d\n", msg.AccountPosition.StockPosition.ShortTdBuy);
        printf("ShortDirectRepaid: %d\n", msg.AccountPosition.StockPosition.ShortDirectRepaid);
        printf("SpecialPositionAvl: %d\n", msg.AccountPosition.StockPosition.SpecialPositionAvl);
    }
    printf("UpdateTime: %s\n", msg.AccountPosition.UpdateTime);
}

int main(int argc, char** argv) 
{
    if(argc < 3)
    {
        printf("Usage: %s <ClientName> <ServerName>\n", argv[0]);
        return -1;
    }

    SHMIPC::SHMConnection<Message::PackMessage, ClientConf> client(argv[1]);
    client.Start(argv[2]);
    Message::PackMessage recvMsg;
    while(true)
    {
        if constexpr(!ClientConf::Performance)
        {
            client.HandleMsg();
        }
        if(client.Pop(recvMsg))
        {
            if(recvMsg.MessageType == Message::EMessageType::EFutureMarketData)
            {
                PrintFutureMarketData(recvMsg);
            }
            else if(recvMsg.MessageType == Message::EMessageType::EOrderRequest)
            {
                PrintOrderRequest(recvMsg);
            }
            else if(recvMsg.MessageType == Message::EMessageType::EOrderStatus)
            {
                PrintOderStatus(recvMsg);
            }
            else if(recvMsg.MessageType == Message::EMessageType::EAccountFund)
            {
                PrintAccountFund(recvMsg);
            }
            else if(recvMsg.MessageType == Message::EMessageType::EAccountPosition)
            {
                PrintAccountPosition(recvMsg);
            }
            else
            {
                printf("recvMsg.MessageType = 0X%X\n", recvMsg.MessageType);
            }
        }
        else
        {
            usleep(1000);
        }
    }
    
    return 0;
}