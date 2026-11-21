%module pack_message

%{
#include "Common.hpp"
#include "MarketData.hpp"
#include "PackMessage.hpp"
%}

%include <stdint.i>
%include <std_vector.i>

#define UTIL_HPP

%warnfilter(312);
%ignore MarketData::Check;

%typemap(in) char [ANY] (char *tempArray = nullptr)
{
    const char *source = nullptr;
    Py_ssize_t sourceSize = 0;
    if (PyUnicode_Check($input))
    {
        source = PyUnicode_AsUTF8AndSize($input, &sourceSize);
    }
    else if (PyBytes_Check($input))
    {
        char *byteSource = nullptr;
        if (PyBytes_AsStringAndSize($input, &byteSource, &sourceSize) == -1)
        {
            SWIG_exception_fail(SWIG_TypeError, "could not read bytes");
        }
        source = byteSource;
    }
    else
    {
        SWIG_exception_fail(SWIG_TypeError, "expected str or bytes");
    }

    if (!source && sourceSize != 0)
    {
        SWIG_exception_fail(SWIG_TypeError, "could not encode string");
    }

    tempArray = %new_array($1_dim0, char);
    std::memset(tempArray, 0, $1_dim0);
    const size_t available = sourceSize > 0
        ? static_cast<size_t>(sourceSize)
        : 0;
    const size_t copySize = available < static_cast<size_t>($1_dim0 - 1)
        ? available
        : static_cast<size_t>($1_dim0 - 1);
    if (copySize)
    {
        std::memcpy(tempArray, source, copySize);
    }
    $1 = tempArray;
}

%typemap(freearg) char [ANY]
{
    if (tempArray$argnum)
    {
        %delete_array(tempArray$argnum);
    }
}

%typemap(out) PyObject*
{
    $result = $1;
}

namespace SHMIPC
{
enum EMsgType
{
    EMSG_TYPE_DATA = 0,
    EMSG_TYPE_LOGIN = 1,
    EMSG_TYPE_SERVER_ACK = 2,
    EMSG_TYPE_CLIENT_ACK = 3,
    EMSG_TYPE_HEARTBEAT = 10,
};

template <class T>
struct TDataMsg
{
    uint32_t MsgType;
    uint64_t MsgID;
    uint64_t TimeStamp;
    int32_t ChannelID;
    T Data;
};
}

%include "MarketData.hpp"
%include "PackMessage.hpp"

%template(TDataMsg) SHMIPC::TDataMsg<Message::PackMessage>;

%template(_DoubleVector) std::vector<double>;
%template(_IntVector) std::vector<int>;

%extend MarketData::TStockMarketData
{
    std::vector<double> _get_BidPrice() const
    {
        return std::vector<double>(self->BidPrice, self->BidPrice + 10);
    }

    void _set_BidPrice(const std::vector<double>& values)
    {
        if (values.size() != 10)
        {
            throw std::runtime_error("BidPrice array must have size 10");
        }
        std::copy(values.begin(), values.end(), self->BidPrice);
    }

    std::vector<int> _get_BidVolume() const
    {
        return std::vector<int>(self->BidVolume, self->BidVolume + 10);
    }

    void _set_BidVolume(const std::vector<int>& values)
    {
        if (values.size() != 10)
        {
            throw std::runtime_error("BidVolume array must have size 10");
        }
        std::copy(values.begin(), values.end(), self->BidVolume);
    }

    std::vector<double> _get_AskPrice() const
    {
        return std::vector<double>(self->AskPrice, self->AskPrice + 10);
    }

    void _set_AskPrice(const std::vector<double>& values)
    {
        if (values.size() != 10)
        {
            throw std::runtime_error("AskPrice array must have size 10");
        }
        std::copy(values.begin(), values.end(), self->AskPrice);
    }

    std::vector<int> _get_AskVolume() const
    {
        return std::vector<int>(self->AskVolume, self->AskVolume + 10);
    }

    void _set_AskVolume(const std::vector<int>& values)
    {
        if (values.size() != 10)
        {
            throw std::runtime_error("AskVolume array must have size 10");
        }
        std::copy(values.begin(), values.end(), self->AskVolume);
    }
}

%extend Message::PackMessage
{
    Message::TTest& _get_Test() { return self->Test; }
    void _set_Test(const Message::TTest& value) { self->Test = value; }

    Message::TLoginRequest& _get_LoginRequest() { return self->LoginRequest; }
    void _set_LoginRequest(const Message::TLoginRequest& value)
    {
        self->LoginRequest = value;
    }

    Message::TLoginResponse& _get_LoginResponse()
    {
        return self->LoginResponse;
    }
    void _set_LoginResponse(const Message::TLoginResponse& value)
    {
        self->LoginResponse = value;
    }

    Message::TCommand& _get_Command() { return self->Command; }
    void _set_Command(const Message::TCommand& value) { self->Command = value; }

    Message::TEventLog& _get_EventLog() { return self->EventLog; }
    void _set_EventLog(const Message::TEventLog& value)
    {
        self->EventLog = value;
    }

    Message::TOrderStatus& _get_OrderStatus() { return self->OrderStatus; }
    void _set_OrderStatus(const Message::TOrderStatus& value)
    {
        self->OrderStatus = value;
    }

    Message::TAccountFund& _get_AccountFund() { return self->AccountFund; }
    void _set_AccountFund(const Message::TAccountFund& value)
    {
        self->AccountFund = value;
    }

    Message::TAccountPosition& _get_AccountPosition()
    {
        return self->AccountPosition;
    }
    void _set_AccountPosition(const Message::TAccountPosition& value)
    {
        self->AccountPosition = value;
    }

    Message::TOrderRequest& _get_OrderRequest() { return self->OrderRequest; }
    void _set_OrderRequest(const Message::TOrderRequest& value)
    {
        self->OrderRequest = value;
    }

    Message::TActionRequest& _get_ActionRequest()
    {
        return self->ActionRequest;
    }
    void _set_ActionRequest(const Message::TActionRequest& value)
    {
        self->ActionRequest = value;
    }

    Message::TRiskReport& _get_RiskReport() { return self->RiskReport; }
    void _set_RiskReport(const Message::TRiskReport& value)
    {
        self->RiskReport = value;
    }

    Message::TColoStatus& _get_ColoStatus() { return self->ColoStatus; }
    void _set_ColoStatus(const Message::TColoStatus& value)
    {
        self->ColoStatus = value;
    }

    Message::TAppStatus& _get_AppStatus() { return self->AppStatus; }
    void _set_AppStatus(const Message::TAppStatus& value)
    {
        self->AppStatus = value;
    }

    Message::TFastOrder& _get_FastOrder() { return self->FastOrder; }
    void _set_FastOrder(const Message::TFastOrder& value)
    {
        self->FastOrder = value;
    }

    MarketData::TFutureMarketData& _get_FutureMarketData()
    {
        return self->FutureMarketData;
    }
    void _set_FutureMarketData(const MarketData::TFutureMarketData& value)
    {
        self->FutureMarketData = value;
    }

    MarketData::TStockMarketData& _get_StockMarketData()
    {
        return self->StockMarketData;
    }
    void _set_StockMarketData(const MarketData::TStockMarketData& value)
    {
        self->StockMarketData = value;
    }

    MarketData::TSpotMarketData& _get_SpotMarketData()
    {
        return self->SpotMarketData;
    }
    void _set_SpotMarketData(const MarketData::TSpotMarketData& value)
    {
        self->SpotMarketData = value;
    }

    PyObject* to_bytes()
    {
        return PyBytes_FromStringAndSize(
            reinterpret_cast<const char*>(self), sizeof(*self));
    }
}

%extend Message::TAccountPosition
{
    Message::TFuturePosition& _get_FuturePosition()
    {
        return self->FuturePosition;
    }
    void _set_FuturePosition(const Message::TFuturePosition& value)
    {
        self->FuturePosition = value;
    }

    Message::TStockPosition& _get_StockPosition()
    {
        return self->StockPosition;
    }
    void _set_StockPosition(const Message::TStockPosition& value)
    {
        self->StockPosition = value;
    }
}

%pythoncode %{
import numpy as _numpy


def _stock_array_property(getter_name, setter_name, dtype, property_name):
    def getter(self):
        return _numpy.asarray(
            getattr(self, getter_name)(), dtype=dtype
        ).copy()

    def setter(self, value):
        array = _numpy.asarray(value, dtype=dtype).reshape(-1)
        if array.size != 10:
            raise RuntimeError(
                f"{property_name} array must have size 10"
            )
        vector_type = _IntVector if dtype == _numpy.intc else _DoubleVector
        getattr(self, setter_name)(vector_type(array.tolist()))

    return property(getter, setter)


TStockMarketData.BidPrice = _stock_array_property(
    "_get_BidPrice", "_set_BidPrice", _numpy.float64, "BidPrice"
)
TStockMarketData.BidVolume = _stock_array_property(
    "_get_BidVolume", "_set_BidVolume", _numpy.intc, "BidVolume"
)
TStockMarketData.AskPrice = _stock_array_property(
    "_get_AskPrice", "_set_AskPrice", _numpy.float64, "AskPrice"
)
TStockMarketData.AskVolume = _stock_array_property(
    "_get_AskVolume", "_set_AskVolume", _numpy.intc, "AskVolume"
)


def _union_property(getter_name, setter_name):
    return property(
        lambda self: getattr(self, getter_name)(),
        lambda self, value: getattr(self, setter_name)(value),
    )


PackMessage.Test = _union_property("_get_Test", "_set_Test")
PackMessage.LoginRequest = _union_property(
    "_get_LoginRequest", "_set_LoginRequest"
)
PackMessage.LoginResponse = _union_property(
    "_get_LoginResponse", "_set_LoginResponse"
)
PackMessage.Command = _union_property("_get_Command", "_set_Command")
PackMessage.EventLog = _union_property("_get_EventLog", "_set_EventLog")
PackMessage.OrderStatus = _union_property(
    "_get_OrderStatus", "_set_OrderStatus"
)
PackMessage.AccountFund = _union_property(
    "_get_AccountFund", "_set_AccountFund"
)
PackMessage.AccountPosition = _union_property(
    "_get_AccountPosition", "_set_AccountPosition"
)
PackMessage.OrderRequest = _union_property(
    "_get_OrderRequest", "_set_OrderRequest"
)
PackMessage.ActionRequest = _union_property(
    "_get_ActionRequest", "_set_ActionRequest"
)
PackMessage.RiskReport = _union_property(
    "_get_RiskReport", "_set_RiskReport"
)
PackMessage.ColoStatus = _union_property(
    "_get_ColoStatus", "_set_ColoStatus"
)
PackMessage.AppStatus = _union_property("_get_AppStatus", "_set_AppStatus")
PackMessage.FastOrder = _union_property(
    "_get_FastOrder", "_set_FastOrder"
)
PackMessage.FutureMarketData = _union_property(
    "_get_FutureMarketData", "_set_FutureMarketData"
)
PackMessage.StockMarketData = _union_property(
    "_get_StockMarketData", "_set_StockMarketData"
)
PackMessage.SpotMarketData = _union_property(
    "_get_SpotMarketData", "_set_SpotMarketData"
)

TAccountPosition.FuturePosition = _union_property(
    "_get_FuturePosition", "_set_FuturePosition"
)
TAccountPosition.StockPosition = _union_property(
    "_get_StockPosition", "_set_StockPosition"
)

from enum import IntEnum as _IntEnum


_ENUM_MEMBERS = {
    "EMsgType": (
        "EMSG_TYPE_DATA",
        "EMSG_TYPE_LOGIN",
        "EMSG_TYPE_SERVER_ACK",
        "EMSG_TYPE_CLIENT_ACK",
        "EMSG_TYPE_HEARTBEAT",
    ),
    "ELoginStatus": (
        "ELOGIN_PREPARED",
        "ELOGIN_CONNECTED",
        "ELOGIN_SUCCESSED",
        "ELOGIN_FAILED",
    ),
    "EClientType": (
        "EXTRADER",
        "EXMONITOR",
        "EXMARKETCENTER",
        "EXRISKJUDGE",
        "EXWATCHER",
        "EXQUANT",
        "EHFTRADER",
        "EXDATAPLAYER",
    ),
    "EPermissionOperation": (
        "EUSER_ADD",
        "EUSER_UPDATE",
        "EUSER_DELETE",
    ),
    "EExchange": (
        "ESHSE",
        "ESZSE",
        "EBJSE",
        "ECFFEX",
        "EDCE",
        "ECZCE",
        "ESHFE",
    ),
    "EBusinessType": ("ESTOCK", "ECREDIT", "EFUTURE", "ESPOT"),
    "EOrderType": ("EFAK", "EFOK", "ELIMIT", "EMARKET"),
    "EOrderDirection": (
        "EBUY",
        "ESELL",
        "EREVERSE_REPO",
        "ESUBSCRIPTION",
        "EALLOTMENT",
        "ECOLLATERAL_TRANSFER_IN",
        "ECOLLATERAL_TRANSFER_OUT",
        "EMARGIN_BUY",
        "EREPAY_MARGIN_BY_SELL",
        "ESHORT_SELL",
        "EREPAY_STOCK_BY_BUY",
        "EREPAY_STOCK_DIRECT",
    ),
    "EOrderOffset": ("EOPEN", "ECLOSE", "ECLOSE_TODAY", "ECLOSE_YESTODAY"),
    "ERiskStatusType": (
        "EPREPARE_CHECKED",
        "ECHECKED_PASS",
        "ECHECKED_NOPASS",
        "ENOCHECKED",
        "ECHECK_INIT",
    ),
    "EEngineType": ("ETRADER_ORDER",),
    "StrategyEngine": ("ELATENCYTEST", "EPORTFOLIO"),
    "EOrderStatusType": (
        "EORDER_SENDED",
        "EBROKER_ACK",
        "EEXCHANGE_ACK",
        "EPARTTRADED",
        "EALLTRADED",
        "ECANCELLING",
        "ECANCELLED",
        "EPARTTRADED_CANCELLED",
        "EBROKER_ERROR",
        "EEXCHANGE_ERROR",
        "EACTION_ERROR",
        "ERISK_ORDER_REJECTED",
        "ERISK_ACTION_REJECTED",
        "ERISK_CHECK_INIT",
    ),
    "EOrderSide": (
        "EOPEN_LONG",
        "ECLOSE_TD_LONG",
        "ECLOSE_YD_LONG",
        "EOPEN_SHORT",
        "ECLOSE_TD_SHORT",
        "ECLOSE_YD_SHORT",
        "ECLOSE_LONG",
        "ECLOSE_SHORT",
        "ESIDE_REVERSE_REPO",
        "ESIDE_SUBSCRIPTION",
        "ESIDE_ALLOTMENT",
        "ESIDE_COLLATERAL_BUY",
        "ESIDE_COLLATERAL_SELL",
        "ESIDE_MARGIN_BUY",
        "ESIDE_REPAY_MARGIN_BY_SELL",
        "ESIDE_SHORT_SELL",
        "ESIDE_REPAY_STOCK_BY_BUY",
        "ESIDE_REPAY_STOCK_DIRECT",
    ),
    "ECommandType": (
        "EUPDATE_RISK_LIMIT",
        "EDELETE_RISK_LIMIT",
        "EUPDATE_RISK_POSITION_LIMIT",
        "EDELETE_RISK_POSITION_LIMIT",
        "EUPDATE_RISK_ACCOUNT_LOCKED",
        "EDELETE_RISK_ACCOUNT_LOCKED",
        "EUPDATE_USERPERMISSION",
        "EKILL_APP",
        "ESTART_APP",
        "ETRANSFER_FUND_IN",
        "ETRANSFER_FUND_OUT",
        "EREPAY_MARGIN_DIRECT",
    ),
    "EEventLogLevel": ("EINFO", "EWARNING", "EERROR"),
    "ERiskRejectedType": (
        "EFLOW_LIMITED",
        "ESELF_MATCHED",
        "ETICKER_ACTION_LIMITED",
        "EORDER_ACTION_LIMITED",
        "EINVALID_PRICE",
        "EACCOUNT_NOT_FOUND",
        "ETICKER_NOT_FOUND",
        "EVOLUME_EXCEEDED",
        "EREQUEST_LIMITED",
        "EACCOUNT_LOCKED_ALL",
        "EACCOUNT_LOCKED_BUY",
        "EACCOUNT_LOCKED_BUY_OPEN",
        "EACCOUNT_LOCKED_BUY_CLOSE",
        "EACCOUNT_LOCKED_BUY_CLOSE_TODAY",
        "EACCOUNT_LOCKED_BUY_CLOSE_YESTODAY",
        "EACCOUNT_LOCKED_SELL",
        "EACCOUNT_LOCKED_SELL_OPEN",
        "EACCOUNT_LOCKED_SELL_CLOSE",
        "EACCOUNT_LOCKED_SELL_CLOSE_TODAY",
        "EACCOUNT_LOCKED_SELL_CLOSE_YESTODAY",
        "EACCOUNT_LOCKED_OPEN",
        "EACCOUNT_LOCKED_CLOSE",
        "ETICKER_LOCKED_ALL",
        "ETICKER_LOCKED_BUY",
        "ETICKER_LOCKED_BUY_OPEN",
        "ETICKER_LOCKED_BUY_CLOSE",
        "ETICKER_LOCKED_BUY_CLOSE_TODAY",
        "ETICKER_LOCKED_BUY_CLOSE_YESTODAY",
        "ETICKER_LOCKED_SELL",
        "ETICKER_LOCKED_SELL_OPEN",
        "ETICKER_LOCKED_SELL_CLOSE",
        "ETICKER_LOCKED_SELL_CLOSE_TODAY",
        "ETICKER_LOCKED_SELL_CLOSE_YESTODAY",
        "ETICKER_LOCKED_OPEN",
        "ETICKER_LOCKED_CLOSE",
        "ETICKER_LONG_LIMIT",
        "ETICKER_SHORT_LIMIT",
        "ETICKER_NET_LONG_LIMIT",
        "ETICKER_NET_SHORT_LIMIT",
        "ETICKER_LONG_INSUFFICIENT",
        "ETICKER_SHORT_INSUFFICIENT",
        "ESTRATEGY_LONG_LIMIT",
        "ESTRATEGY_SHORT_LIMIT",
        "ESTRATEGY_NET_LONG_LIMIT",
        "ESTRATEGY_NET_SHORT_LIMIT",
    ),
    "ERiskReportType": (
        "ERISK_LIMIT",
        "ERISK_POSITION_LIMIT",
        "ERISK_ACCOUNT_LOCKED",
        "ERISK_EVENTLOG",
    ),
    "EAccountLockSide": (
        "EUNLOCK",
        "ELOCK_ACCOUNT",
        "ELOCK_OPEN",
        "ELOCK_CLOSE",
        "ELOCK_BUY",
        "ELOCK_BUY_OPEN",
        "ELOCK_BUY_CLOSE",
        "ELOCK_BUY_CLOSE_TODAY",
        "ELOCK_BUY_CLOSE_YESTODAY",
        "ELOCK_SELL",
        "ELOCK_SELL_OPEN",
        "ELOCK_SELL_CLOSE",
        "ELOCK_SELL_CLOSE_TODAY",
        "ELOCK_SELL_CLOSE_YESTODAY",
    ),
    "EMessageType": (
        "ETest",
        "ELoginRequest",
        "ELoginResponse",
        "ECommand",
        "EEventLog",
        "EOrderStatus",
        "EAccountFund",
        "EAccountPosition",
        "EOrderRequest",
        "EActionRequest",
        "ERiskReport",
        "EColoStatus",
        "EAppStatus",
        "EFastOrder",
        "EFutureMarketData",
        "EStockMarketData",
        "ESpotMarketData",
    ),
}


def _define_enum(enum_name, member_names):
    enum_type = _IntEnum(
        enum_name,
        {name: globals()[name] for name in member_names},
        module=__name__,
    )
    globals()[enum_name] = enum_type
    for member_name in member_names:
        globals()[member_name] = enum_type[member_name]


for _enum_name, _enum_members in _ENUM_MEMBERS.items():
    _define_enum(_enum_name, _enum_members)

del _enum_name, _enum_members
%}
