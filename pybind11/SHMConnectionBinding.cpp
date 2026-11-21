#include <pybind11/pybind11.h>
#include "SHMConnection.hpp"
#include "PackMessage.hpp"

namespace py = pybind11;

struct ClientConf : public SHMIPC::CommonConf
{
    static const bool Publish = false;
};

struct DataClientConf : public SHMIPC::CommonConf
{
    static const bool Publish = true;
};

PYBIND11_MODULE(shm_connection, m) {

    using SHMConnectionClass = SHMIPC::SHMConnection<Message::PackMessage, ClientConf>;
    py::class_<SHMConnectionClass>(m, "SHMConnection")
        .def(py::init<const std::string&>())
        .def("Start", &SHMConnectionClass::Start, py::arg("server_name"))
        .def("Push", &SHMConnectionClass::Push)
        .def("Pop", &SHMConnectionClass::Pop)
        .def("IsConnected", &SHMConnectionClass::IsConnected);

    using SHMDataConnectionClass = SHMIPC::SHMConnection<Message::PackMessage, DataClientConf>;
    py::class_<SHMDataConnectionClass>(m, "SHMDataConnection")
        .def(py::init<const std::string&>())
        .def("Start", &SHMDataConnectionClass::Start, py::arg("server_name"))
        .def("Push", &SHMDataConnectionClass::Push)
        .def("Pop", &SHMDataConnectionClass::Pop)
        .def("IsConnected", &SHMDataConnectionClass::IsConnected);
    m.doc() = "Python binding for SHMConnection";
}
