1,46c1,6
< #8
< _program
<   #8
<   _class
<     MyClass
<     Object
<     "../case1.cl"
<     (
<     #7
<     _method
<       do
<       #2
<       _formal
<         arg
<         Object
<       Void
<       #6
<       _typcase
<         #3
<         _object
<           arg
<         : _no_type
<         #4
<         _branch
<           o1
<           Object1
<           #4
<           _int
<             1
<           : _no_type
<         #5
<         _branch
<           o2
<           Object2
<           #5
<           _dispatch
<             #5
<             _object
<               o2
<             : _no_type
<             special
<             (
<             )
<           : _no_type
<       : _no_type
<     )
---
> Class Main is not defined.
> ../case1.cl:7: Undefined return type Void in method do.
> ../case1.cl:4: Class Object1 of case branch is undefined.
> ../case1.cl:5: Class Object2 of case branch is undefined.
> ../case1.cl:5: Dispatch on undefined class Object2.
> Compilation halted due to static semantic errors.
