#include <pybind11/pybind11.h>
#include <string>
#include "HPPackClient.hpp"

namespace py = pybind11;


PYBIND11_MODULE(hp_pack_client, m) {
    m.doc() = "HPPackClient binding with pybind11";

    py::class_<HPPackClient>(m, "HPPackClient")
        .def(py::init<const std::string&, unsigned int>(),
             py::arg("ip"), py::arg("port"))
        .def("Start", &HPPackClient::Start, py::arg("account"), py::call_guard<py::gil_scoped_release>())
        .def("Stop", &HPPackClient::Stop, py::call_guard<py::gil_scoped_release>())
        .def("SendData", &HPPackClient::SendData, py::arg("msg"), py::call_guard<py::gil_scoped_release>())
        .def("UpdateAppStatus", &HPPackClient::UpdateAppStatus, py::arg("cmd"), py::arg("app_name"), py::call_guard<py::gil_scoped_release>());
}