import svar

basic=svar.load("/data/zhaoyong/Program/Apps/Tests/JSVar/python/build/libbasic.so")

pyobj={"i":1,"d":1.2,"s":"hello world","v":[1,2,3]}
print("pyobj is:",pyobj)

ret=basic.obtainInfo(pyobj)
print("ret is:",ret)
print("convert to py:",ret.py())

print(basic.dtos(100.423))

c=basic.add(34,3)
print(c)
print(basic.add(c,3))


#robj=basic.obtainInfo(pyobj)
#print(robj)

