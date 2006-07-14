@echo off
if not defined HOST_PYTHON set HOST_PYTHON=python
%HOST_PYTHON% build_ssl.py %1 %2

