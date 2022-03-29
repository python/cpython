#include <Python.h>
#include <vector>
#include <string>
#include <iostream>


static std::string usage = "./reader <map_name> <size>";


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << usage << std::endl;
        return 1;
    }

    const char *path = argv[1];
    size_t size = 0;
    if (sscanf(argv[2], "%zu", &size) != 1)
    {
        std::cerr << "Cannot read size" << std::endl;
        return 1;
    }

    std::cout << "Mapping " << size << " bytes from " << path << std::endl;

    PyObject *obj = PyObject_FromMmap(&PyUnicode_Type, path, size);
    if (obj == nullptr)
    {
        std::cerr << "Cannot create PyObject from mmap" << std::endl;
    }

    Py_ssize_t strsize = PyUnicode_GET_LENGTH(obj);
    wchar_t *cstr = PyUnicode_AsWideCharString(obj, &strsize);
    std::cout << "Read string: ";
    std::wcout << cstr << std::endl;

    std::cout << "Waiting.." << std::endl;
    std::cin.get();

    return 0;
}