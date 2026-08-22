# Matrix Constructor Evaluation Design

## Scope

Support valid Cg matrix constructors whose scalar or vector arguments have
side effects while preserving strict GLSL 1.10 output. Each original
constructor argument must occur exactly once at the original expression site.
The translation does not promise a stronger left-to-right argument evaluation
order than Cg and GLSL specify.

Pure constructors retain the existing direct, readable `matN(...)` output.
This change does not add fragment translation, compatibility extensions, or
new matrix shapes.

## Lowering

When any source matrix-constructor argument has side effects, lower the
original arguments once in source order as the argument list of a synthesized
GLSL function call. The synthesized function accepts the original mapped
argument shapes and returns the square matrix. Its body references only its
pure parameters, flattens vector parameters in source row order, and emits the
matrix constructor in GLSL column-major order.

Keep synthesized helpers in a lowering-owned list until source helpers and the
entry function have been lowered. Emit synthesized helpers before source
functions so callers do not require prototypes. The solution remains valid in
assignment, return, conditional, and loop expression positions because no
statement-level temporary is hoisted.

## Identity and Naming

Deduplicate synthesized helpers by:

- square matrix dimension and return type; and
- the complete ordered mapped parameter types, including base, vector shape,
  matrix shape, array shape, and struct identity where applicable.

Task 7 constructor arguments are scalar or vector numeric types. Reject array,
matrix, struct, sampler, or otherwise invalid parameter shapes before helper
creation.

Mapped floating-point and integer scalar/vector arguments are numeric. Preserve
their base and width in synthesized helper signatures and parameter
declarations. Strict GLSL 1.10 matrix constructors perform the final numeric
conversion to their floating matrix element type; all representative scalar
and vector forms must pass `glslangValidator`. Boolean arguments are
nonnumeric for the practical Task 7 subset and remain explicitly unsupported,
even when the common Cg front end accepts an all-boolean constructor list.

Build a deterministic readable base name from the matrix dimension and
parameter shapes. Allocate the final function name through the module's shared
namespace so user functions, types, globals, and generated interfaces cannot
collide. Emit one definition per signature.

## Validation

Add an exact vertex fixture covering:

- two side-effecting `mat2` constructors sharing one synthesized signature;
- a user helper colliding with the generated base name;
- a mixed scalar, `vec2`, and `vec3` side-effecting `mat3` constructor;
- pure and side-effecting integer scalar/vector matrix arguments, including
  `mat2`, `mat3`, and `mat4` coverage;
- a matrix constructor in a helper return that is invoked only inside a
  conditional branch; and
- exactly one textual occurrence of each original side-effecting call per
  constructor expression.

The exact output must pass `glslangValidator`. Existing pure matrix constructor
goldens must remain unchanged. Full Debug and Release tests, frozen generic
hashes, standard-library regeneration, and whitespace checks remain required.
