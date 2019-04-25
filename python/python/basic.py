import svar

basic=svar.loadSvarPlugin("/data/zhaoyong/Program/Apps/Tests/JSVar/python/build/libbasic.so")

print("basic type ")
print("context")

pyobj={"i":1,"d":1.2,"s":"hello world","v":[1,2,3]}
print("pyobj is:",pyobj)


#robj=basic.obtainInfo(pyobj)
#print(robj)

