1,90c1,3
< #16
< _program
<   #4
<   _class
<     C1
<     SELF_TYPE
<     "../self_type.cl"
<     (
<     #2
<     _method
<       f
<       #2
<       _formal
<         x
<         Int
<       #2
<       _formal
<         y
<         SELF_TYPE
<       Int
<       #2
<       _int
<         1
<       : _no_type
<     )
<   #16
<   _class
<     SELF_TYPE
<     Object
<     "../self_type.cl"
<     (
<     #7
<     _attr
<       b
<       SELF_TYPE
<       #7
<       _new
<         SELF_TYPE
<       : _no_type
<     #15
<     _method
<       g
<       #9
<       _formal
<         x
<         Int
<       SELF_TYPE
<       #15
<       _let
<         y
<         SELF_TYPE
<         #10
<         _no_expr
<         : _no_type
<         #14
<         _typcase
<           #11
<           _object
<             y
<           : _no_type
<           #12
<           _branch
<             a
<             SELF_TYPE
<             #12
<             _new
<               SELF_TYPE
<             : _no_type
<           #13
<           _branch
<             a
<             Int
<             #13
<             _static_dispatch
<               #13
<               _new
<                 SELF_TYPE
<               : _no_type
<               SELF_TYPE
<               g
<               (
<               #13
<               _object
<                 a
<               : _no_type
<               )
<             : _no_type
<         : _no_type
<       : _no_type
<     )
---
> ../self_type.cl:16: Redefinition of basic class SELF_TYPE.
> ../self_type.cl:4: Class C1 cannot inherit class SELF_TYPE.
> Compilation halted due to static semantic errors.
