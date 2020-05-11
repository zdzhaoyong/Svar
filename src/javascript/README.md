# The V8 wrapper for svar_modules.

[Svar](https://github.com/zdzhaoyong/Svar) is a project aims to unify c++ interfaces for all languages. 
Now we are able to use svar_modules in some languages such as python.
This project is use to provide javascript support.

**Warning**: This project is still under develop.

svar_napi auto wrap the following c++ objects:

* Json Types:  number, boolean, string, object, array
* Function  :  Both Js call C++ and C++ call JS (Not thread safe now)
* C++ Classes: Auto wrap constructors, member functions, properties
* C++ Objects: User don't need to declare every classes, svar auto wrap them.

TODO:

* Class inherit
* ThreadSafe Callback

## Compile and Install

Please install [Svar](https://github.com/zdzhaoyong/Svar) firstly.

```
cd svar_v8
npm install .
```

## Usage Demo

Write the following index.js file and run with node:

```
node sample_module.js
```


