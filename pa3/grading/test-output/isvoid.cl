Segmentation fault
1c1,44
< ../isvoid.cl:5: The declared return type of method main is Object but the type of the method body is 
\ No newline at end of file
---
> #6
> _program
>   #6
>   _class
>     Main
>     IO
>     "../isvoid.cl"
>     (
>     #2
>     _attr
>       x
>       Bool
>       #2
>       _no_expr
>       : _no_type
>     #5
>     _method
>       main
>       Object
>       #5
>       _block
>         #4
>         _isvoid
>           #4
>           _bool
>             1
>           : Bool
>         : Bool
>         #4
>         _isvoid
>           #4
>           _bool
>             0
>           : Bool
>         : Bool
>         #4
>         _isvoid
>           #4
>           _object
>             x
>           : Bool
>         : Bool
>       : Bool
>     )
