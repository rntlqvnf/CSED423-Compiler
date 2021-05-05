1,28c1,2
< #3
< _program
<   #3
<   _class
<     Main
<     Object
<     "../block_incorrect.cl"
<     (
<     #2
<     _method
<       main
<       Int
<       #2
<       _block
<         #2
<         _string
<           "123"
<         : _no_type
<         #2
<         _int
<           1
<         : _no_type
<         #2
<         _bool
<           0
<         : _no_type
<       : _no_type
<     )
---
> ../block_incorrect.cl:2: Inferred return type Bool of method main does not conform to declared return type Int.
> Compilation halted due to static semantic errors.
