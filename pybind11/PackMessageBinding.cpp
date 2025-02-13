#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h> // 用于处理 numpy 数组
#include <string.h>
#include "PackMessage.hpp"
#include "MarketData.hpp"
#include "Common.hpp"

namespace py = pybind11;


PYBIND11_MODULE(pack_message, m) {

    m.doc() = "pybind11 bind PackMessage";

    // 绑定 MsgType 枚举
    py::enum_<SHMIPC::EMsgType>(m, "EMsgType")
        .value("EMSG_TYPE_DATA", SHMIPC::EMsgType::EMSG_TYPE_DATA)
        .value("EMSG_TYPE_LOGIN", SHMIPC::EMsgType::EMSG_TYPE_LOGIN)
        .value("EMSG_TYPE_SERVER_ACK", SHMIPC::EMsgType::EMSG_TYPE_SERVER_ACK)
        .value("EMSG_TYPE_CLIENT_ACK", SHMIPC::EMsgType::EMSG_TYPE_CLIENT_ACK)
        .value("EMSG_TYPE_HEARTBEAT", SHMIPC::EMsgType::EMSG_TYPE_HEARTBEAT)
        .export_values();

    // ChannelMsg template binding
    py::class_<SHMIPC::TChannelMsg<Message::PackMessage>>(m, "TChannelMsg")
        .def(py::init<>())
        .def_readwrite("MsgType", &SHMIPC::TChannelMsg<Message::PackMessage>::MsgType)
        .def_readwrite("MsgID", &SHMIPC::TChannelMsg<Message::PackMessage>::MsgID)
        .def_readwrite("TimeStamp", &SHMIPC::TChannelMsg<Message::PackMessage>::TimeStamp)
        .def_readwrite("ChannelID", &SHMIPC::TChannelMsg<Message::PackMessage>::ChannelID)
        .def_property("Data",
            [](const SHMIPC::TChannelMsg<Message::PackMessage>& self) {
                return self.Data;
            },
            [](SHMIPC::TChannelMsg<Message::PackMessage>& self, const Message::PackMessage& value) {
                self.Data = value;
            });

    // EClientType enum binding
    py::enum_<Message::EClientType>(m, "EClientType")
        .value("EXTRADER", Message::EClientType::EXTRADER)
        .value("EXMONITOR", Message::EClientType::EXMONITOR)
        .value("EXMARKETCENTER", Message::EClientType::EXMARKETCENTER)
        .value("EXRISKJUDGE", Message::EClientType::EXRISKJUDGE)  
        .value("EXWATCHER", Message::EClientType::EXWATCHER)
        .value("EXQUANT", Message::EClientType::EXQUANT)
        .value("EHFTRADER", Message::EClientType::EHFTRADER)   
        .value("EXDATAPLAYER", Message::EClientType::EXDATAPLAYER)   
        .export_values(); 

    py::class_<Message::TLoginRequest>(m, "TLoginRequest")
        .def(py::init<>())
        .def_readwrite("ClientType", &Message::TLoginRequest::ClientType)
        .def_property("Colo",
            [](const Message::TLoginRequest& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](Message::TLoginRequest& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Account",
            [](const Message::TLoginRequest& self) {
                return std::string(self.Account);  // 返回 bytes 类型
            },
            [](Message::TLoginRequest& self, const std::string& value) {
                std::strncpy(self.Account, value.c_str(), sizeof(self.Account) - 1);
                self.Account[sizeof(self.Account) - 1] = '\0';
            })
        .def_property("PassWord",
            [](const Message::TLoginRequest& self) {
                return std::string(self.PassWord);  // 返回 bytes 类型
            },
            [](Message::TLoginRequest& self, const std::string& value) {
                std::strncpy(self.PassWord, value.c_str(), sizeof(self.PassWord) - 1);
                self.PassWord[sizeof(self.PassWord) - 1] = '\0';
            })
        .def_property("Operation",
            [](const Message::TLoginRequest& self) {
                return std::string(self.Operation);  // 返回 bytes 类型
            },
            [](Message::TLoginRequest& self, const std::string& value) {
                std::strncpy(self.Operation, value.c_str(), sizeof(self.Operation) - 1);
                self.Operation[sizeof(self.Operation) - 1] = '\0';
            })
        .def_property("Role",
            [](const Message::TLoginRequest& self) {
                return std::string(self.Role);  // 返回 bytes 类型
            },
            [](Message::TLoginRequest& self, const std::string& value) {
                std::strncpy(self.Role, value.c_str(), sizeof(self.Role) - 1);
                self.Role[sizeof(self.Role) - 1] = '\0';
            })
        .def_property("Plugins",
            [](const Message::TLoginRequest& self) {
                return std::string(self.Plugins);  // 返回 bytes 类型
            },
            [](Message::TLoginRequest& self, const std::string& value) {
                std::strncpy(self.Plugins, value.c_str(), sizeof(self.Plugins) - 1);
                self.Plugins[sizeof(self.Plugins) - 1] = '\0';
            })
        .def_property("Messages",
            [](const Message::TLoginRequest& self) {
                return std::string(self.Messages);  // 返回 bytes 类型
            },
            [](Message::TLoginRequest& self, const std::string& value) {
                std::strncpy(self.Messages, value.c_str(), sizeof(self.Messages) - 1);
                self.Messages[sizeof(self.Messages) - 1] = '\0';
            })
        .def_property("UUID",
            [](const Message::TLoginRequest& self) {
                return std::string(self.UUID);  // 返回 bytes 类型
            },
            [](Message::TLoginRequest& self, const std::string& value) {
                std::strncpy(self.UUID, value.c_str(), sizeof(self.UUID) - 1);
                self.UUID[sizeof(self.UUID) - 1] = '\0';
            });

    // EEventLogLevel enum binding
    py::enum_<Message::EEventLogLevel>(m, "EEventLogLevel")
        .value("EINFO", Message::EEventLogLevel::EINFO)
        .value("EWARNING", Message::EEventLogLevel::EWARNING)
        .value("EERROR", Message::EEventLogLevel::EERROR)
        .export_values(); 

    py::class_<Message::TEventLog>(m, "TEventLog")
        .def(py::init<>())
        .def_property("Colo",
            [](const Message::TEventLog& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Broker",
            [](const Message::TEventLog& self) {
                return std::string(self.Broker);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.Broker, value.c_str(), sizeof(self.Broker) - 1);
                self.Broker[sizeof(self.Broker) - 1] = '\0';
            })
        .def_property("Product",
            [](const Message::TEventLog& self) {
                return std::string(self.Product);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.Product, value.c_str(), sizeof(self.Product) - 1);
                self.Product[sizeof(self.Product) - 1] = '\0';
            })
        .def_property("Account",
            [](const Message::TEventLog& self) {
                return std::string(self.Account);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.Account, value.c_str(), sizeof(self.Account) - 1);
                self.Account[sizeof(self.Account) - 1] = '\0';
            })
        .def_property("Ticker",
            [](const Message::TEventLog& self) {
                return std::string(self.Ticker);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.Ticker, value.c_str(), sizeof(self.Ticker) - 1);
                self.Ticker[sizeof(self.Ticker) - 1] = '\0';
            })
        .def_property("ExchangeID",
            [](const Message::TEventLog& self) {
                return std::string(self.ExchangeID);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.ExchangeID, value.c_str(), sizeof(self.ExchangeID) - 1);
                self.ExchangeID[sizeof(self.ExchangeID) - 1] = '\0';
            })
        .def_property("App",
            [](const Message::TEventLog& self) {
                return std::string(self.App);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.App, value.c_str(), sizeof(self.App) - 1);
                self.App[sizeof(self.App) - 1] = '\0';
            })
        .def_property("Event",
            [](const Message::TEventLog& self) {
                return std::string(self.Event);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.Event, value.c_str(), sizeof(self.Event) - 1);
                self.Event[sizeof(self.Event) - 1] = '\0';
            })
        .def_readwrite("Level", &Message::TEventLog::Level)
        .def_property("UpdateTime",
            [](const Message::TEventLog& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](Message::TEventLog& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            });

    // EBusinessType enum binding
    py::enum_<Message::EBusinessType>(m, "EBusinessType")
        .value("ESTOCK", Message::EBusinessType::ESTOCK)
        .value("ECREDIT", Message::EBusinessType::ECREDIT)
        .value("EFUTURE", Message::EBusinessType::EFUTURE)
        .value("ESPOT", Message::EBusinessType::ESPOT)        
        .export_values(); 

    // EOrderType enum binding
    py::enum_<Message::EOrderType>(m, "EOrderType")
        .value("EFAK", Message::EOrderType::EFAK)
        .value("EFOK", Message::EOrderType::EFOK)
        .value("ELIMIT", Message::EOrderType::ELIMIT)
        .value("EMARKET", Message::EOrderType::EMARKET)
        .export_values(); 

    // EOrderDirection enum binding
    py::enum_<Message::EOrderDirection>(m, "EOrderDirection")
        .value("EBUY", Message::EOrderDirection::EBUY)
        .value("ESELL", Message::EOrderDirection::ESELL)
        .value("EREVERSE_REPO", Message::EOrderDirection::EREVERSE_REPO)
        .value("ESUBSCRIPTION", Message::EOrderDirection::ESUBSCRIPTION)
        .value("EALLOTMENT", Message::EOrderDirection::EALLOTMENT)
        .value("ECOLLATERAL_TRANSFER_IN", Message::EOrderDirection::ECOLLATERAL_TRANSFER_IN)
        .value("ECOLLATERAL_TRANSFER_OUT", Message::EOrderDirection::ECOLLATERAL_TRANSFER_OUT)
        .value("EMARGIN_BUY", Message::EOrderDirection::EMARGIN_BUY)
        .value("EREPAY_MARGIN_BY_SELL", Message::EOrderDirection::EREPAY_MARGIN_BY_SELL)
        .value("ESHORT_SELL", Message::EOrderDirection::ESHORT_SELL)
        .value("EREPAY_STOCK_BY_BUY", Message::EOrderDirection::EREPAY_STOCK_BY_BUY)
        .value("EREPAY_STOCK_DIRECT", Message::EOrderDirection::EREPAY_STOCK_DIRECT)
        .export_values(); 

    // EOrderOffset enum binding
    py::enum_<Message::EOrderOffset>(m, "EOrderOffset")
        .value("EOPEN", Message::EOrderOffset::EOPEN)
        .value("ECLOSE", Message::EOrderOffset::ECLOSE)
        .value("ECLOSE_TODAY", Message::EOrderOffset::ECLOSE_TODAY)
        .value("ECLOSE_YESTODAY", Message::EOrderOffset::ECLOSE_YESTODAY)
        .export_values(); 

    // ERiskStatusType enum binding
    py::enum_<Message::ERiskStatusType>(m, "ERiskStatusType")
        .value("EPREPARE_CHECKED", Message::ERiskStatusType::EPREPARE_CHECKED)
        .value("ECHECKED_PASS", Message::ERiskStatusType::ECHECKED_PASS)
        .value("ECHECKED_NOPASS", Message::ERiskStatusType::ECHECKED_NOPASS)
        .value("ENOCHECKED", Message::ERiskStatusType::ENOCHECKED)
        .value("ECHECK_INIT", Message::ERiskStatusType::ECHECK_INIT)
        .export_values(); 


    py::class_<Message::TOrderRequest>(m, "TOrderRequest")
        .def(py::init<>())
        .def_property("Colo",
            [](const Message::TOrderRequest& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Broker",
            [](const Message::TOrderRequest& self) {
                return std::string(self.Broker);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.Broker, value.c_str(), sizeof(self.Broker) - 1);
                self.Broker[sizeof(self.Broker) - 1] = '\0';
            })
        .def_property("Product",
            [](const Message::TOrderRequest& self) {
                return std::string(self.Product);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.Product, value.c_str(), sizeof(self.Product) - 1);
                self.Product[sizeof(self.Product) - 1] = '\0';
            })
        .def_property("Account",
            [](const Message::TOrderRequest& self) {
                return std::string(self.Account);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.Account, value.c_str(), sizeof(self.Account) - 1);
                self.Account[sizeof(self.Account) - 1] = '\0';
            })
        .def_property("Ticker",
            [](const Message::TOrderRequest& self) {
                return std::string(self.Ticker);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.Ticker, value.c_str(), sizeof(self.Ticker) - 1);
                self.Ticker[sizeof(self.Ticker) - 1] = '\0';
            })
        .def_property("ExchangeID",
            [](const Message::TOrderRequest& self) {
                return std::string(self.ExchangeID);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.ExchangeID, value.c_str(), sizeof(self.ExchangeID) - 1);
                self.ExchangeID[sizeof(self.ExchangeID) - 1] = '\0';
            })
        .def_readwrite("BusinessType", &Message::TOrderRequest::BusinessType)
        .def_readwrite("OrderType", &Message::TOrderRequest::OrderType)
        .def_readwrite("Direction", &Message::TOrderRequest::Direction)
        .def_readwrite("Offset", &Message::TOrderRequest::Offset)
        .def_readwrite("RiskStatus", &Message::TOrderRequest::RiskStatus)
        .def_readwrite("OrderToken", &Message::TOrderRequest::OrderToken)
        .def_readwrite("EngineID", &Message::TOrderRequest::EngineID)
        .def_readwrite("UserReserved1", &Message::TOrderRequest::UserReserved1)
        .def_readwrite("UserReserved2", &Message::TOrderRequest::UserReserved2)
        .def_readwrite("Price", &Message::TOrderRequest::Price)
        .def_readwrite("Volume", &Message::TOrderRequest::Volume)
        .def_property("RecvMarketTime",
            [](const Message::TOrderRequest& self) {
                return std::string(self.RecvMarketTime);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.RecvMarketTime, value.c_str(), sizeof(self.RecvMarketTime) - 1);
                self.RecvMarketTime[sizeof(self.RecvMarketTime) - 1] = '\0';
            })
        .def_property("SendTime",
            [](const Message::TOrderRequest& self) {
                return std::string(self.SendTime);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.SendTime, value.c_str(), sizeof(self.SendTime) - 1);
                self.SendTime[sizeof(self.SendTime) - 1] = '\0';
            })
        .def_property("RiskID",
            [](const Message::TOrderRequest& self) {
                return std::string(self.RiskID);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.RiskID, value.c_str(), sizeof(self.RiskID) - 1);
                self.RiskID[sizeof(self.RiskID) - 1] = '\0';
            })
        .def_property("Trader",
            [](const Message::TOrderRequest& self) {
                return std::string(self.Trader);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.Trader, value.c_str(), sizeof(self.Trader) - 1);
                self.Trader[sizeof(self.Trader) - 1] = '\0';
            })
        .def_readwrite("ErrorID", &Message::TOrderRequest::ErrorID)
        .def_property("ErrorMsg",
            [](const Message::TOrderRequest& self) {
                return std::string(self.ErrorMsg);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.ErrorMsg, value.c_str(), sizeof(self.ErrorMsg) - 1);
                self.ErrorMsg[sizeof(self.ErrorMsg) - 1] = '\0';
            })
        .def_property("UpdateTime",
            [](const Message::TOrderRequest& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](Message::TOrderRequest& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            });

    py::class_<Message::TActionRequest>(m, "TActionRequest")
        .def(py::init<>())
        .def_property("Colo",
            [](const Message::TActionRequest& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](Message::TActionRequest& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Account",
            [](const Message::TActionRequest& self) {
                return std::string(self.Account);  // 返回 bytes 类型
            },
            [](Message::TActionRequest& self, const std::string& value) {
                std::strncpy(self.Account, value.c_str(), sizeof(self.Account) - 1);
                self.Account[sizeof(self.Account) - 1] = '\0';
            })
        .def_property("OrderRef",
            [](const Message::TActionRequest& self) {
                return std::string(self.OrderRef);  // 返回 bytes 类型
            },
            [](Message::TActionRequest& self, const std::string& value) {
                std::strncpy(self.OrderRef, value.c_str(), sizeof(self.OrderRef) - 1);
                self.OrderRef[sizeof(self.OrderRef) - 1] = '\0';
            })
        .def_property("ExchangeID",
            [](const Message::TActionRequest& self) {
                return std::string(self.ExchangeID);  // 返回 bytes 类型
            },
            [](Message::TActionRequest& self, const std::string& value) {
                std::strncpy(self.ExchangeID, value.c_str(), sizeof(self.ExchangeID) - 1);
                self.ExchangeID[sizeof(self.ExchangeID) - 1] = '\0';
            })
        .def_readwrite("BusinessType", &Message::TActionRequest::BusinessType)
        .def_readwrite("EngineID", &Message::TActionRequest::EngineID)
        .def_readwrite("RiskStatus", &Message::TActionRequest::RiskStatus)
        .def_property("Trader",
            [](const Message::TActionRequest& self) {
                return std::string(self.Trader);  // 返回 bytes 类型
            },
            [](Message::TActionRequest& self, const std::string& value) {
                std::strncpy(self.Trader, value.c_str(), sizeof(self.Trader) - 1);
                self.Trader[sizeof(self.Trader) - 1] = '\0';
            })
        .def_property("RiskID",
            [](const Message::TActionRequest& self) {
                return std::string(self.RiskID);  // 返回 bytes 类型
            },
            [](Message::TActionRequest& self, const std::string& value) {
                std::strncpy(self.RiskID, value.c_str(), sizeof(self.RiskID) - 1);
                self.RiskID[sizeof(self.RiskID) - 1] = '\0';
            })
        .def_readwrite("ErrorID", &Message::TActionRequest::ErrorID)
        .def_property("ErrorMsg",
            [](const Message::TActionRequest& self) {
                return std::string(self.ErrorMsg);  // 返回 bytes 类型
            },
            [](Message::TActionRequest& self, const std::string& value) {
                std::strncpy(self.ErrorMsg, value.c_str(), sizeof(self.ErrorMsg) - 1);
                self.ErrorMsg[sizeof(self.ErrorMsg) - 1] = '\0';
            })
        .def_property("UpdateTime",
            [](const Message::TActionRequest& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](Message::TActionRequest& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            });

    // EOrderStatus enum binding
    py::enum_<Message::EOrderStatusType>(m, "EOrderStatusType")
        .value("EORDER_SENDED", Message::EOrderStatusType::EORDER_SENDED)
        .value("EBROKER_ACK", Message::EOrderStatusType::EBROKER_ACK)
        .value("EEXCHANGE_ACK", Message::EOrderStatusType::EEXCHANGE_ACK)
        .value("EPARTTRADED", Message::EOrderStatusType::EPARTTRADED)
        .value("EALLTRADED", Message::EOrderStatusType::EALLTRADED)
        .value("ECANCELLING", Message::EOrderStatusType::ECANCELLING)
        .value("ECANCELLED", Message::EOrderStatusType::ECANCELLED)
        .value("EPARTTRADED_CANCELLED", Message::EOrderStatusType::EPARTTRADED_CANCELLED)
        .value("EBROKER_ERROR", Message::EOrderStatusType::EBROKER_ERROR)
        .value("EEXCHANGE_ERROR", Message::EOrderStatusType::EEXCHANGE_ERROR)
        .value("EACTION_ERROR", Message::EOrderStatusType::EACTION_ERROR)
        .value("ERISK_ORDER_REJECTED", Message::EOrderStatusType::ERISK_ORDER_REJECTED)
        .value("ERISK_ACTION_REJECTED", Message::EOrderStatusType::ERISK_ACTION_REJECTED)
        .value("ERISK_CHECK_INIT", Message::EOrderStatusType::ERISK_CHECK_INIT)
        .value("ERISK_CHECK_SELFMATCH", Message::EOrderStatusType::ERISK_CHECK_SELFMATCH)
        .value("ERISK_CHECK_CANCELLIMIT", Message::EOrderStatusType::ERISK_CHECK_CANCELLIMIT)
        .export_values(); 

    // EOrderSide enum binding
    py::enum_<Message::EOrderSide>(m, "EOrderSide")
        .value("EOPEN_LONG", Message::EOrderSide::EOPEN_LONG)
        .value("ECLOSE_TD_LONG", Message::EOrderSide::ECLOSE_TD_LONG)
        .value("ECLOSE_YD_LONG", Message::EOrderSide::ECLOSE_YD_LONG)
        .value("EOPEN_SHORT", Message::EOrderSide::EOPEN_SHORT)
        .value("ECLOSE_TD_SHORT", Message::EOrderSide::ECLOSE_TD_SHORT)
        .value("ECLOSE_YD_SHORT", Message::EOrderSide::ECLOSE_YD_SHORT)
        .value("ECLOSE_LONG", Message::EOrderSide::ECLOSE_LONG)
        .value("ECLOSE_SHORT", Message::EOrderSide::ECLOSE_SHORT)
        .value("ESIDE_REVERSE_REPO", Message::EOrderSide::ESIDE_REVERSE_REPO)
        .value("ESIDE_SUBSCRIPTION", Message::EOrderSide::ESIDE_SUBSCRIPTION)
        .value("ESIDE_ALLOTMENT", Message::EOrderSide::ESIDE_ALLOTMENT)
        .value("ESIDE_COLLATERAL_BUY", Message::EOrderSide::ESIDE_COLLATERAL_BUY)
        .value("ESIDE_COLLATERAL_SELL", Message::EOrderSide::ESIDE_COLLATERAL_SELL)
        .value("ESIDE_MARGIN_BUY", Message::EOrderSide::ESIDE_MARGIN_BUY)
        .value("ESIDE_REPAY_MARGIN_BY_SELL", Message::EOrderSide::ESIDE_REPAY_MARGIN_BY_SELL)
        .value("ESIDE_SHORT_SELL", Message::EOrderSide::ESIDE_SHORT_SELL)
        .value("ESIDE_REPAY_STOCK_BY_BUY", Message::EOrderSide::ESIDE_REPAY_STOCK_BY_BUY)
        .value("ESIDE_REPAY_STOCK_DIRECT", Message::EOrderSide::ESIDE_REPAY_STOCK_DIRECT)
        .export_values(); 

    // TOrderStatus type bindings
    py::class_<Message::TOrderStatus>(m, "TOrderStatus")
        .def(py::init<>())
        .def_property("Colo",
            [](const Message::TOrderStatus& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Broker",
            [](const Message::TOrderStatus& self) {
                return std::string(self.Broker);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.Broker, value.c_str(), sizeof(self.Broker) - 1);
                self.Broker[sizeof(self.Broker) - 1] = '\0';
            })
        .def_property("Product",
            [](const Message::TOrderStatus& self) {
                return std::string(self.Product);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.Product, value.c_str(), sizeof(self.Product) - 1);
                self.Product[sizeof(self.Product) - 1] = '\0';
            })
        .def_property("Account",
            [](const Message::TOrderStatus& self) {
                return std::string(self.Account);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.Account, value.c_str(), sizeof(self.Account) - 1);
                self.Account[sizeof(self.Account) - 1] = '\0';
            })
        .def_property("Ticker",
            [](const Message::TOrderStatus& self) {
                return std::string(self.Ticker);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.Ticker, value.c_str(), sizeof(self.Ticker) - 1);
                self.Ticker[sizeof(self.Ticker) - 1] = '\0';
            })
        .def_property("ExchangeID",
            [](const Message::TOrderStatus& self) {
                return std::string(self.ExchangeID);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.ExchangeID, value.c_str(), sizeof(self.ExchangeID) - 1);
                self.ExchangeID[sizeof(self.ExchangeID) - 1] = '\0';
            })
        .def_readwrite("BusinessType", &Message::TOrderStatus::BusinessType)
        .def_property("OrderRef",
            [](const Message::TOrderStatus& self) {
                return std::string(self.OrderRef);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.OrderRef, value.c_str(), sizeof(self.OrderRef) - 1);
                self.OrderRef[sizeof(self.OrderRef) - 1] = '\0';
            })
        .def_property("OrderSysID",
            [](const Message::TOrderStatus& self) {
                return std::string(self.OrderSysID);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.OrderSysID, value.c_str(), sizeof(self.OrderSysID) - 1);
                self.OrderSysID[sizeof(self.OrderSysID) - 1] = '\0';
            })
        .def_property("OrderLocalID",
            [](const Message::TOrderStatus& self) {
                return std::string(self.OrderLocalID);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.OrderLocalID, value.c_str(), sizeof(self.OrderLocalID) - 1);
                self.OrderLocalID[sizeof(self.OrderLocalID) - 1] = '\0';
            })
        .def_readwrite("OrderToken", &Message::TOrderStatus::OrderToken)
        .def_readwrite("EngineID", &Message::TOrderStatus::EngineID)
        .def_readwrite("UserReserved1", &Message::TOrderStatus::UserReserved1)
        .def_readwrite("UserReserved2", &Message::TOrderStatus::UserReserved2)
        .def_readwrite("OrderType", &Message::TOrderStatus::OrderType)
        .def_readwrite("OrderSide", &Message::TOrderStatus::OrderSide)
        .def_readwrite("OrderStatus", &Message::TOrderStatus::OrderStatus)
        .def_readwrite("SendPrice", &Message::TOrderStatus::SendPrice)
        .def_readwrite("SendVolume", &Message::TOrderStatus::SendVolume)
        .def_readwrite("TotalTradedVolume", &Message::TOrderStatus::TotalTradedVolume)
        .def_readwrite("TradedAvgPrice", &Message::TOrderStatus::TradedAvgPrice)
        .def_readwrite("TradedVolume", &Message::TOrderStatus::TradedVolume)
        .def_readwrite("TradedPrice", &Message::TOrderStatus::TradedPrice)
        .def_readwrite("CanceledVolume", &Message::TOrderStatus::CanceledVolume)
        .def_readwrite("Commission", &Message::TOrderStatus::Commission)
        .def_property("RecvMarketTime",
            [](const Message::TOrderStatus& self) {
                return std::string(self.RecvMarketTime);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.RecvMarketTime, value.c_str(), sizeof(self.RecvMarketTime) - 1);
                self.RecvMarketTime[sizeof(self.RecvMarketTime) - 1] = '\0';
            })
        .def_property("SendTime",
            [](const Message::TOrderStatus& self) {
                return std::string(self.SendTime);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.SendTime, value.c_str(), sizeof(self.SendTime) - 1);
                self.SendTime[sizeof(self.SendTime) - 1] = '\0';
            })
        .def_property("InsertTime",
            [](const Message::TOrderStatus& self) {
                return std::string(self.InsertTime);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.InsertTime, value.c_str(), sizeof(self.InsertTime) - 1);
                self.InsertTime[sizeof(self.InsertTime) - 1] = '\0';
            })
        .def_property("BrokerACKTime",
            [](const Message::TOrderStatus& self) {
                return std::string(self.BrokerACKTime);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.BrokerACKTime, value.c_str(), sizeof(self.BrokerACKTime) - 1);
                self.BrokerACKTime[sizeof(self.BrokerACKTime) - 1] = '\0';
            })
        .def_property("ExchangeACKTime",
            [](const Message::TOrderStatus& self) {
                return std::string(self.ExchangeACKTime);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.ExchangeACKTime, value.c_str(), sizeof(self.ExchangeACKTime) - 1);
                self.ExchangeACKTime[sizeof(self.ExchangeACKTime) - 1] = '\0';
            })
        .def_property("RiskID",
            [](const Message::TOrderStatus& self) {
                return std::string(self.RiskID);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.RiskID, value.c_str(), sizeof(self.RiskID) - 1);
                self.RiskID[sizeof(self.RiskID) - 1] = '\0';
            })
        .def_property("Trader",
            [](const Message::TOrderStatus& self) {
                return std::string(self.Trader);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.Trader, value.c_str(), sizeof(self.Trader) - 1);
                self.Trader[sizeof(self.Trader) - 1] = '\0';
            })
        .def_readwrite("ErrorID", &Message::TOrderStatus::ErrorID)
        .def_property("ErrorMsg",
            [](const Message::TOrderStatus& self) {
                return std::string(self.ErrorMsg);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.ErrorMsg, value.c_str(), sizeof(self.ErrorMsg) - 1);
                self.ErrorMsg[sizeof(self.ErrorMsg) - 1] = '\0';
            })
        .def_property("UpdateTime",
            [](const Message::TOrderStatus& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](Message::TOrderStatus& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            });

    py::class_<Message::TAccountFund>(m, "TAccountFund")
        .def(py::init<>())
        .def_property("Colo",
            [](const Message::TAccountFund& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](Message::TAccountFund& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Broker",
            [](const Message::TAccountFund& self) {
                return std::string(self.Broker);  // 返回 bytes 类型
            },
            [](Message::TAccountFund& self, const std::string& value) {
                std::strncpy(self.Broker, value.c_str(), sizeof(self.Broker) - 1);
                self.Broker[sizeof(self.Broker) - 1] = '\0';
            })
        .def_property("Product",
            [](const Message::TAccountFund& self) {
                return std::string(self.Product);  // 返回 bytes 类型
            },
            [](Message::TAccountFund& self, const std::string& value) {
                std::strncpy(self.Product, value.c_str(), sizeof(self.Product) - 1);
                self.Product[sizeof(self.Product) - 1] = '\0';
            })
        .def_property("Account",
            [](const Message::TAccountFund& self) {
                return std::string(self.Account);  // 返回 bytes 类型
            },
            [](Message::TAccountFund& self, const std::string& value) {
                std::strncpy(self.Account, value.c_str(), sizeof(self.Account) - 1);
                self.Account[sizeof(self.Account) - 1] = '\0';
            })
        .def_readwrite("BusinessType", &Message::TAccountFund::BusinessType)
        .def_readwrite("Deposit", &Message::TAccountFund::Deposit)
        .def_readwrite("Withdraw", &Message::TAccountFund::Withdraw)
        .def_readwrite("CurrMargin", &Message::TAccountFund::CurrMargin)
        .def_readwrite("Commission", &Message::TAccountFund::Commission)
        .def_readwrite("CloseProfit", &Message::TAccountFund::CloseProfit)
        .def_readwrite("PositionProfit", &Message::TAccountFund::PositionProfit)
        .def_readwrite("Available", &Message::TAccountFund::Available)
        .def_readwrite("WithdrawQuota", &Message::TAccountFund::WithdrawQuota)
        .def_readwrite("ExchangeMargin", &Message::TAccountFund::ExchangeMargin)
        .def_readwrite("Balance", &Message::TAccountFund::Balance)
        .def_readwrite("PreBalance", &Message::TAccountFund::PreBalance)
        .def_property("UpdateTime",
            [](const Message::TAccountFund& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](Message::TAccountFund& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            });

    py::class_<Message::TFuturePosition>(m, "TFuturePosition")
        .def(py::init<>())
        .def_readwrite("LongTdVolume", &Message::TFuturePosition::LongTdVolume)
        .def_readwrite("LongYdVolume", &Message::TFuturePosition::LongYdVolume)
        .def_readwrite("LongOpenVolume", &Message::TFuturePosition::LongOpenVolume)
        .def_readwrite("LongOpeningVolume", &Message::TFuturePosition::LongOpeningVolume)
        .def_readwrite("LongClosingTdVolume", &Message::TFuturePosition::LongClosingTdVolume)
        .def_readwrite("LongClosingYdVolume", &Message::TFuturePosition::LongClosingYdVolume)
        .def_readwrite("ShortTdVolume", &Message::TFuturePosition::ShortTdVolume)
        .def_readwrite("ShortYdVolume", &Message::TFuturePosition::ShortYdVolume)
        .def_readwrite("ShortOpenVolume", &Message::TFuturePosition::ShortOpenVolume)
        .def_readwrite("ShortOpeningVolume", &Message::TFuturePosition::ShortOpeningVolume)
        .def_readwrite("ShortClosingTdVolume", &Message::TFuturePosition::ShortClosingTdVolume)
        .def_readwrite("ShortClosingYdVolume", &Message::TFuturePosition::ShortClosingYdVolume);

    py::class_<Message::TStockPosition>(m, "TStockPosition")
        .def(py::init<>())
        .def_readwrite("LongYdPosition", &Message::TStockPosition::LongYdPosition)
        .def_readwrite("LongPosition", &Message::TStockPosition::LongPosition)
        .def_readwrite("LongTdBuy", &Message::TStockPosition::LongTdBuy)
        .def_readwrite("LongTdSell", &Message::TStockPosition::LongTdSell)
        .def_readwrite("MarginYdPosition", &Message::TStockPosition::MarginYdPosition)
        .def_readwrite("MarginPosition", &Message::TStockPosition::MarginPosition)
        .def_readwrite("MarginTdBuy", &Message::TStockPosition::MarginTdBuy)
        .def_readwrite("MarginTdSell", &Message::TStockPosition::MarginTdSell)
        .def_readwrite("ShortYdPosition", &Message::TStockPosition::ShortYdPosition)
        .def_readwrite("ShortPosition", &Message::TStockPosition::ShortPosition)
        .def_readwrite("ShortTdSell", &Message::TStockPosition::ShortTdSell)
        .def_readwrite("ShortTdBuy", &Message::TStockPosition::ShortTdBuy)
        .def_readwrite("ShortDirectRepaid", &Message::TStockPosition::ShortDirectRepaid)
        .def_readwrite("SpecialPositionAvl", &Message::TStockPosition::SpecialPositionAvl);

    py::class_<Message::TAccountPosition>(m, "TAccountPosition")
        .def(py::init<>())
        .def_property("Colo",
            [](const Message::TAccountPosition& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](Message::TAccountPosition& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Broker",
            [](const Message::TAccountPosition& self) {
                return std::string(self.Broker);  // 返回 bytes 类型
            },
            [](Message::TAccountPosition& self, const std::string& value) {
                std::strncpy(self.Broker, value.c_str(), sizeof(self.Broker) - 1);
                self.Broker[sizeof(self.Broker) - 1] = '\0';
            })
        .def_property("Product",
            [](const Message::TAccountPosition& self) {
                return std::string(self.Product);  // 返回 bytes 类型
            },
            [](Message::TAccountPosition& self, const std::string& value) {
                std::strncpy(self.Product, value.c_str(), sizeof(self.Product) - 1);
                self.Product[sizeof(self.Product) - 1] = '\0';
            })
        .def_property("Account",
            [](const Message::TAccountPosition& self) {
                return std::string(self.Account);  // 返回 bytes 类型
            },
            [](Message::TAccountPosition& self, const std::string& value) {
                std::strncpy(self.Account, value.c_str(), sizeof(self.Account) - 1);
                self.Account[sizeof(self.Account) - 1] = '\0';
            })
        .def_property("Ticker",
            [](const Message::TAccountPosition& self) {
                return std::string(self.Ticker);  // 返回 bytes 类型
            },
            [](Message::TAccountPosition& self, const std::string& value) {
                std::strncpy(self.Ticker, value.c_str(), sizeof(self.Ticker) - 1);
                self.Ticker[sizeof(self.Ticker) - 1] = '\0';
            })
        .def_property("ExchangeID",
            [](const Message::TAccountPosition& self) {
                return std::string(self.ExchangeID);  // 返回 bytes 类型
            },
            [](Message::TAccountPosition& self, const std::string& value) {
                std::strncpy(self.ExchangeID, value.c_str(), sizeof(self.ExchangeID) - 1);
                self.ExchangeID[sizeof(self.ExchangeID) - 1] = '\0';
            })
        .def_readwrite("BusinessType", &Message::TAccountPosition::BusinessType)
        .def_readwrite("FuturePosition", &Message::TAccountPosition::FuturePosition)
        .def_readwrite("StockPosition", &Message::TAccountPosition::StockPosition)
        .def_property("UpdateTime",
            [](const Message::TAccountPosition& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](Message::TAccountPosition& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            });

    py::class_<MarketData::TFutureMarketData>(m, "TFutureMarketData")
        .def(py::init<>())
        .def_property("Colo",
            [](const MarketData::TFutureMarketData& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](MarketData::TFutureMarketData& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Broker",
            [](const MarketData::TFutureMarketData& self) {
                return std::string(self.Broker);  // 返回 bytes 类型
            },
            [](MarketData::TFutureMarketData& self, const std::string& value) {
                std::strncpy(self.Broker, value.c_str(), sizeof(self.Broker) - 1);
                self.Broker[sizeof(self.Broker) - 1] = '\0';
            })
        .def_property("Ticker",
            [](const MarketData::TFutureMarketData& self) {
                return std::string(self.Ticker);  // 返回 bytes 类型
            },
            [](MarketData::TFutureMarketData& self, const std::string& value) {
                std::strncpy(self.Ticker, value.c_str(), sizeof(self.Ticker) - 1);
                self.Ticker[sizeof(self.Ticker) - 1] = '\0';
            })
        .def_property("ExchangeID",
            [](const MarketData::TFutureMarketData& self) {
                return std::string(self.ExchangeID);  // 返回 bytes 类型
            },
            [](MarketData::TFutureMarketData& self, const std::string& value) {
                std::strncpy(self.ExchangeID, value.c_str(), sizeof(self.ExchangeID) - 1);
                self.ExchangeID[sizeof(self.ExchangeID) - 1] = '\0';
            })
        .def_readwrite("LastTick", &MarketData::TFutureMarketData::LastTick)
        .def_readwrite("Tick", &MarketData::TFutureMarketData::Tick)
        .def_property("TradingDay",
            [](const MarketData::TFutureMarketData& self) {
                return std::string(self.TradingDay);  // 返回 bytes 类型
            },
            [](MarketData::TFutureMarketData& self, const std::string& value) {
                std::strncpy(self.TradingDay, value.c_str(), sizeof(self.TradingDay) - 1);
                self.TradingDay[sizeof(self.TradingDay) - 1] = '\0';
            })
        .def_property("ActionDay",
            [](const MarketData::TFutureMarketData& self) {
                return std::string(self.ActionDay);  // 返回 bytes 类型
            },
            [](MarketData::TFutureMarketData& self, const std::string& value) {
                std::strncpy(self.ActionDay, value.c_str(), sizeof(self.ActionDay) - 1);
                self.ActionDay[sizeof(self.ActionDay) - 1] = '\0';
            })
        .def_property("UpdateTime",
            [](const MarketData::TFutureMarketData& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](MarketData::TFutureMarketData& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            })
        .def_readwrite("MillSec", &MarketData::TFutureMarketData::MillSec)
        .def_readwrite("LastPrice", &MarketData::TFutureMarketData::LastPrice)
        .def_readwrite("Volume", &MarketData::TFutureMarketData::Volume)
        .def_readwrite("Turnover", &MarketData::TFutureMarketData::Turnover)
        .def_readwrite("OpenPrice", &MarketData::TFutureMarketData::OpenPrice)
        .def_readwrite("ClosePrice", &MarketData::TFutureMarketData::ClosePrice)
        .def_readwrite("PreClosePrice", &MarketData::TFutureMarketData::PreClosePrice)
        .def_readwrite("SettlementPrice", &MarketData::TFutureMarketData::SettlementPrice)
        .def_readwrite("PreSettlementPrice", &MarketData::TFutureMarketData::PreSettlementPrice)
        .def_readwrite("OpenInterest", &MarketData::TFutureMarketData::OpenInterest)
        .def_readwrite("PreOpenInterest", &MarketData::TFutureMarketData::PreOpenInterest)
        .def_readwrite("PreDelta", &MarketData::TFutureMarketData::PreDelta)
        .def_readwrite("CurrDelta", &MarketData::TFutureMarketData::CurrDelta)
        .def_readwrite("HighestPrice", &MarketData::TFutureMarketData::HighestPrice)
        .def_readwrite("LowestPrice", &MarketData::TFutureMarketData::LowestPrice)
        .def_readwrite("UpperLimitPrice", &MarketData::TFutureMarketData::UpperLimitPrice)
        .def_readwrite("LowerLimitPrice", &MarketData::TFutureMarketData::LowerLimitPrice)
        .def_readwrite("AveragePrice", &MarketData::TFutureMarketData::AveragePrice)
        .def_readwrite("BidPrice1", &MarketData::TFutureMarketData::BidPrice1)
        .def_readwrite("BidVolume1", &MarketData::TFutureMarketData::BidVolume1)
        .def_readwrite("AskPrice1", &MarketData::TFutureMarketData::AskPrice1)
        .def_readwrite("AskVolume1", &MarketData::TFutureMarketData::AskVolume1)
        .def_readwrite("BidPrice2", &MarketData::TFutureMarketData::BidPrice2)
        .def_readwrite("BidVolume2", &MarketData::TFutureMarketData::BidVolume2)
        .def_readwrite("AskPrice2", &MarketData::TFutureMarketData::AskPrice2)
        .def_readwrite("AskVolume2", &MarketData::TFutureMarketData::AskVolume2)
        .def_readwrite("BidPrice3", &MarketData::TFutureMarketData::BidPrice3)
        .def_readwrite("BidVolume3", &MarketData::TFutureMarketData::BidVolume3)
        .def_readwrite("AskPrice3", &MarketData::TFutureMarketData::AskPrice3)
        .def_readwrite("AskVolume3", &MarketData::TFutureMarketData::AskVolume3)
        .def_readwrite("BidPrice4", &MarketData::TFutureMarketData::BidPrice4)
        .def_readwrite("BidVolume4", &MarketData::TFutureMarketData::BidVolume4)
        .def_readwrite("AskPrice4", &MarketData::TFutureMarketData::AskPrice4)
        .def_readwrite("AskVolume4", &MarketData::TFutureMarketData::AskVolume4)
        .def_readwrite("BidPrice5", &MarketData::TFutureMarketData::BidPrice5)
        .def_readwrite("BidVolume5", &MarketData::TFutureMarketData::BidVolume5)
        .def_readwrite("AskPrice5", &MarketData::TFutureMarketData::AskPrice5)
        .def_readwrite("AskVolume5", &MarketData::TFutureMarketData::AskVolume5)
        .def_readwrite("SectionFirstTick", &MarketData::TFutureMarketData::SectionFirstTick)
        .def_readwrite("SectionLastTick", &MarketData::TFutureMarketData::SectionLastTick)
        .def_readwrite("TotalTick", &MarketData::TFutureMarketData::TotalTick)
        .def_readwrite("ErrorID", &MarketData::TFutureMarketData::ErrorID)
        .def_property("RecvLocalTime",
            [](const MarketData::TFutureMarketData& self) {
                return std::string(self.RecvLocalTime);  // 返回 bytes 类型
            },
            [](MarketData::TFutureMarketData& self, const std::string& value) {
                std::strncpy(self.RecvLocalTime, value.c_str(), sizeof(self.RecvLocalTime) - 1);
                self.RecvLocalTime[sizeof(self.RecvLocalTime) - 1] = '\0';
            });

    py::class_<MarketData::TStockMarketData>(m, "TStockMarketData")
        .def(py::init<>())
        .def_property("Colo",
            [](const MarketData::TStockMarketData& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](MarketData::TStockMarketData& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Broker",
            [](const MarketData::TStockMarketData& self) {
                return std::string(self.Broker);  // 返回 bytes 类型
            },
            [](MarketData::TStockMarketData& self, const std::string& value) {
                std::strncpy(self.Broker, value.c_str(), sizeof(self.Broker) - 1);
                self.Broker[sizeof(self.Broker) - 1] = '\0';
            })
        .def_property("Ticker",
            [](const MarketData::TStockMarketData& self) {
                return std::string(self.Ticker);  // 返回 bytes 类型
            },
            [](MarketData::TStockMarketData& self, const std::string& value) {
                std::strncpy(self.Ticker, value.c_str(), sizeof(self.Ticker) - 1);
                self.Ticker[sizeof(self.Ticker) - 1] = '\0';
            })
        .def_property("ExchangeID",
            [](const MarketData::TStockMarketData& self) {
                return std::string(self.ExchangeID);  // 返回 bytes 类型
            },
            [](MarketData::TStockMarketData& self, const std::string& value) {
                std::strncpy(self.ExchangeID, value.c_str(), sizeof(self.ExchangeID) - 1);
                self.ExchangeID[sizeof(self.ExchangeID) - 1] = '\0';
            })
        .def_readwrite("LastTick", &MarketData::TStockMarketData::LastTick)
        .def_readwrite("Tick", &MarketData::TStockMarketData::Tick)
        .def_property("UpdateTime",
            [](const MarketData::TStockMarketData& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](MarketData::TStockMarketData& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            })
        .def_readwrite("MillSec", &MarketData::TStockMarketData::MillSec)
        .def_readwrite("LastPrice", &MarketData::TStockMarketData::LastPrice)
        .def_readwrite("Volume", &MarketData::TStockMarketData::Volume)
        .def_readwrite("Turnover", &MarketData::TStockMarketData::Turnover)
        .def_readwrite("PreSettlementPrice", &MarketData::TStockMarketData::PreSettlementPrice)
        .def_readwrite("PreClosePrice", &MarketData::TStockMarketData::PreClosePrice)
        .def_readwrite("OpenInterest", &MarketData::TStockMarketData::OpenInterest)
        .def_readwrite("OpenPrice", &MarketData::TStockMarketData::OpenPrice)
        .def_readwrite("HighestPrice", &MarketData::TStockMarketData::HighestPrice)
        .def_readwrite("LowestPrice", &MarketData::TStockMarketData::LowestPrice)
        .def_readwrite("UpperLimitPrice", &MarketData::TStockMarketData::UpperLimitPrice)
        .def_readwrite("LowerLimitPrice", &MarketData::TStockMarketData::LowerLimitPrice)
        .def_property("BidPrice", // 将 data 数组暴露为属性
            [](MarketData::TStockMarketData &self) { // Getter: 返回 numpy 数组
                return py::array_t<double>(
                    {10}, // 数组形状
                    {sizeof(double)}, // 每个元素的步长
                    self.BidPrice // 数据指针
                );
            },
            [](MarketData::TStockMarketData &self, py::array_t<double> arr) { // Setter: 从 numpy 数组设置数据
                auto buf = arr.request(); // 获取数组信息
                if (buf.size != 10) { // 检查数组大小
                    throw std::runtime_error("BidPrice array must have size 10");
                }
                double *ptr = static_cast<double *>(buf.ptr); // 获取数据指针
                std::copy(ptr, ptr + 10, self.BidPrice); // 复制数据到 struct
            })
        .def_property("BidVolume", // 将 data 数组暴露为属性
            [](MarketData::TStockMarketData &self) { // Getter: 返回 numpy 数组
                return py::array_t<int>(
                    {10}, // 数组形状
                    {sizeof(int)}, // 每个元素的步长
                    self.BidVolume // 数据指针
                );
            },
            [](MarketData::TStockMarketData &self, py::array_t<int> arr) { // Setter: 从 numpy 数组设置数据
                auto buf = arr.request(); // 获取数组信息
                if (buf.size != 10) { // 检查数组大小
                    throw std::runtime_error("BidVolume array must have size 10");
                }
                int *ptr = static_cast<int *>(buf.ptr); // 获取数据指针
                std::copy(ptr, ptr + 10, self.BidVolume); // 复制数据到 struct
            })
        .def_property("AskPrice", // 将 data 数组暴露为属性
            [](MarketData::TStockMarketData &self) { // Getter: 返回 numpy 数组
                return py::array_t<double>(
                    {10}, // 数组形状
                    {sizeof(double)}, // 每个元素的步长
                    self.AskPrice // 数据指针
                );
            },
            [](MarketData::TStockMarketData &self, py::array_t<double> arr) { // Setter: 从 numpy 数组设置数据
                auto buf = arr.request(); // 获取数组信息
                if (buf.size != 10) { // 检查数组大小
                    throw std::runtime_error("AskPrice array must have size 10");
                }
                double *ptr = static_cast<double *>(buf.ptr); // 获取数据指针
                std::copy(ptr, ptr + 10, self.AskPrice); // 复制数据到 struct
            })
        .def_property("AskVolume", // 将 data 数组暴露为属性
            [](MarketData::TStockMarketData &self) { // Getter: 返回 numpy 数组
                return py::array_t<int>(
                    {10}, // 数组形状
                    {sizeof(int)}, // 每个元素的步长
                    self.AskVolume // 数据指针
                );
            },
            [](MarketData::TStockMarketData &self, py::array_t<int> arr) { // Setter: 从 numpy 数组设置数据
                auto buf = arr.request(); // 获取数组信息
                if (buf.size != 10) { // 检查数组大小
                    throw std::runtime_error("AskVolume array must have size 10");
                }
                int *ptr = static_cast<int *>(buf.ptr); // 获取数据指针
                std::copy(ptr, ptr + 10, self.AskVolume); // 复制数据到 struct
            })
        .def_readwrite("ErrorID", &MarketData::TStockMarketData::ErrorID)
        .def_property("RecvLocalTime",
            [](const MarketData::TStockMarketData& self) {
                return std::string(self.RecvLocalTime);  // 返回 bytes 类型
            },
            [](MarketData::TStockMarketData& self, const std::string& value) {
                std::strncpy(self.RecvLocalTime, value.c_str(), sizeof(self.RecvLocalTime) - 1);
                self.RecvLocalTime[sizeof(self.RecvLocalTime) - 1] = '\0';
            });

    py::class_<Message::TAppStatus>(m, "TAppStatus")
        .def(py::init<>())
        .def_property("Colo",
            [](const Message::TAppStatus& self) {
                return std::string(self.Colo);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.Colo, value.c_str(), sizeof(self.Colo) - 1);
                self.Colo[sizeof(self.Colo) - 1] = '\0';
            })
        .def_property("Account",
            [](const Message::TAppStatus& self) {
                return std::string(self.Account);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.Account, value.c_str(), sizeof(self.Account) - 1);
                self.Account[sizeof(self.Account) - 1] = '\0';
            })
        .def_property("AppName",
            [](const Message::TAppStatus& self) {
                return std::string(self.AppName);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.AppName, value.c_str(), sizeof(self.AppName) - 1);
                self.AppName[sizeof(self.AppName) - 1] = '\0';
            })
        .def_readwrite("PID", &Message::TAppStatus::PID)
        .def_property("Status",
            [](const Message::TAppStatus& self) {
                return std::string(self.Status);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.Status, value.c_str(), sizeof(self.Status) - 1);
                self.Status[sizeof(self.Status) - 1] = '\0';
            })
        .def_readwrite("UsedCPURate", &Message::TAppStatus::UsedCPURate)
        .def_readwrite("UsedMemSize", &Message::TAppStatus::UsedMemSize)
        .def_property("StartTime",
            [](const Message::TAppStatus& self) {
                return std::string(self.StartTime);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.StartTime, value.c_str(), sizeof(self.StartTime) - 1);
                self.StartTime[sizeof(self.StartTime) - 1] = '\0';
            })
        .def_property("LastStartTime",
            [](const Message::TAppStatus& self) {
                return std::string(self.LastStartTime);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.LastStartTime, value.c_str(), sizeof(self.LastStartTime) - 1);
                self.LastStartTime[sizeof(self.LastStartTime) - 1] = '\0';
            })
        .def_property("CommitID",
            [](const Message::TAppStatus& self) {
                return std::string(self.CommitID);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.CommitID, value.c_str(), sizeof(self.CommitID) - 1);
                self.CommitID[sizeof(self.CommitID) - 1] = '\0';
            })
        .def_property("UtilsCommitID",
            [](const Message::TAppStatus& self) {
                return std::string(self.UtilsCommitID);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.UtilsCommitID, value.c_str(), sizeof(self.UtilsCommitID) - 1);
                self.UtilsCommitID[sizeof(self.UtilsCommitID) - 1] = '\0';
            })
        .def_property("APIVersion",
            [](const Message::TAppStatus& self) {
                return std::string(self.APIVersion);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.APIVersion, value.c_str(), sizeof(self.APIVersion) - 1);
                self.APIVersion[sizeof(self.APIVersion) - 1] = '\0';
            })
        .def_property("StartScript",
            [](const Message::TAppStatus& self) {
                return std::string(self.StartScript);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.StartScript, value.c_str(), sizeof(self.StartScript) - 1);
                self.StartScript[sizeof(self.StartScript) - 1] = '\0';
            })
        .def_property("UpdateTime",
            [](const Message::TAppStatus& self) {
                return std::string(self.UpdateTime);  // 返回 bytes 类型
            },
            [](Message::TAppStatus& self, const std::string& value) {
                std::strncpy(self.UpdateTime, value.c_str(), sizeof(self.UpdateTime) - 1);
                self.UpdateTime[sizeof(self.UpdateTime) - 1] = '\0';
            });
    // MessageType enum binding
    py::enum_<Message::EMessageType>(m, "EMessageType")
        .value("ETest", Message::EMessageType::ETest)
        .value("ELoginRequest", Message::EMessageType::ELoginRequest)
        .value("ELoginResponse", Message::EMessageType::ELoginResponse)
        .value("ECommand", Message::EMessageType::ECommand)
        .value("EEventLog", Message::EMessageType::EEventLog)
        .value("EOrderStatus", Message::EMessageType::EOrderStatus)
        .value("EAccountFund", Message::EMessageType::EAccountFund)
        .value("EAccountPosition", Message::EMessageType::EAccountPosition)
        .value("EOrderRequest", Message::EMessageType::EOrderRequest)
        .value("EActionRequest", Message::EMessageType::EActionRequest)
        .value("ERiskReport", Message::EMessageType::ERiskReport)
        .value("EColoStatus", Message::EMessageType::EColoStatus)
        .value("EAppStatus", Message::EMessageType::EAppStatus)
        .value("EFastOrder", Message::EMessageType::EFastOrder)
        .value("EFutureMarketData", Message::EMessageType::EFutureMarketData)
        .value("EStockMarketData", Message::EMessageType::EStockMarketData)
        .value("ESpotMarketData", Message::EMessageType::ESpotMarketData)
        .export_values(); 

    // PackMessage binding
    py::class_<Message::PackMessage>(m, "PackMessage")
        .def(py::init<>())
        .def_readwrite("MessageType", &Message::PackMessage::MessageType)
        .def_readwrite("ChannelID", &Message::PackMessage::ChannelID)
        .def_readwrite("TimeStamp", &Message::PackMessage::TimeStamp)
        .def_readwrite("Test", &Message::PackMessage::Test)
        .def_readwrite("LoginRequest", &Message::PackMessage::LoginRequest)
        .def_readwrite("LoginResponse", &Message::PackMessage::LoginResponse)
        .def_readwrite("Command", &Message::PackMessage::Command)
        .def_readwrite("EventLog", &Message::PackMessage::EventLog)
        .def_readwrite("OrderStatus", &Message::PackMessage::OrderStatus)
        .def_readwrite("AccountFund", &Message::PackMessage::AccountFund)
        .def_readwrite("AccountPosition", &Message::PackMessage::AccountPosition)
        .def_readwrite("OrderRequest", &Message::PackMessage::OrderRequest)
        .def_readwrite("ActionRequest", &Message::PackMessage::ActionRequest)
        .def_readwrite("RiskReport", &Message::PackMessage::RiskReport)
        .def_readwrite("ColoStatus", &Message::PackMessage::ColoStatus)
        .def_readwrite("AppStatus", &Message::PackMessage::AppStatus)
        .def_readwrite("FastOrder", &Message::PackMessage::FastOrder)
        .def_readwrite("FutureMarketData", &Message::PackMessage::FutureMarketData)
        .def_readwrite("StockMarketData", &Message::PackMessage::StockMarketData)
        .def_readwrite("SpotMarketData", &Message::PackMessage::SpotMarketData)
        .def("to_bytes", [](const Message::PackMessage& s) {
            // 使用std::array确保有足够的内存空间来存储结构体数据
            std::array<char, sizeof(Message::PackMessage)> buffer;
            // 将结构体复制到buffer中，这里使用了memcpy来简化操作，确保类型正确对齐和大小匹配
            std::memcpy(buffer.data(), &s, sizeof(Message::PackMessage));
            // 返回一个指向该内存的bytes对象，注意这里使用了std::move来转移所有权，避免复制整个数组
            return py::bytes(buffer.data(), sizeof(Message::PackMessage));
        });
}