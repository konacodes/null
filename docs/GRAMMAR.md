# Null Language Grammar

## EBNF Specification

This document defines the formal grammar of the Null programming language.

### Lexical Elements

```ebnf
(* Identifiers *)
identifier     = letter , { letter | digit | "_" } ;
letter         = "a" | "b" | ... | "z" | "A" | "B" | ... | "Z" | "_" ;
digit          = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;

(* Literals *)
int_literal    = digit , { digit } ;
float_literal  = digit , { digit } , "." , digit , { digit } ;
string_literal = '"' , { string_char } , '"' ;
string_char    = ? any character except '"' or '\' ?
               | '\' , escape_char ;
escape_char    = 'n' | 't' | 'r' | '\\' | '"' | '0' ;
bool_literal   = "true" | "false" ;

(* Comments *)
line_comment   = "--" , { ? any character except newline ? } , newline ;
block_comment  = "---" , { ? any character ? } , "---" ;
```

### Program Structure

```ebnf
program        = { declaration } ;

declaration    = fn_decl
               | struct_decl
               | var_decl
               | extern_decl
               | use_directive ;

use_directive  = "@use" , string_literal ;

extern_decl    = "@extern" , string_literal , "do" , { extern_fn_decl } , "end" ;

extern_fn_decl = "fn" , identifier , "(" , [ param_list ] , ")" , "->" , type ;
```

### Declarations

```ebnf
fn_decl        = "fn" , identifier , "(" , [ param_list ] , ")" , "->" , type , "do" , block , "end" ;

param_list     = param , { "," , param } ;
param          = identifier , "::" , type ;

struct_decl    = "struct" , identifier , "do" , { field_decl } , "end" ;
field_decl     = identifier , "::" , type ;

var_decl       = ( "let" | "mut" ) , identifier , [ "::" , type ] , "=" , expression ;
```

### Types

```ebnf
type           = primitive_type
               | pointer_type
               | array_type
               | slice_type
               | identifier ;  (* struct type *)

primitive_type = "void" | "bool"
               | "i8" | "i16" | "i32" | "i64"
               | "u8" | "u16" | "u32" | "u64"
               | "f32" | "f64" ;

pointer_type   = "ptr" , "<" , type , ">" ;
array_type     = "[" , type , ";" , int_literal , "]" ;
slice_type     = "[" , type , "]" ;
```

### Statements

```ebnf
block          = { statement } ;

statement      = var_decl
               | assignment
               | if_stmt
               | while_stmt
               | for_stmt
               | return_stmt
               | expression_stmt ;

assignment     = lvalue , "=" , expression ;
lvalue         = identifier
               | lvalue , "." , identifier    (* member access *)
               | lvalue , "[" , expression , "]" ;  (* index access *)

if_stmt        = "if" , expression , "do" , block ,
                 { "elif" , expression , "do" , block } ,
                 [ "else" , [ "do" ] , block ] ,
                 "end" ;

while_stmt     = "while" , expression , "do" , block , "end" ;

for_stmt       = "for" , identifier , "in" , expression , ".." , expression , "do" , block , "end" ;

return_stmt    = "ret" , [ expression ] ;

expression_stmt = expression ;
```

### Expressions

```ebnf
expression     = or_expr ;

or_expr        = and_expr , { "or" , and_expr } ;
and_expr       = bitor_expr , { "and" , bitor_expr } ;
bitor_expr     = bitxor_expr , { "|" , bitxor_expr } ;
bitxor_expr    = bitand_expr , { "^" , bitand_expr } ;
bitand_expr    = equality_expr , { "&" , equality_expr } ;
equality_expr  = compare_expr , { ( "==" | "!=" ) , compare_expr } ;
compare_expr   = shift_expr , { ( "<" | "<=" | ">" | ">=" ) , shift_expr } ;
shift_expr     = add_expr , { ( "<<" | ">>" ) , add_expr } ;
add_expr       = mul_expr , { ( "+" | "-" ) , mul_expr } ;
mul_expr       = unary_expr , { ( "*" | "/" | "%" ) , unary_expr } ;

unary_expr     = [ "-" | "!" | "&" | "*" ] , postfix_expr ;

postfix_expr   = primary_expr , { postfix_op } ;
postfix_op     = "(" , [ arg_list ] , ")"        (* function call *)
               | "." , identifier                 (* member access *)
               | "[" , expression , "]" ;        (* index access *)

arg_list       = expression , { "," , expression } ;

primary_expr   = int_literal
               | float_literal
               | string_literal
               | bool_literal
               | identifier
               | struct_init
               | array_init
               | "(" , expression , ")" ;

struct_init    = identifier , "{" , [ field_init_list ] , "}" ;
field_init_list = field_init , { "," , field_init } ;
field_init     = identifier , "=" , expression ;

array_init     = "[" , [ expression , { "," , expression } ] , "]" ;
```

### Operator Precedence (lowest to highest)

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| 1 | `or` | Left |
| 2 | `and` | Left |
| 3 | `\|` (bitor) | Left |
| 4 | `^` (bitxor) | Left |
| 5 | `&` (bitand) | Left |
| 6 | `==` `!=` | Left |
| 7 | `<` `<=` `>` `>=` | Left |
| 8 | `<<` `>>` | Left |
| 9 | `+` `-` | Left |
| 10 | `*` `/` `%` | Left |
| 11 | `-` `!` `&` `*` (unary) | Right |
| 12 | `()` `.` `[]` (postfix) | Left |

### Keywords

```
and      do       elif     else     end      false    fn       for
if       in       let      mut      or       ptr      ret      struct
true     while
```

### Reserved for Future Use

```
break    continue enum     import   match    module   pub
return   self     trait    type     use
```
