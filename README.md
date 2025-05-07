# ProX Programming Language (v1.0.0 Alpha)

[![Build Status](https://github.com/ProXentix/ProX/actions/workflows/build.yml/badge.svg)](https://github.com/ProXentix/ProX/actions)
[![Discourse Chat](https://img.shields.io/badge/discourse-join_chat-brightgreen.svg)](https://community.proxlang.org)

> **ProX** is a high-level, beginner-friendly programming language inspired by Python — built to empower developers with simplicity, speed, and readability.

---

## Contents

- [General Information](#general-information)
- [Installation](#installation)
- [Build Instructions](#build-instructions)
- [Contributing](#contributing)
- [Testing](#testing)
- [Documentation](#documentation)
- [License](#license)

---

## General Information

- **Website**: [https://proxlang.org](https://proxlang.org)
- **Source Code**: [https://github.com/ProXentix/ProX](https://github.com/ProXentix/ProX)
- **Documentation**: [https://docs.proxlang.org](https://docs.proxlang.org)
- **Issue Tracker**: [https://github.com/ProXentix/ProX/issues](https://github.com/ProXentix/ProX/issues)
- **Community Chat**: [https://community.proxlang.org](https://community.proxlang.org)

---

## Installation

You can install ProX using the official installer or build it from source.

**To install from PyPI:**

```bash
pip install prox

To install manually:

git clone https://github.com/ProXentix/ProX.git
cd ProX
python setup.py install


---

Build Instructions

To build from source:

On Linux/macOS:

./configure
make
make test
sudo make install

On Windows:

.\configure.bat
nmake
nmake test
nmake install

For an optimized build:

./configure --enable-optimizations
make


---

Contributing

We welcome contributions! Please read the Developer Guide before submitting pull requests.

git clone https://github.com/ProXentix/ProX.git
cd ProX


---

Testing

Run all tests using:

make test

Run specific tests in verbose mode:

make test TESTOPTS="-v test_parser test_stdlib"


---

Documentation

Read Online: https://docs.proxlang.org

Build Locally:


cd docs
make html


---

License

© 2025 ProXentix Foundation. All rights reserved.

ProX is licensed under the MIT License.


---

Join the Community

Be part of the future of ProX:

ProX Community

Discord

GitHub Discussions
