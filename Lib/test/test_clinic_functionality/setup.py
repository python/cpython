from setuptools import setup, Extension
from test import support


def main():
    SOURCE = support.findfile('test_clinic_functionality.c', subdir='test_clinic_functionality')
    module = Extension('test_clinic_functionality', sources=[SOURCE])
    setup(name='test_clinic_functionality', version='0.0', ext_modules=[module])


if __name__ == '__main__':
    main()
