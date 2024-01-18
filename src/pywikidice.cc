#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>
#include <string_view>

#include "wikidice_query.h"

PYBIND11_MODULE(pywikidice, m) {
    pybind11::class_<net_zelcon::wikidice::Session>(m, "Session")
        .def(pybind11::init<std::string_view>())
        .def("pick_random_article",
             &net_zelcon::wikidice::Session::pick_random_article)
        .def("autocomplete_category_name",
             &net_zelcon::wikidice::Session::autocomplete_category_name)
        .def("take", &net_zelcon::wikidice::Session::take)
        .def("get_entry", &net_zelcon::wikidice::Session::get_entry);
}