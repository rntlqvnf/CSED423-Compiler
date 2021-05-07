Segmentation fault
1c1,87
< ../test_good.cl:10: The declared return type of method init is C but the type of the method body is 
\ No newline at end of file
---
> #17
> _program
>   #11
>   _class
>     C
>     Object
>     "../test_good.cl"
>     (
>     #2
>     _attr
>       a
>       Int
>       #2
>       _no_expr
>       : _no_type
>     #3
>     _attr
>       b
>       Bool
>       #3
>       _no_expr
>       : _no_type
>     #10
>     _method
>       init
>       #4
>       _formal
>         x
>         Int
>       #4
>       _formal
>         y
>         Bool
>       C
>       #9
>       _block
>         #6
>         _assign
>           a
>           #6
>           _object
>             x
>           : Int
>         : Int
>         #7
>         _assign
>           b
>           #7
>           _object
>             y
>           : Bool
>         : Bool
>         #8
>         _object
>           self
>         : SELF_TYPE
>       : SELF_TYPE
>     )
>   #17
>   _class
>     Main
>     Object
>     "../test_good.cl"
>     (
>     #16
>     _method
>       main
>       C
>       #15
>       _dispatch
>         #15
>         _new
>           C
>         : C
>         init
>         (
>         #15
>         _int
>           1
>         : Int
>         #15
>         _bool
>           1
>         : Bool
>         )
>       : C
>     )
