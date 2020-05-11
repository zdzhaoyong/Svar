s    = require('bindings')('svar')('sample_module')

console.log(s)

p = new s.Person(3,'zy')
console.log(p.age())
console.log(s.Person.all())

student = new s.Student(3,"zy","npu")

console.log(student.school,student.intro())

