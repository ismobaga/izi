# IZI


## Mot cle 
`var, if, switch, case, default while for, print, class, fun, return, this, super, new`
## Types
`number, string, function, class`
## Operators
- `number` : `+, -, *, /`
- `string` : `+`(concat)
- `class`  : `<` inheritance


## Features
- function def
- function call
- closure 
- class definition
- class instanciation
- class get field
- class set field
- call methods
- this 
- super class
- super methodscall
```js
var iz = 21;
var b = "dsjsdjs";
var b = 21.22;
var i = 1;
while(i<10){
    print i;
    i = i+1;
}

fun areWeHavingItYet() {
  print "Yes we are!";
}

print areWeHavingItYet;
fun noReturn() {
  print "Do stuff";
  // No return here.
}

print noReturn(); // nil

fun sum(a, b, c) {
  print a + b + c;
}

print 4 sum(5, 6, 7);

class A{};
class B < A{}
print A;
var a = A();
a.b = "hello";

print a.b;
```

# TODO 
- [ ] Opimizaion method call
- [ ] debugger (code printing)
- [ ] add importing modules
- [ ] add a core module (list, map, io, ...)
- [ ] string interpolation
- [ ] thread
- [ ] gc
- [ ] Dynamically load C libraries
## Tools
- `prremake`
- `vcode-premake`

## Resources
[notes.md](notes.md)