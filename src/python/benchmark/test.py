import time
import bench

num=10000
items=10000
t = time.time()
i=0
while i<num:
  i=i+1
  a=bench.BenchClass.create(items)

t1 =time.time()
 
print("create instance by",num,"times used",t1-t,"seconds")

print(bench.BenchClass(1).get(0))
