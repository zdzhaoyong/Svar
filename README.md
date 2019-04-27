# Svar, A Tiny Header Brings Dynamic C++ Interface

---

[![Build Status](https://travis-ci.org/zdzhaoyong/Svar.svg?branch=master)](https://travis-ci.org/zdzhaoyong/Svar)
[![License](https://img.shields.io/badge/license-BSD--2--Clause-blue.svg)](./LICENSE)
[![Version](https://img.shields.io/github/release/zdzhaoyong/Svar.svg)](https://github.com/zdzhaoyong/Svar/releases)

---
[English](./README_en.md)

# 1. Why Svar? 把C++变成动态语言的高性能中介！

[Dynamic C++ Proposal](https://www.codeproject.com/Articles/31988/Dynamic-C-Proposal)

C++作为静态类型语言，在执行高效的同时也给使用带来了一系列的麻烦.
Dynamism brings several advantages, some of these are:

* More abstraction (no FBC).
* Easy framework development.
* Easy scripting language integration.
* Independent external objects.
* Generic compiled code (discussed later).
* Faster compiling (discussed later).
* Less ugly C++ hacks.

使用Svar可以带来以下特性：
1. 一个动态链接库，可同时作为C++,Python,Node-JS等语言的模块使用;
2. 统一C++库接口，可插件机制调用，发布的库自带文档，再也不用繁琐的*.h头文件接口说明;
3. 给C++带来动态特性，包裹变量，函数，类到Svar中，可高效转换及调用;
4. 内置Json支持，参数配置，解析，线程安全读写,模块间数据解耦分享;


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









