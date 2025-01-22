#include <pybind11/pybind11.h>
#include "Common.hpp"
#include "PackMessage.hpp"
#include "SPSCQueue.hpp"

namespace py = pybind11;

PYBIND11_MODULE(spscqueue, m) {

    py::class_<SHMIPC::SPSCQueue<Message::PackMessage, SHMIPC::CommonConf::ShmQueueSize>>(m, "SPSCQueue")
        .def(py::init<>())
        .def("Push", &SHMIPC::SPSCQueue<Message::PackMessage, SHMIPC::CommonConf::ShmQueueSize>::Push)
        .def("Pop", &SHMIPC::SPSCQueue<Message::PackMessage, SHMIPC::CommonConf::ShmQueueSize>::Pop)
        .def("Reset", &SHMIPC::SPSCQueue<Message::PackMessage, SHMIPC::CommonConf::ShmQueueSize>::Reset);

    m.doc() = "Python binding for SPSCQueue";
}