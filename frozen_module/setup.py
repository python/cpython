#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""The setup script."""
import os
from setuptools import setup, find_packages, Extension

if os.path.isfile('README.rst'):
    with open('README.rst') as readme_file:
        readme = readme_file.read()
else:
    readme = ""

if os.path.isfile('HISTORY.rst'):
    with open('HISTORY.rst') as history_file:
        history = history_file.read()
else:
    history = ""

requirements = []

setup_requirements = []

test_requirements = []

setup(
    author="Jeethu Rao",
    author_email='jeethu@jeethurao.com',
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Intended Audience :: Developers',
        'Natural Language :: English',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
    ],
    description="A module to freeze pyc files into C structures",
    install_requires=requirements,
    long_description=readme + '\n\n' + history,
    include_package_data=True,
    keywords='frozen_module',
    name='frozen_module',
    packages=find_packages(include=['frozen_module']),
    ext_modules=[
        Extension('frozen_module._serializer',
                  sources=['_serializer/_serializer.c'])
    ],
    setup_requires=setup_requirements,
    test_suite='tests',
    tests_require=test_requirements,
    version='0.0.1',
    zip_safe=False,
)
