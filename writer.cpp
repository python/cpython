#include <Python.h>
#include <vector>
#include <string>
#include <iostream>


static std::string usage = "./writer <string_to_write>";


int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << usage << std::endl;
        return 1;
    }

    const char *input_str = argv[1];
    PyObject *py_str = PyUnicode_FromString(input_str);
    std::cout << "Written string: " << input_str << std::endl;

    std::cout << "Waiting.." << std::endl;
    std::cin.get();

    // Note:
    // This may give segfault if the reader has mapped the string, read, and reduced ref count.
    // Need to fix garbage collection and ref counting when mapping across processes.
    Py_ssize_t py_str_size = PyUnicode_GET_LENGTH(py_str);
    wchar_t *cstr = PyUnicode_AsWideCharString(py_str, &py_str_size);
    std::wcout << cstr << std::endl;

    return 0;
}