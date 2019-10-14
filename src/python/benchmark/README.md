# Comparison between pybind11 and Svar

This folder performs a simple compare which used to evaluate the performance of bind C++ using Svar instead of pybind11.
Two method all compile a python module and use the same python script to run the module.

## Run test.py with Svar

```
cd bench_svar
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cp ../../test.py .
python3 test.py
```

The output should like this:

```
build> python3 test.py 
create instance by 10000 times used 0.04513406753540039 seconds
<BenchClass::BenchClassData object at 0x7f5c4885f2b8>
```

## Run test.py with pybind11

```
cd bench_svar
git clone https://github.com/pybind/pybind11
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cp ../../test.py .
python3 test.py 
```

The output should like this:

```
build> python3 test.py 
create instance by 10000 times used 0.2272651195526123 seconds
Traceback (most recent call last):
  File "test.py", line 16, in <module>
    print(bench.BenchClass(1).get(0))
TypeError: Unable to convert function return value to a Python type! The signature was
	(self: bench.BenchClass, arg0: int) -> BenchClass::BenchClassData
```

**The result demostrate that Svar is much more faster than pybin11 in this example and auto binded class BenchClass::BenchClassData!**


