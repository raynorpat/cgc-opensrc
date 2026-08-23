# Aggregate Uniform Default Conversion Design

## Goal

Make strict GLSL 1.10 uniform-default metadata preserve Cg numeric conversion
semantics for scalar, vector, matrix, and fixed-array aggregate leaves without
changing generic-profile folding or shader-expression lowering.

## Typed evaluation

The GLSL default flattener will retain both the Cg base and the
`scalar_constant` payload for every component.  Constant nodes establish the
source base.  Aggregate lists and `VECTOR_V_OP` retain source order.
`CAST_CS_OP`, `CAST_CV_OP`, and `CAST_CM_OP` recursively evaluate their operand
once, validate the cast width encoded in the subopcode, and convert exactly the
components produced by that operand.  Nested casts therefore remain ordered
instead of being collapsed to the final destination type.

After expression casts have been applied, a recursive walk of the emitted
`GlslType` supplies the declared base of every flattened leaf.  Each typed
component is normalized to that base before it is stored in the existing float
metadata array.  This final pass both checks the AST/type correspondence and
covers aggregate initializer forms whose implicit destination conversion is
not retained as an explicit cast node.

## Conversion and validation rules

Floating-to-integer conversion truncates toward zero, matching Cg and C for
finite in-range values.  Values below `INT_MIN` or above `INT_MAX` are rejected
before a C integer cast; the upper check accounts for float rounding at
`INT_MAX`.  Numeric-to-boolean conversion stores exactly zero or one according
to whether the source is zero.  Boolean-to-numeric conversion also produces
zero or one.  Integer-to-floating conversion uses the compiler's existing
single-precision metadata representation.

Every floating source is checked for NaN, infinity, and range before use.  An
unsupported base, malformed cast width, component mismatch, or unsafe
conversion produces one source-located profile diagnostic before module
writing.  Capacity and multiplication checks remain subtraction- or
division-based so malformed ASTs cannot overrun arena-backed storage.

## Isolation and verification

Only `glsl_lower.c` changes behavior.  Generic constant folding and binding
metadata remain unchanged.  Exact fixtures compare scalar and aggregate forms
for float-to-int, numeric-to-bool, and int-to-float conversions, exercise
scalar/vector/matrix cast nodes where accepted by the front end, and include
clean finite/range failures.  Successful outputs pass `glslangValidator`; the
full Debug and Release suites, generic hashes, bundled shaders, stdlib/parser
reproducibility, and diff hygiene remain required gates.
