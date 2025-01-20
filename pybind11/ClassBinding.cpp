#include <pybind11/pybind11.h>
#include <iostream>

// 基类模板
template <typename T>
class Base {
public:
    virtual ~Base() = default; // 虚析构函数

    virtual void foo() const {
        std::cout << "Base::foo()" << std::endl;
    }
};

// 派生类模板
template <typename T>
class Derived : public Base<T> {
public:
    void foo() const override {
        std::cout << "Derived::foo()" << std::endl;
    }

    void bar() const {
        std::cout << "Derived::bar()" << std::endl;
    }
};

namespace py = pybind11;

// Trampoline 类，用于支持 Python 继承并重写虚函数
template <typename T>
class PyBase : public Base<T> {
public:
    using Base<T>::Base; // 继承构造函数

    void foo() const override {
        PYBIND11_OVERLOAD(
            void,    // 返回值类型
            Base<T>, // 基类
            foo,     // 函数名
        );
    }
};

// 显式实例化模板
template class Base<int>;
template class Derived<int>;

// 绑定函数
template <typename T>
void bind_derived(py::module &m, const std::string &typestr) {
    py::class_<Derived<T>, Base<T>>(m, typestr.c_str())
        .def(py::init<>()) // 默认构造函数
        .def("bar", &Derived<T>::bar); // 绑定 bar 函数
}

PYBIND11_MODULE(Class, m) {
    // 绑定 Base<int>
    py::class_<Base<int>, PyBase<int>>(m, "BaseInt")
        .def(py::init<>())
        .def("foo", &Base<int>::foo);

    // 绑定 Derived<int>
    bind_derived<int>(m, "DerivedInt");
}