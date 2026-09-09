# DirectX 9 HLSL Shader Model 3 compatibility

The `hlslv` and `hlslf` profiles translate the repository's Cg 2.0 source
language to standalone HLSL for `vs_3_0` and `ps_3_0`. This matrix is the
closed compatibility contract. `native` means the construct is represented
directly in HLSL, `legalized` means the compiler rewrites it while preserving
the supported behavior, and `vertex` or `pixel` means the construct is legal
only in that stage. A rejection names the exact diagnostic asserted by the
test in the final column.

The inventory was produced from the grammar productions in `parser.y`, the
token and predefined-type registries in `tokens.h`, `cg_types.c`, and
`symbols.c`, the binding and default paths in `compile.c` and `binding.c`, the
standard-library catalog in `src/cg_stdlib.def`, `src/cg_stdlib.c`, and `src/stdlib.cg`,
and the HLSL semantic, intrinsic, texture, register, and validation tables.
Scanner tokens that are reserved but have no grammar production are not a
source feature. Rows group spellings only when they share one parser or
table-driven implementation and one compatibility result.

| Category | Feature | Status | Test |
|---|---|---|---|
| Target | `hlslv` profile registration and `vs_3_0` target | vertex | `hlslv_registration` |
| Target | `hlslf` profile registration and `ps_3_0` target | pixel | `hlslf_registration` |
| Target | generated shader has one public `main` wrapper | native | `hlslv_vp_passthrough` |
| Target | generated pixel shader has one public `main` wrapper | native | `hlslf_fp_passthrough` |
| Target | internal entry is called exactly once | legalized | `hlslv_evaluation_order` |
| Target | profile and target metadata comments | native | `hlslv_registration` |
| Type | `void` helper result | native | `hlslv_declaration_forms` |
| Type | `float` and `float1` through `float4` | native | `hlslv_declaration_forms` |
| Type | `bool` and `bool1` through `bool4` | native | `hlslv_declaration_forms` |
| Type | `int` and `int1` through `int4` | native | `hlslv_declaration_forms` |
| Type | `cfloat` and `cfloat1` through `cfloat4` | legalized | `hlslv_declaration_forms` |
| Type | `cint` and `cint1` through `cint4` | legalized | `hlslv_declaration_forms` |
| Type | `half` and `half1` through `half4` | legalized | `hlslv_declaration_forms` |
| Type | `fixed` scalar and vector spellings | legalized | `hlslv_declaration_forms` |
| Type | `double` scalar and vector spellings | legalized | `hlslv_declaration_forms` |
| Type | `char`/`uchar` scalar and vector spellings plus scalar `unsigned char` | legalized | `hlslv_declaration_forms` |
| Type | `short`/`ushort` scalar and vector spellings plus scalar `unsigned short` | legalized | `hlslv_declaration_forms` |
| Type | `uint` scalar and vector spellings plus scalar `unsigned` and `unsigned int` | legalized | `hlslv_declaration_forms` |
| Type | `long`/`ulong` scalar and vector spellings plus scalar `unsigned long` | legalized | `hlslv_declaration_forms` |
| Type | `float1x1` through `float4x4` matrices | native | `hlslv_declaration_forms` |
| Type | rectangular matrices are emitted `row_major` | legalized | `hlslv_matrix_order` |
| Type | `sampler1D` | native | `hlslf_texture_1d` |
| Type | `sampler2D` | native | `hlslf_texture_2d` |
| Type | `sampler3D` | native | `hlslf_texture_3d` |
| Type | `samplerCUBE` | native | `hlslf_texture_cube` |
| Type | deprecated base `sampler` | rejected C6400 | `hlslv_diagnostic_unsupported_type` |
| Type | `samplerRECT` | rejected C6409 | `hlslf_texture_sampler_rect` |
| Aggregate | tagged structure declaration and value | native | `hlslv_struct_array` |
| Aggregate | structure forward declaration elision | legalized | `hlslv_declaration_forms` |
| Aggregate | untagged or anonymous structure value | rejected C6400 | `hlslv_diagnostic_untagged_struct` |
| Aggregate | nested structure members | native | `hlslv_nested_out` |
| Aggregate | structure arrays | native | `hlslv_struct_array` |
| Aggregate | sized arrays | native | `hlslv_struct_array` |
| Aggregate | multidimensional sized arrays | native | `hlslv_vp_int_bank_aggregate_shapes` |
| Aggregate | unsized arrays | rejected C6400 | `hlslv_diagnostic_unsized_array` |
| Aggregate | array initializer lists | native | `hlslv_defaults` |
| Aggregate | structure initializer lists flattened into leaf values | legalized | `hlslv_defaults` |
| Aggregate | nested output aggregates | legalized | `hlslv_nested_out` |
| Aggregate | aliased aggregate out arguments | legalized | `hlslv_aliased_out` |
| Aggregate | mixed float, int, and bool structures | legalized | `hlslv_vp_mixed_struct` |
| Aggregate | interface declarations and dispatch | rejected C6400 | `hlslv_diagnostic_interface_dispatch` |
| Aggregate | structures implementing interfaces | rejected C6400 | `hlslv_diagnostic_interface_dispatch` |
| Aggregate | connector-tagged structure declaration | rejected C5001 | `hlslv_diagnostic_connector_tag` |
| Aggregate | ordinary function definition inside a structure | rejected C6401 | `hlslv_diagnostic_struct_method` |
| Aggregate | ordinary function prototype inside a structure | rejected C6401 | `hlslv_diagnostic_struct_method_prototype` |
| Aggregate | `AttribArray` outside a selected geometry entry | rejected C6309 | `cg20_geometry_attrib_array_non_geometry` |
| Qualifier | `const` is lowered to initialized mutable HLSL storage | legalized | `hlslv_declaration_forms` |
| Qualifier | `packed` | legalized | `hlslv_declaration_forms` |
| Qualifier | explicit `uniform` domain | native | `hlslv_vp_passthrough` |
| Qualifier | explicit `varying` domain | native | `hlslv_declaration_forms` |
| Qualifier | explicit `in` parameter | native | `hlslv_parameter_shapes` |
| Qualifier | `out` parameter | legalized | `hlslv_vp_out_inout` |
| Qualifier | `inout` parameter | legalized | `hlslv_vp_out_inout` |
| Qualifier | `typedef` aliases are resolved and elided | legalized | `hlslv_declaration_forms` |
| Qualifier | `inline` qualifier is elided from the emitted helper | legalized | `hlslv_inline_helper` |
| Qualifier | source `__internal` function | rejected C5201 | `hlslv_diagnostic_internal_function` |
| Qualifier | profile-qualified declaration | legalized | `hlslv_declaration_forms` |
| Qualifier | `static` variable storage | rejected C6401 | `hlslv_diagnostic_static_storage` |
| Qualifier | `extern` variable storage | rejected C6401 | `hlslv_diagnostic_extern_storage` |
| Qualifier | `static` helper storage | rejected C6401 | `hlslv_diagnostic_static_function` |
| Qualifier | `extern` helper storage | rejected C6401 | `hlslv_diagnostic_extern_function` |
| Qualifier | `static` entry storage | rejected C6401 | `hlslv_diagnostic_static_entry` |
| Qualifier | geometry input modifiers `POINT`, `LINE`, `LINE_ADJ`, `TRIANGLE`, and `TRIANGLE_ADJ` | rejected C6401 | `hlslv_diagnostic_geometry_input_modifier` |
| Qualifier | geometry output modifiers `POINT_OUT`, `LINE_OUT`, and `TRIANGLE_OUT` | rejected C6401 | `hlslv_diagnostic_geometry_output_modifier` |
| Qualifier | declarator annotations | legalized | `hlslv_annotation` |
| Default | scalar entry-parameter default | legalized | `hlslv_defaults` |
| Default | vector entry-parameter default | legalized | `hlslv_defaults` |
| Default | matrix entry-parameter default | legalized | `hlslv_defaults` |
| Default | array entry-parameter default | legalized | `hlslv_defaults` |
| Default | structure entry-parameter default | legalized | `hlslv_defaults` |
| Default | helper default argument | legalized | `hlslv_declaration_forms` |
| Default | profile-qualified overload default | legalized | `hlslv_declaration_forms` |
| Default | `#pragma bind` numeric default | legalized | `hlslv_pragma_bindings` |
| Default | emitted `cgc-default` metadata and HLSL initializer | legalized | `hlslv_pragma_bindings` |
| Default | unqualified entry parameter has varying program domain | native | `hlslv_default_domains` |
| Default | unqualified global has uniform program domain | native | `hlslv_default_domains` |
| Operator | identifier, scoped lookup, and parenthesized primary expressions | native | `hlslv_declaration_forms` |
| Operator | unsuffixed integer and floating literal forms | native | `hlslv_declaration_forms` |
| Operator | integer literal suffixes `t`, `s`, `i`, `l`, `u`, `ut`, `us`, `ui`, and `ul` | legalized | `hlslv_declaration_forms` |
| Operator | floating literal suffixes `h`, `x`, `f`, and `d` | legalized | `hlslv_declaration_forms` |
| Operator | scalar and vector constructors | native | `hlslv_arithmetic` |
| Operator | matrix constructors | native | `hlslv_matrix_arithmetic` |
| Operator | structure constructors | rejected C1066 | `hlslv_diagnostic_struct_constructor` |
| Operator | typedef-array constructors | rejected C1066 | `hlslv_diagnostic_array_constructor` |
| Operator | interface constructors | rejected C1066 | `hlslv_diagnostic_interface_constructor` |
| Operator | sampler constructors | rejected C5502 | `hlslf_diagnostic_sampler_constructor` |
| Operator | member selection, swizzle, and write mask | native | `hlslv_parameter_shapes` |
| Operator | array, vector, and matrix indexing | native | `hlslv_parameter_shapes` |
| Operator | sized array, vector, and matrix `.length` constant fold | legalized | `hlslv_declaration_forms` |
| Operator | helper call | native | `hlslv_helpers` |
| Operator | prefix increment and decrement | legalized | `hlslv_operator_surface` |
| Operator | postfix increment and decrement | legalized | `hlslv_operator_surface` |
| Operator | unary plus and minus | native | `hlslv_arithmetic` |
| Operator | logical not | native | `hlslv_arithmetic` |
| Operator | runtime bitwise not | rejected C6401 | `hlslv_diagnostic_bitwise_not` |
| Operator | constant-folded bitwise not | legalized | `hlslv_operator_surface` |
| Operator | scalar explicit cast | native | `hlslv_arithmetic` |
| Operator | vector explicit cast | native | `hlslv_parameter_shapes` |
| Operator | matrix explicit cast | rejected C6401 | `hlslv_diagnostic_matrix_cast` |
| Operator | structure explicit cast | rejected C6401 | `hlslv_diagnostic_struct_cast` |
| Operator | array explicit cast | rejected C6401 | `hlslv_diagnostic_array_cast` |
| Operator | sampler explicit cast | rejected C1033 | `hlslf_diagnostic_sampler_cast` |
| Operator | multiply, divide, and remainder | native | `hlslv_arithmetic` |
| Operator | add and subtract | native | `hlslv_arithmetic` |
| Operator | runtime left shift | rejected C6401 | `hlslv_diagnostic_shift_operator` |
| Operator | runtime right shift | rejected C6401 | `hlslv_diagnostic_right_shift_operator` |
| Operator | constant-folded left and right shift | legalized | `hlslv_operator_surface` |
| Operator | less, greater, less-equal, and greater-equal | native | `hlslv_arithmetic` |
| Operator | equality and inequality | native | `hlslv_arithmetic` |
| Operator | runtime bitwise and | rejected C6401 | `hlslv_diagnostic_bitwise_and` |
| Operator | runtime bitwise xor | rejected C6401 | `hlslv_diagnostic_bitwise_xor` |
| Operator | runtime bitwise or | rejected C6401 | `hlslv_diagnostic_bitwise_operator` |
| Operator | constant-folded bitwise and, xor, and or | legalized | `hlslv_operator_surface` |
| Operator | logical and and or | legalized | `hlslv_arithmetic` |
| Operator | scalar conditional expression | legalized | `hlslv_scalar_conditional_order` |
| Operator | vector conditional expression | legalized | `hlslv_side_effects` |
| Operator | scalar, vector, and matrix simple assignment | native | `hlslv_parameter_shapes` |
| Operator | array and structure assignment by recursive member copy | legalized | `hlslv_struct_array` |
| Operator | `+=`, `-=`, `*=`, `/=`, and `%=` compound assignment | legalized | `hlslv_operator_surface` |
| Operator | side-effecting nested expressions preserve source order | legalized | `hlslv_evaluation_order` |
| Statement | declaration statement | native | `hlslv_declaration_forms` |
| Statement | expression statement | native | `hlslv_arithmetic` |
| Statement | empty statement | native | `hlslv_empty_control` |
| Statement | compound block | native | `hlslv_control` |
| Statement | `if` without `else` | native | `hlslv_control` |
| Statement | `if` with `else` and dangling-else association | native | `hlslv_empty_control` |
| Statement | `for` loop | native | `hlslv_control` |
| Statement | comma-separated `for` initializer and step expressions | legalized | `hlslv_loop_prefixes` |
| Statement | `while` loop | native | `hlslv_control` |
| Statement | `do while` loop | native | `hlslv_control` |
| Statement | `break` | native | `hlslv_control` |
| Statement | `continue` | native | `hlslv_control` |
| Statement | `return` with value | native | `hlslv_helpers` |
| Statement | `return` without value | native | `hlslv_declaration_forms` |
| Statement | bare `discard` | pixel | `hlslf_discard` |
| Statement | scalar and vector conditional `discard` | pixel | `hlslf_vector_discard` |
| Statement | `discard` in vertex stage | rejected C6402 | `hlslv_discard` |
| Function | prototype followed by definition | native | `hlslv_helpers` |
| Function | overloaded helper functions | native | `hlslv_helpers` |
| Function | scalar, vector, matrix, array, and structure parameters | native | `hlslv_parameter_shapes` |
| Function | sampler helper parameter | native | `hlslf_texture_sampler_helper_parameter` |
| Function | `out` and `inout` copy-in or copy-out behavior | legalized | `hlslv_evaluation_order` |
| Function | default parameters | legalized | `hlslv_declaration_forms` |
| Function | profile-qualified overload resolution | legalized | `hlslv_declaration_forms` |
| Function | inline helper body | native | `hlslv_inline_helper` |
| Function | direct recursion | rejected C6401 | `hlslv_recursion` |
| Function | mutual and inline recursion | rejected C6401 | `hlslv_inline_mutual_recursion` |
| Function | user helper sharing an intrinsic spelling | native | `hlslv_intrinsic_user_same_name` |
| Semantic | vertex input `POSITION`, `BLENDWEIGHT`, and `BLENDINDICES` | vertex | `hlslv_vp_all_semantics` |
| Semantic | vertex input `NORMAL`, `TANGENT`, and `BINORMAL` | vertex | `hlslv_vp_all_semantics` |
| Semantic | vertex input `PSIZE` | vertex | `hlslv_vp_all_semantics` |
| Semantic | vertex input `TEXCOORD0` through `TEXCOORD15` | vertex | `hlsl_semantics_unit` |
| Semantic | vertex input `COLOR0` and `COLOR1` | vertex | `hlslv_vp_all_semantics` |
| Semantic | vertex output required `POSITION0` | vertex | `hlslv_vp_all_semantics` |
| Semantic | vertex output `PSIZE0` and `FOG0` | vertex | `hlslv_vp_all_semantics` |
| Semantic | vertex output `COLOR0` and `COLOR1` | vertex | `hlslv_vp_all_semantics` |
| Semantic | vertex output `TEXCOORD0` through `TEXCOORD7` | vertex | `hlsl_semantics_unit` |
| Semantic | vertex aliases `HPOS`, `COL`, `TEX`, `ATTR`, and `ATTRIB` | legalized | `hlsl_semantics_unit` |
| Semantic | pixel input `COLOR0` and `COLOR1` | pixel | `hlslf_fp_all_semantics` |
| Semantic | pixel input `TEXCOORD0` through `TEXCOORD7` and `FOG0` | pixel | `hlsl_semantics_unit` |
| Semantic | pixel input `VPOS` and `VFACE` | pixel | `hlsl_semantics_unit` |
| Semantic | pixel input aliases `WPOS`, `FACE`, `COL`, and `TEX` | legalized | `hlsl_semantics_unit` |
| Semantic | pixel output `COLOR0` through `COLOR3` | pixel | `hlslf_limit_colors4` |
| Semantic | pixel output `DEPTH0` | pixel | `hlslf_fp_all_semantics` |
| Semantic | pixel output aliases `COL0` through `COL3` | legalized | `hlsl_semantics_unit` |
| Semantic | semantic names are case-insensitive | legalized | `hlslv_interface_case_output_conflict` |
| Semantic | duplicate or aliasing output semantic | rejected C6404 | `hlslv_interface_alias_output_conflict` |
| Semantic | unknown or direction-invalid semantic | rejected C6403 | `hlslv_interface_invalid_semantic` |
| Semantic | wrong semantic width or base type | rejected C6403 | `hlslv_interface_invalid_width` |
| Semantic | missing vertex position | rejected C6405 | `hlslv_diagnostic_missing_position` |
| Binding | semantic `C#` float-constant register | native | `hlslv_matrix_order` |
| Binding | semantic `I#` integer-constant register | native | `hlslv_vp_int_bank_aggregate_shapes` |
| Binding | semantic `B#` boolean-constant register | native | `hlslv_vp_bool_bank_shapes` |
| Binding | semantic `S#` sampler register | native | `hlslf_fp_samplers` |
| Binding | semantic `TEXUNIT#` sampler register | native | `hlslf_texture_2d` |
| Binding | `#pragma bind` register array form | native | `hlslv_pragma_bindings` |
| Binding | `#pragma bind` texture-unit form | native | `hlslf_pragma_texunit` |
| Binding | `#pragma bind` default form | native | `hlslv_pragma_bindings` |
| Binding | `#pragma bind` constant form | rejected C5040 | `hlslv_diagnostic_pragma_constant` |
| Binding | `#pragma bind` connector form | rejected C5040 | `hlslv_diagnostic_pragma_connector` |
| Binding | `#pragma bind` float value to `i#` mismatch | rejected C5040 | `hlslv_diagnostic_pragma_float_to_int_bank` |
| Binding | `#pragma bind` integer value to `c#` mismatch | rejected C5040 | `hlslv_diagnostic_pragma_int_to_float_bank` |
| Binding | `#pragma bind` boolean value to `s#` mismatch | rejected C5040 | `hlslv_diagnostic_pragma_bool_to_sampler_bank` |
| Binding | `#pragma bind` homogeneous float structure array to `i#` mismatch | rejected C5040 | `hlslv_diagnostic_pragma_float_struct_array_to_int_bank` |
| Binding | `#pragma bind` homogeneous integer structure to `c#` mismatch | rejected C5040 | `hlslv_diagnostic_pragma_int_struct_to_float_bank` |
| Binding | unbound uniforms allocate by source order and first fit | legalized | `hlslv_vp_register_banks` |
| Binding | explicit ranges reserve before implicit allocation | legalized | `hlslv_vp_int_bank_aggregate_shapes` |
| Binding | homogeneous arrays and structures reserve contiguous spans | legalized | `hlslv_vp_int_bank_aggregate_shapes` |
| Binding | mixed-bank structures split into leaf bindings | legalized | `hlslv_vp_mixed_struct` |
| Binding | colliding explicit ranges | rejected C6407 | `hlslv_binding_register_collision` |
| Binding | integer-bank arithmetic data path | rejected C6402 | `hlslv_diagnostic_int_bank_arithmetic` |
| Binding | integer-bank cast data path | rejected C6402 | `hlslv_diagnostic_int_bank_cast` |
| Binding | integer-bank assignment data path | rejected C6402 | `hlslv_diagnostic_int_bank_copy` |
| Binding | integer-bank indexed write | rejected C6402 | `hlslv_diagnostic_int_bank_index_write` |
| Binding | integer-bank indexed read | rejected C6402 | `hlslv_diagnostic_int_bank_index_read` |
| Binding | integer-bank helper argument | rejected C6402 | `hlslv_diagnostic_int_bank_helper` |
| Binding | integer-bank helper return | rejected C6402 | `hlslv_diagnostic_int_bank_helper_return` |
| Binding | integer-bank bitwise data path | rejected C6402 | `hlslv_diagnostic_int_bank_bitwise` |
| Binding | integer-bank aggregate helper use | rejected C6402 | `hlslv_diagnostic_int_bank_array_helper_use` |
| Binding | boolean-bank logical value data path | rejected C6402 | `hlslv_diagnostic_bool_bank_logical_value` |
| Binding | boolean-bank conditional value data path | rejected C6402 | `hlslv_diagnostic_bool_bank_conditional_value` |
| Binding | boolean-bank helper argument | rejected C6402 | `hlslv_diagnostic_bool_bank_helper` |
| Binding | boolean-bank helper return | rejected C6402 | `hlslv_diagnostic_bool_bank_helper_return` |
| Binding | boolean-bank comparison control path | rejected C6402 | `hlslv_diagnostic_bool_bank_equal_control` |
| Binding | boolean-bank comparison value path | rejected C6402 | `hlslv_diagnostic_bool_bank_equal_value` |
| Binding | boolean-bank loop condition selection | rejected C6402 | `hlslv_diagnostic_bool_bank_loop_condition_selection` |
| Binding | boolean-bank loop step selection | rejected C6402 | `hlslv_diagnostic_bool_bank_loop_step_selection` |
| Binding | mixed-bank helper argument | rejected C6402 | `hlslv_diagnostic_mixed_bank_helper_argument` |
| Binding | mixed-bank helper return | rejected C6402 | `hlslv_diagnostic_mixed_bank_helper_return_int` |
| Intrinsic | `mul`, dot products, and matrix-vector products | native | `hlslv_intrinsic_geometric` |
| Intrinsic | `cross`, `normalize`, `reflect`, `refract`, `length`, and `distance` | native | `hlslv_intrinsic_geometric` |
| Intrinsic | `min`, `max`, `clamp`, `lerp`, `step`, and `smoothstep` | native | `hlslv_intrinsic_numeric` |
| Intrinsic | `abs`, `sign`, `floor`, `ceil`, `round`, `trunc`, and `frac` | native | `hlslv_intrinsic_numeric` |
| Intrinsic | `saturate` expansion | legalized | `hlslv_intrinsic_numeric` |
| Intrinsic | `sqrt`, `pow`, `exp`, `exp2`, `log`, and `log2` | native | `hlslv_intrinsic_numeric` |
| Intrinsic | `rsqrt` helper lowering | legalized | `hlslv_intrinsic_numeric` |
| Intrinsic | `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, and `atan2` | native | `hlslv_intrinsic_numeric` |
| Intrinsic | `sinh`, `cosh`, and `tanh` | native | `hlslv_intrinsic_numeric` |
| Intrinsic | `fmod` | native | `hlslv_intrinsic_numeric` |
| Intrinsic | `any` and `all` scalar or vector reductions | native | `hlslv_intrinsic_numeric` |
| Intrinsic | `ddx` and `ddy` | pixel | `hlslf_intrinsic_derivatives` |
| Intrinsic | derivatives in vertex stage | rejected C6402 | `hlslv_intrinsic_derivative_stage` |
| Intrinsic | `degrees` and `radians` | rejected C6410 | `hlslv_diagnostic_intrinsic_angle` |
| Intrinsic | `determinant` and `transpose` | rejected C6410 | `hlslv_diagnostic_intrinsic_matrix` |
| Intrinsic | `log10` | rejected C6410 | `hlslv_diagnostic_intrinsic_log10` |
| Intrinsic | `frexp`, `ldexp`, and `sincos` decomposition family | rejected C6410 | `hlslv_diagnostic_intrinsic_decompose` |
| Intrinsic | `modf` and other unsupported out-parameter forms | rejected C6410 | `hlslv_diagnostic_intrinsic_outparam` |
| Intrinsic | `isfinite`, `isinf`, and `isnan` classification family | rejected C6410 | `hlslv_diagnostic_intrinsic_classify` |
| Intrinsic | `lit`, `noise`, `debug`, and `faceforward` miscellaneous family | rejected C6410 | `hlslv_diagnostic_intrinsic_misc` |
| Intrinsic | geometry `emitVertex`, `flatAttrib`, and `restartStrip` | rejected C6410 | `hlslv_diagnostic_geometry_entry` |
| Intrinsic | unsupported intrinsic overload or scalar kind | rejected C6410 | `hlslv_intrinsic_bad_overload` |
| Texture | implicit 1D sample | pixel | `hlslf_texture_1d` |
| Texture | implicit 2D sample | pixel | `hlslf_texture_2d` |
| Texture | implicit 3D sample | pixel | `hlslf_texture_3d` |
| Texture | implicit cube sample | pixel | `hlslf_texture_cube` |
| Texture | projected 1D, 2D, 3D, and cube forms | pixel | `hlslf_all_texture_forms` |
| Texture | bias 1D, 2D, 3D, and cube forms | pixel | `hlslf_all_texture_forms` |
| Texture | explicit LOD 1D, 2D, 3D, and cube forms | native | `hlslf_all_texture_forms` |
| Texture | explicit gradient 1D, 2D, 3D, and cube forms | pixel | `hlslf_all_texture_forms` |
| Texture | vertex explicit LOD for all four sampler dimensions | vertex | `hlslv_all_texture_lod` |
| Texture | vertex implicit sampling | rejected C6409 | `hlslv_texture_implicit_stage` |
| Texture | non-rectangle `h4tex` and `x4tex` precision aliases | legalized | `hlslf_all_texture_forms` |
| Texture | rectangle `texRECT` | rejected C6409 | `hlslf_texture_rect` |
| Texture | rectangle `texRECTproj` | rejected C6409 | `hlslf_texture_rect_proj` |
| Texture | rectangle `h4texRECT` | rejected C6409 | `hlslf_texture_rect_alias` |
| Texture | rectangle `x4texRECT` | rejected C6409 | `hlslf_texture_rect_x4` |
| Texture | rectangle `h4texRECTproj` | rejected C6409 | `hlslf_texture_rect_h4_proj` |
| Texture | rectangle `x4texRECTproj` | rejected C6409 | `hlslf_texture_rect_x4_proj` |
| Texture | sampler coordinate shape mismatch | rejected C6409 | `hlslf_texture_sampler_coord_mismatch` |
| Texture | bias or LOD coordinate shape mismatch | rejected C6409 | `hlslf_texture_bias_coord_mismatch` |
| Texture | sampler arrays | rejected C6409 | `hlslf_texture_sampler_array` |
| Texture | local sampler values | rejected C1151 | `hlslf_texture_local_sampler` |
| Texture | conflicting sampler dimensions on one unit | rejected C6409 | `hlslf_sampler_unit_type_conflict` |
| Resource | vertex input limit 16 | vertex | `hlslv_limit_inputs16` |
| Resource | vertex input one-over limit | rejected C6408 | `hlslv_limit_inputs17` |
| Resource | vertex output limit 12 | vertex | `hlslv_limit_outputs12` |
| Resource | vertex output one-over limit | rejected C6408 | `hlslv_limit_outputs13` |
| Resource | pixel input limit 10 | pixel | `hlslf_limit_inputs10` |
| Resource | pixel input one-over limit | rejected C6408 | `hlslf_limit_inputs11` |
| Resource | pixel total output limit 5 | pixel | `hlslf_fp_all_semantics` |
| Resource | pixel total output one-over limit | rejected C6408 | `hlslf_limit_outputs6` |
| Resource | vertex float constant limit 256 | vertex | `hlslv_limit_constants256` |
| Resource | vertex float constant one-over limit | rejected C6408 | `hlslv_limit_constants257` |
| Resource | pixel float constant limit 224 | pixel | `hlslf_limit_constants224` |
| Resource | pixel float constant one-over limit | rejected C6408 | `hlslf_limit_constants225` |
| Resource | integer constant limit 16 | native | `hlslv_limit_int16` |
| Resource | integer constant one-over limit | rejected C6408 | `hlslv_limit_int17` |
| Resource | boolean constant limit 16 | native | `hlslv_limit_bool16` |
| Resource | boolean constant one-over limit | rejected C6408 | `hlslv_limit_bool17` |
| Resource | vertex sampler limit 4 | vertex | `hlslv_limit_samplers4` |
| Resource | vertex sampler one-over limit | rejected C6408 | `hlslv_limit_samplers5` |
| Resource | pixel sampler limit 16 | pixel | `hlslf_limit_samplers16` |
| Resource | pixel sampler one-over limit | rejected C6408 | `hlslf_limit_samplers17` |
| Resource | pixel color output limit 4 | pixel | `hlslf_limit_colors4` |
| Resource | fifth pixel color output | rejected C6408 | `hlslf_limit_colors5` |
| Resource | sparse pixel color output | rejected C6408 | `hlslf_limit_sparse_color4` |
| Validation | entry interface must be representable by a public wrapper | rejected C6406 | `hlslv_diagnostic_entry_abi` |
| Validation | vertex and pixel interface link agreement | native | `hlsl_link_interface` |
| Validation | cross-stage semantic or type mismatch detection | native | `hlsl_link_interface_mismatch` |
| Validation | exact HLSL golden comparison | native | `hlslv_position` |
| Validation | optional positive external compiler contract | native | `hlsl_positive_oracle_contract_unit` |
| Validation | metadata direction normalization | native | `hlsl_metadata_direction_unit` |
| Validation | fresh-directory compilation and link artifacts | native | `hlsl_link_fresh_directory` |
| Validation | cleanup remains inside the configured build root | native | `hlsl_fresh_directory_cleanup_safety` |
| Validation | generated-name collision avoidance | legalized | `hlslv_global_entry_name_collision` |
| Validation | matrix vocabulary, category, and registered-test audit | native | `hlsl_compatibility_matrix` |
