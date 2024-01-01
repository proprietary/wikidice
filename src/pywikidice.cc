#include <string>
#include <pybind11/pybind11.h>

int add(int a, int b) {
    return a + b;
}

PYBIND11_MODULE(pywikidice, m) {
    m.doc() = "pywikidice";
    m.def("add", &add, "Adds two numbers");
}