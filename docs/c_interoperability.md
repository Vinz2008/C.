# Interoperability with C

## Mangling

### Order of Mangling

TODO

### Struct Member Functions

Struct Member functions will be mangled with 2 underscores :
"struct_variable.function_name()" -> "struct_name__function_name(&struct_variable)

### Namespaces

Namespaced functions will be mangled with 3 underscores :
"namespace::function_name()" -> "namespace___function_name()"

### Templates

Templated functions and structs will be mangled with 4 underscores and then the type :
"function_name~int~()" -> "function_name____int()"
"function_name~i64 ptr~()" -> "function_name____i64_ptr()"
"struct struct_name~int~" -> "struct struct_name____int"
"struct struct_name~i16 ptr~" -> "struct struct_name____i16_ptr"