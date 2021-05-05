1,181c1,6
< #35
< _program
<   #12
<   _class
<     List
<     Object
<     "../redef_feature.cl"
<     (
<     #2
<     _method
<       isNil
<       Object
<       #2
<       _dispatch
<         #2
<         _object
<           self
<         : _no_type
<         abort
<         (
<         )
<       : _no_type
<     #7
<     _method
<       cons
<       #4
<       _formal
<         hd
<         Int
<       Cons
<       #7
<       _let
<         new_cell
<         Cons
<         #5
<         _new
<           Cons
<         : _no_type
<         #6
<         _dispatch
<           #6
<           _object
<             new_cell
<           : _no_type
<           init
<           (
<           #6
<           _object
<             hd
<           : _no_type
<           #6
<           _object
<             self
<           : _no_type
<           )
<         : _no_type
<       : _no_type
<     #9
<     _method
<       car
<       Int
<       #9
<       _dispatch
<         #9
<         _object
<           self
<         : _no_type
<         abort
<         (
<         )
<       : _no_type
<     #11
<     _method
<       cdr
<       List
<       #11
<       _dispatch
<         #11
<         _object
<           self
<         : _no_type
<         abort
<         (
<         )
<       : _no_type
<     )
<   #31
<   _class
<     Cons
<     List
<     "../redef_feature.cl"
<     (
<     #15
<     _attr
<       car
<       Int
<       #15
<       _no_expr
<       : _no_type
<     #16
<     _attr
<       cdr
<       List
<       #16
<       _no_expr
<       : _no_type
<     #18
<     _method
<       isNil
<       Bool
<       #18
<       _bool
<         0
<       : _no_type
<     #26
<     _method
<       init
<       #20
<       _formal
<         hd
<         Int
<       #20
<       _formal
<         tl
<         List
<       Cons
<       #25
<       _block
<         #22
<         _assign
<           car
<           #22
<           _object
<             hd
<           : _no_type
<         : _no_type
<         #23
<         _assign
<           cdr
<           #23
<           _object
<             tl
<           : _no_type
<         : _no_type
<         #24
<         _object
<           self
<         : _no_type
<       : _no_type
<     #28
<     _method
<       car
<       Int
<       #28
<       _object
<         car
<       : _no_type
<     #30
<     _method
<       cdr
<       List
<       #30
<       _object
<         cdr
<       : _no_type
<     )
<   #35
<   _class
<     Nil
<     List
<     "../redef_feature.cl"
<     (
<     #34
<     _method
<       isNil
<       Bool
<       #34
<       _bool
<         1
<       : _no_type
<     )
---
> ../redef_feature.cl:18: In redefined method isNil, return type Bool is different from original return type Object.
> ../redef_feature.cl:34: In redefined method isNil, return type Bool is different from original return type Object.
> Class Main is not defined.
> ../redef_feature.cl:9: Inferred return type Object of method car does not conform to declared return type Int.
> ../redef_feature.cl:11: Inferred return type Object of method cdr does not conform to declared return type List.
> Compilation halted due to static semantic errors.
