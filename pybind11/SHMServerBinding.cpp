#include <pybind11/pybind11.h>
#include "SHMServer.hpp"
#include "PackMessage.hpp"

namespace py = pybind11;


// 配置结构体
struct ServerConf : public SHMIPC::CommonConf 
{
    static const bool Publish = false;
    static const bool Performance = false;
};

// 显式实例化模板
template class SHMIPC::SHMServer<Message::PackMessage, ServerConf>;

using  SHMServerBase = SHMIPC::SHMServer<Message::PackMessage, ServerConf>;

// Trampoline类，支持Python派生
class PySHMServer : public SHMServerBase
{
public:
    PySHMServer() : SHMServerBase()
    {

    }

    virtual ~PySHMServer()
    {
        
    }
    
    // 重写HandleMsg以支持Python派生
    void HandleMsg() override
    {
        PYBIND11_OVERLOAD(
            void,             // 返回值类型
            SHMServerBase, // 基类
            HandleMsg,         // 函数名
        );
    }
};


PYBIND11_MODULE(shm_server, m) {
    // 绑定SHMServer类
    py::class_<SHMIPC::SHMServer<Message::PackMessage, ServerConf>, PySHMServer>(m, "SHMServer")
        .def(py::init<>())
        .def("Start", &SHMIPC::SHMServer<Message::PackMessage, ServerConf>::Start, "启动SHMServer", py::arg("server_name"), py::arg("cpu_id") = -1)
        .def("Stop", &SHMIPC::SHMServer<Message::PackMessage, ServerConf>::Stop, "停止SHMServer")
        .def("Push", &SHMIPC::SHMServer<Message::PackMessage, ServerConf>::Push, "推送消息到队列", py::arg("msg"))
        .def("Pop", &SHMIPC::SHMServer<Message::PackMessage, ServerConf>::Pop, "从队列弹出消息", py::arg("msg"))
        .def("HandleMsg", &SHMIPC::SHMServer<Message::PackMessage, ServerConf>::HandleMsg, "处理消息回调");

    m.doc() = "SHMServer Python绑定";
}
