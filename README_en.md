# Svar, A Tiny Header Brings Dynamic C++ Interface

---
[![Build Status](https://travis-ci.org/zdzhaoyong/Svar.svg?branch=master)](https://travis-ci.org/zdzhaoyong/Svar)
[![License](https://img.shields.io/badge/license-BSD--2--Clause-blue.svg)](./LICENSE)
[![Version](https://img.shields.io/github/release/zdzhaoyong/Svar.svg)](https://github.com/zdzhaoyong/Svar/releases)

---
[English](./README_en.md)

# 1. Why Svar? An Elegent Dynamism Implementation Based on C++11.

[Dynamic C++ Proposal](https://www.codeproject.com/Articles/31988/Dynamic-C-Proposal)


Dynamism brings several advantages, some of these are:

* More abstraction (no FBC).
* Easy framework development.
* Easy scripting language integration.
* Independent external objects.
* Generic compiled code (discussed later).
* Faster compiling (discussed later).
* Less ugly C++ hacks.


Using Svar brings the following features:
1. A dynamic link library that can be used as a module in languages such as C++, Python, and Node-JS;
2. Unified C++ library interface, which can be called as a plug-in module. The released library comes with documentation,  making *.h header file interface description unnecessary;
3. Dynamic features.It wraps variables, functions, and classes into Svar while maintaining efficiency;
4. Built-in Json support, parameter configuration parsing, thread-safe reading and writing, data decoupling sharing between modules, etc.


# 2. Usage

## 2.1. Obtain source code from github

```
git clone https://github.com/zdzhaoyong/Svar
```

## 2.2. C++ Users

Compile the source code with cmake:
```
cd Svar
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

Usage help:
```
svar --help
```

Perform tests:
```
svar -tests
```

Show the context of the sample plugin module:

```
# Show the structure of a plugin
svar -plugin libsample_module.so
# Show the documentation of a named class
svar -plugin libsample_module.so -name __builtin__.Json
# Show the documentation of a named function
svar -plugin libsample_module.so -name add
```

Use sample plugin module with c++: [sample_use](./src/cpp/sample_use).

## 2.3. Python Users

Install svar using pip:
```
# python2
sudo pip install Svar
# python3
sudo pip3 install Svar
```

Use with existed svar sample module:
```
import svar
# Show the help of the svar module
help(svar)
# Import a svar module as a python module
sample=svar.load("libsample_module.so")
# Show the help of sample module
help(sample)
result_svar=sample.add(2,3)
result_py=result_svar.py()
```

## 2.4. Node-JS Users

Install svar using npm:
```
npm install Svar
```

Use with existed svar sample module:
```
svar=require("svar")
# Import a svar module as a js module
sample=svar.load("libsample_module.so")
result_svar=sample.add(2,3)
result_js=result_svar.js()
```









