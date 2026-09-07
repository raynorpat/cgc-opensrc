# DirectX 10 and 11 HLSL compatibility

The `hlslv40`, `hlslg40`, and `hlslf40` profiles emit source for Shader
Model 4.0. The corresponding `50` profiles emit Shader Model 5.0 source.
This is a source compiler: it emits one public `main`; `fxc` remains an
optional external validator and bytecode compiler.

The status vocabulary is closed. `native` maps directly to HLSL,
`legalized` names a semantics-preserving rewrite, `stage/model-specific`
means availability depends on the selected stage or model, `rejected C####`
names the exact compiler diagnostic, and `not exposed` means the current Cg
2.0 frontend has no source construct for the feature. The final column names
a registered test. Grouped spellings share one frontend or table-driven
implementation and one result.

| Category | Feature | Status | Test |
|---|---|---|---|
| Target | `hlslv40` emits `vs_4_0` | stage/model-specific | `hlslv40_registration` |
| Target | `hlslg40` emits `gs_4_0` | stage/model-specific | `hlslg40_registration_output` |
| Target | `hlslf40` emits `ps_4_0` | stage/model-specific | `hlslf40_registration` |
| Target | `hlslv50` emits `vs_5_0` | stage/model-specific | `hlslv50_registration` |
| Target | `hlslg50` emits `gs_5_0` | stage/model-specific | `hlslg50_registration_output` |
| Target | `hlslf50` emits `ps_5_0` | stage/model-specific | `hlslf50_registration` |
| Target | one public `main` wrapper | native | `modern_position_v40` |
| Target | one lowered internal entry | legalized | `modern_language_v50` |
| Target | profile, target, and entry metadata | native | `modern_semantic_v40` |
| Target | mixed SM4/SM5 pipeline is detected before external compilation | native | `hlsl_link_mixed_modern_geometry_model` |
| Type | `void` helper result | native | `modern_language_v40` |
| Type | `float` and `float1` through `float4` | native | `modern_language_v40` |
| Type | `bool` and `bool1` through `bool4` | native | `modern_language_v40` |
| Type | `int` and `int1` through `int4` | native | `modern_language_v40` |
| Type | `uint` source spellings and vectors | native | `modern_bitwise_v40` |
| Type | `cfloat` source spellings | legalized | `modern_compile_time_p40` |
| Type | `cint` source spellings | legalized | `modern_compile_time_p40` |
| Type | `half` source spellings use portable promotion | legalized | `modern_uniform_v40` |
| Type | `fixed` source spellings use portable promotion | legalized | `modern_scalar_types_v40` |
| Type | `double` source spellings do not create an SM5-only contract | legalized | `modern_scalar_types_v40` |
| Type | `char` and `uchar` source spellings | legalized | `modern_scalar_types_v40` |
| Type | `short` and `ushort` source spellings | legalized | `modern_scalar_types_v40` |
| Type | `long` and `ulong` source spellings | legalized | `modern_scalar_types_v40` |
| Type | scalar `unsigned` aliases | legalized | `modern_scalar_types_v40` |
| Type | square float matrices | native | `modern_uniform_v40` |
| Type | rectangular matrices | native | `modern_reflection_v40` |
| Type | explicit `row_major` matrix policy | legalized | `modern_reflection_v50` |
| Type | `sampler1D`, `sampler2D`, `sampler3D`, and `samplerCUBE` | legalized | `modern_texture_p40` |
| Type | deprecated base `sampler` | rejected C6400 | `modern_unsupported_type_v40` |
| Type | `samplerRECT` | rejected C6409 | `hlslf_texture_sampler_rect` |
| Type | comparison sampler object | not exposed | `cg20_manifest` |
| Type | multisample texture object | not exposed | `cg20_manifest` |
| Aggregate | tagged structure declaration and value | native | `modern_language_v40` |
| Aggregate | structure forward declaration | legalized | `modern_declaration_forms_v40` |
| Aggregate | structure implements interface | rejected C6400 | `modern_interface_implementation_v40` |
| Aggregate | connector-tagged structure declaration | rejected C5001 | `modern_connector_tag_v40` |
| Aggregate | nested structure members | native | `hlslg40_attrib_array_nested` |
| Aggregate | sized arrays | native | `modern_language_v50` |
| Aggregate | multidimensional sized arrays | native | `modern_multidimensional_array_v40` |
| Aggregate | structure arrays | native | `modern_struct_array_v40` |
| Aggregate | structure and array initialization | legalized | `modern_uniform_v40` |
| Aggregate | nested output aggregates | legalized | `hlslg40_attrib_array_nested` |
| Aggregate | aliased aggregate `out` arguments | legalized | `modern_aliased_out_v40` |
| Aggregate | unsized array value | rejected C6400 | `hlslv_diagnostic_unsized_array` |
| Aggregate | anonymous structure value | rejected C6400 | `hlslv_diagnostic_untagged_struct` |
| Aggregate | interface dispatch | rejected C6400 | `hlslv_diagnostic_interface_dispatch` |
| Aggregate | `AttribArray<T>` | stage/model-specific | `hlslg40_attrib_array_struct` |
| Aggregate | `AttribArray<T>` outside geometry input | rejected C6309 | `modern_attrib_array_stage_v40` |
| Qualifier | `const` storage | legalized | `modern_declaration_forms_v40` |
| Qualifier | `packed` | legalized | `modern_declaration_forms_v40` |
| Qualifier | `uniform` program domain | native | `modern_uniform_v40` |
| Qualifier | `varying` program domain | native | `modern_language_v40` |
| Qualifier | `in` parameter | native | `modern_language_v40` |
| Qualifier | `out` parameter | legalized | `modern_language_p40` |
| Qualifier | `inout` parameter | legalized | `modern_language_v50` |
| Qualifier | `typedef` | legalized | `modern_typedef_v40` |
| Qualifier | `inline` | legalized | `modern_inline_helper_v40` |
| Qualifier | profile-qualified declaration | legalized | `modern_profile_overload_v40` |
| Qualifier | declarator annotation | legalized | `modern_annotation_v40` |
| Qualifier | source `__internal` function | rejected C5201 | `hlslv_diagnostic_internal_function` |
| Qualifier | `static` storage | rejected C6401 | `modern_unsupported_operation_v40` |
| Qualifier | `extern` storage | rejected C6401 | `hlslv_diagnostic_extern_storage` |
| Qualifier | geometry input topology modifier | stage/model-specific | `hlslg40_triangle_topology` |
| Qualifier | geometry output topology modifier | stage/model-specific | `hlslg50_triangle_topology` |
| Default | scalar entry default | legalized | `modern_defaults_v40` |
| Default | vector entry default | legalized | `modern_defaults_v40` |
| Default | matrix entry default | legalized | `modern_uniform_v40` |
| Default | array entry default | legalized | `modern_defaults_v40` |
| Default | structure entry default | legalized | `modern_defaults_v40` |
| Default | helper default argument | legalized | `modern_default_arguments_v40` |
| Default | `#pragma bind` numeric default | legalized | `modern_uniform_v40` |
| Default | `cgc-default` metadata | legalized | `modern_uniform_v50` |
| Operator | identifiers, scoped names, and parentheses | native | `modern_language_v40` |
| Operator | integer and floating literals | native | `modern_literal_suffixes_v40` |
| Operator | integer literal suffix family | legalized | `modern_literal_int_forms_p40` |
| Operator | floating literal suffix family | legalized | `modern_literal_float_forms_p40` |
| Operator | scalar and vector constructors | native | `modern_language_v40` |
| Operator | matrix constructors | native | `modern_uniform_v40` |
| Operator | structure constructor | rejected C1066 | `hlslv_diagnostic_struct_constructor` |
| Operator | array constructor | rejected C1066 | `hlslv_diagnostic_array_constructor` |
| Operator | interface constructor | rejected C1066 | `hlslv_diagnostic_interface_constructor` |
| Operator | sampler constructor | rejected C5502 | `hlslf_diagnostic_sampler_constructor` |
| Operator | member selection and swizzle | native | `modern_language_v40` |
| Operator | array, vector, and matrix indexing | native | `modern_language_v50` |
| Operator | `.length` constant fold | legalized | `modern_array_length_v40` |
| Operator | helper call | native | `modern_language_v40` |
| Operator | prefix increment and decrement | legalized | `modern_language_v40` |
| Operator | postfix increment and decrement | legalized | `modern_language_v50` |
| Operator | unary plus and minus | native | `modern_language_v40` |
| Operator | logical not | native | `modern_language_v40` |
| Operator | bitwise not | native | `modern_bitwise_v40` |
| Operator | scalar and vector cast | native | `modern_language_v50` |
| Operator | explicit matrix cast | rejected C6401 | `modern_matrix_cast_v40` |
| Operator | aggregate cast | rejected C6401 | `hlslv_diagnostic_struct_cast` |
| Operator | sampler cast | rejected C1033 | `hlslf_diagnostic_sampler_cast` |
| Operator | multiply, divide, and remainder | native | `modern_language_v40` |
| Operator | add and subtract | native | `modern_language_v40` |
| Operator | left and right shift | native | `modern_bitwise_v50` |
| Operator | relational operators | native | `modern_language_v40` |
| Operator | equality and inequality | native | `modern_language_v40` |
| Operator | bitwise and, xor, and or | native | `modern_bitwise_p40` |
| Operator | logical and and or | legalized | `modern_language_v40` |
| Operator | scalar conditional expression | legalized | `modern_language_v40` |
| Operator | vector conditional expression | legalized | `hlslg40_vector_conditional` |
| Operator | scalar, vector, and matrix assignment | native | `modern_language_v50` |
| Operator | aggregate assignment | legalized | `hlslg40_aggregate_once` |
| Operator | compound assignment family | legalized | `modern_bitwise_v40` |
| Operator | nested side effects preserve source order | legalized | `hlslg40_ordered_bundle` |
| Statement | declaration | native | `modern_language_v40` |
| Statement | expression | native | `modern_language_v40` |
| Statement | empty statement | native | `modern_empty_control_v40` |
| Statement | compound block | native | `modern_language_v40` |
| Statement | `if` and `if else` | native | `modern_language_v40` |
| Statement | `for` loop | native | `modern_language_v50` |
| Statement | comma-separated `for` expressions | legalized | `modern_loop_prefixes_v40` |
| Statement | `while` loop | native | `modern_language_v40` |
| Statement | `do while` loop | native | `modern_language_v40` |
| Statement | `break` | native | `modern_language_v40` |
| Statement | `continue` | native | `modern_language_v40` |
| Statement | valued `return` | native | `modern_language_v50` |
| Statement | void `return` | native | `modern_language_v40` |
| Statement | scalar and vector `discard` | stage/model-specific | `modern_language_p40` |
| Statement | vertex-stage `discard` | rejected C6402 | `modern_discard_stage_v40` |
| Statement | switch statement | not exposed | `cg20_manifest` |
| Function | prototype and definition | native | `modern_helpers_v40` |
| Function | overloaded helpers | native | `modern_language_v40` |
| Function | scalar, vector, matrix, array, and structure parameters | native | `modern_language_v50` |
| Function | sampler helper parameter | legalized | `modern_texture_p40` |
| Function | `out` and `inout` copy behavior | legalized | `modern_language_v40` |
| Function | default parameters | legalized | `modern_default_arguments_v40` |
| Function | profile-qualified overload resolution | legalized | `modern_profile_overload_v40` |
| Function | direct recursion | rejected C6401 | `hlslv_recursion` |
| Function | mutual recursion | rejected C6401 | `hlslv_inline_mutual_recursion` |
| Function | user helper sharing intrinsic spelling | native | `modern_intrinsic_user_same_name_v40` |
| Semantic | vertex `POSITION` input and output | stage/model-specific | `modern_position_v40` |
| Semantic | pixel `WPOS` or position input maps to `SV_Position` | legalized | `modern_semantic_p40` |
| Semantic | pixel `COLORn` output maps to `SV_Targetn` | legalized | `modern_semantic_p50` |
| Semantic | pixel `DEPTH` output maps to `SV_Depth` | legalized | `modern_semantic_p40` |
| Semantic | vertex `VERTEXID` maps to `SV_VertexID` | stage/model-specific | `modern_semantic_v40` |
| Semantic | vertex `INSTANCEID` maps to `SV_InstanceID` | stage/model-specific | `hlsl_semantics_unit` |
| Semantic | geometry `PRIMITIVEID` maps to `SV_PrimitiveID` | stage/model-specific | `hlslg40_system_values` |
| Semantic | geometry scalar `INSTANCEID` maps to `SV_PrimitiveID` | legalized | `hlslg40_instance_values` |
| Semantic | geometry `LAYER` maps to `SV_RenderTargetArrayIndex` | stage/model-specific | `hlslg50_system_values` |
| Semantic | pixel `FACE` maps to `SV_IsFrontFace` | legalized | `modern_semantic_p50` |
| Semantic | clip distance output maps to `SV_ClipDistance` | legalized | `modern_semantic_v40` |
| Semantic | `PSIZE` rasterizer output | rejected C6412 | `modern_psize` |
| Semantic | `FOG`, `COLORn`, `TEXCOORDn`, and `ATTRn` user varyings | legalized | `modern_semantic_v40` |
| Semantic | tangent, binormal, blend, and ordinary user values | legalized | `hlsl_semantics_unit` |
| Semantic | integral varying gets `nointerpolation` | legalized | `modern_semantic_v40` |
| Semantic | flat geometry output gets `nointerpolation` | legalized | `hlslg40_flat` |
| Semantic | `centroid` interpolation | stage/model-specific | `hlsl_sm4_pipeline_geometry` |
| Semantic | `noperspective` interpolation | stage/model-specific | `hlsl_sm5_pipeline_geometry` |
| Semantic | semantic identity is case-insensitive | legalized | `modern_semantic_case_output_conflict_v40` |
| Semantic | duplicate canonical semantic | rejected C6404 | `modern_system_conflict` |
| Semantic | reserved `SV_` source semantic | rejected C6412 | `modern_reserved_semantic` |
| Semantic | invalid stage-direction semantic | rejected C6403 | `modern_pixel_user_semantic` |
| Semantic | system-value base or width mismatch | rejected C6321 | `hlslg40_geometry_invalid_system_type` |
| Semantic | vertex-ID bridge user semantic | legalized | `hlsl_sm4_pipeline_vertex_id` |
| Semantic | vertex-ID bridge agreement is checked across stages | native | `hlsl_sm5_pipeline_vertex_id` |
| Binding | one generated application cbuffer at `b0` | legalized | `modern_uniform_v40` |
| Binding | explicit `C#` register binding | legalized | `modern_explicit_c_register_v40` |
| Binding | explicit `I#` register binding | rejected C6414 | `modern_explicit_i_register_v40` |
| Binding | explicit `B#` register binding | rejected C6414 | `modern_explicit_b_register_v40` |
| Binding | explicit `S#` register binding | legalized | `modern_explicit_s_register_p40` |
| Binding | `#pragma bind` register-array form | legalized | `modern_pragma_register_array_v40` |
| Binding | deterministic HLSL cbuffer packing | legalized | `modern_uniform_v50` |
| Binding | explicit `packoffset(cN.component)` | legalized | `modern_uniform_v40` |
| Binding | explicit bindings reserve before implicit bindings | legalized | `modern_uniform_v50` |
| Binding | arrays and structures reserve complete spans | legalized | `modern_uniform_v40` |
| Binding | default values preserve logical identity | legalized | `modern_uniform_v50` |
| Binding | cbuffer overlap | rejected C6414 | `modern_packoffset_conflict` |
| Binding | cbuffer vector exhaustion | rejected C6414 | `hlslv40_limit_cbuffer` |
| Binding | sampler becomes same-index `tN` and `sN` pair | legalized | `modern_texture_p40` |
| Binding | `TEXUNITn` reserves both resource slots | legalized | `modern_texture_p50` |
| Binding | implicit sampler pair uses first fit | legalized | `modern_texture_v40` |
| Binding | sampler pair collision | rejected C6415 | `modern_texture_pair_conflict` |
| Binding | sampler pair exhaustion | rejected C6415 | `hlslf50_limit_sampler_pairs` |
| Binding | local sampler object | rejected C1151 | `hlslf_texture_local_sampler` |
| Binding | sampler array | rejected C6409 | `modern_sampler_feature_p40` |
| Binding | multiple generated cbuffers | not exposed | `cg20_manifest` |
| Binding | UAV register binding | not exposed | `cg20_manifest` |
| Binding | resource array binding | not exposed | `cg20_manifest` |
| Binding | explicit compiled-bytecode output | not exposed | `hlsl_validation_exact_target_contract` |
| Intrinsic | arithmetic and geometric intrinsic families | native | `modern_language_v40` |
| Intrinsic | `mul` matrix and vector products | native | `modern_language_v50` |
| Intrinsic | `cross`, `normalize`, `reflect`, and `refract` | native | `modern_language_v40` |
| Intrinsic | `min`, `max`, `clamp`, `lerp`, `step`, and `smoothstep` | native | `modern_language_p40` |
| Intrinsic | rounding and fractional family | native | `modern_language_v40` |
| Intrinsic | exponential and logarithmic family | native | `modern_language_v50` |
| Intrinsic | trigonometric and hyperbolic family | native | `modern_language_p50` |
| Intrinsic | `rsqrt` portable helper | legalized | `modern_language_v40` |
| Intrinsic | `saturate` | native | `modern_language_p40` |
| Intrinsic | `any` and `all` reductions | native | `modern_language_v50` |
| Intrinsic | `ddx` and `ddy` | stage/model-specific | `modern_language_p40` |
| Intrinsic | derivative in vertex stage | rejected C6402 | `modern_derivative_stage_v40` |
| Intrinsic | implicit-derivative sample in geometry stage | rejected C6420 | `hlsl_sm4_pipeline_geometry_texture_stage` |
| Intrinsic | `degrees` and `radians` | rejected C6410 | `modern_intrinsic_v40` |
| Intrinsic | unsupported overload or scalar-kind combination | rejected C6410 | `modern_intrinsic_bad_overload_v40` |
| Intrinsic | `determinant` and `transpose` | rejected C6410 | `hlslv_diagnostic_intrinsic_matrix` |
| Intrinsic | `frexp`, `ldexp`, `sincos`, and `modf` families | rejected C6410 | `hlslv_diagnostic_intrinsic_decompose` |
| Intrinsic | floating classification family | rejected C6410 | `hlslv_diagnostic_intrinsic_classify` |
| Intrinsic | miscellaneous `lit`, `noise`, `debug`, and `faceforward` family | rejected C6410 | `hlslv_diagnostic_intrinsic_misc` |
| Intrinsic | tessellation intrinsics | not exposed | `cg20_manifest` |
| Intrinsic | compute and barrier intrinsics | not exposed | `cg20_manifest` |
| Texture | `sampler1D` lowers to `Texture1D` plus `SamplerState` | legalized | `modern_texture_p40` |
| Texture | `sampler2D` lowers to `Texture2D` plus `SamplerState` | legalized | `modern_texture_p40` |
| Texture | `sampler3D` lowers to `Texture3D` plus `SamplerState` | legalized | `modern_texture_p50` |
| Texture | `samplerCUBE` lowers to `TextureCube` plus `SamplerState` | legalized | `modern_texture_p50` |
| Texture | non-rectangle `h4tex*` and `x4tex*` aliases | legalized | `modern_texture_aliases_p40` |
| Texture | ordinary pixel sample uses `.Sample` | legalized | `modern_texture_p40` |
| Texture | explicit LOD uses `.SampleLevel` | legalized | `modern_texture_v40` |
| Texture | bias uses `.SampleBias` | stage/model-specific | `modern_texture_p50` |
| Texture | explicit gradients use `.SampleGrad` | stage/model-specific | `modern_texture_v50` |
| Texture | projected sample divides coordinates once | legalized | `modern_texture_p40` |
| Texture | geometry explicit-level and gradient samples | stage/model-specific | `hlsl_sm4_pipeline_geometry_textures` |
| Texture | vertex implicit derivative sample | rejected C6420 | `modern_texture_stage` |
| Texture | geometry implicit derivative sample | rejected C6420 | `hlsl_sm5_pipeline_geometry_texture_stage` |
| Texture | dimension mismatch | rejected C6421 | `modern_texture_dimension` |
| Texture | coordinate mismatch | rejected C6421 | `modern_texture_coordinate` |
| Texture | comparison sampling | not exposed | `cg20_manifest` |
| Texture | load, gather, multisample, and resource-array forms | not exposed | `cg20_manifest` |
| Resource | SM4 vertex input register limit 16 | stage/model-specific | `hlslv40_boundary_inputs` |
| Resource | SM5 vertex input register limit 32 | stage/model-specific | `hlslv50_boundary_inputs` |
| Resource | vertex input one-over | rejected C6408 | `hlslv40_limit_inputs` |
| Resource | geometry input limits 16 in SM4 and 32 in SM5 | stage/model-specific | `hlslg50_boundary_inputs` |
| Resource | geometry input one-over | rejected C6408 | `hlslg40_limit_inputs` |
| Resource | geometry output register limit 32 | stage/model-specific | `hlslg50_boundary_outputs` |
| Resource | geometry output one-over | rejected C6408 | `hlslg50_limit_outputs` |
| Resource | pixel input register limit 32 | stage/model-specific | `hlslf40_boundary_inputs` |
| Resource | pixel input one-over | rejected C6408 | `hlslf50_limit_inputs` |
| Resource | eight pixel render targets plus depth | stage/model-specific | `hlslf40_boundary_color_depth` |
| Resource | ninth pixel render target | rejected C6412 | `hlslf50_limit_outputs` |
| Resource | 4096 cbuffer vectors | stage/model-specific | `hlslv50_boundary_cbuffer` |
| Resource | 16 sampler pairs | stage/model-specific | `hlslf40_boundary_sampler_pairs` |
| Resource | 128 shader-resource slots remain sampler-pair constrained | legalized | `hlslf50_boundary_sampler_pairs` |
| Resource | clip-distance component accounting | legalized | `hlslv40_boundary_clip_cull_components` |
| Resource | failed resource translation publishes no target | rejected C6415 | `hlslf40_limit_sampler_pairs` |
| Geometry | `POINT` input topology and extent 1 | stage/model-specific | `hlslg40_point_topology` |
| Geometry | `LINE` input topology and extent 2 | stage/model-specific | `hlslg40_line_topology` |
| Geometry | `LINE_ADJ` input topology and extent 4 | stage/model-specific | `hlslg50_lineadj_topology` |
| Geometry | `TRIANGLE` input topology and extent 3 | stage/model-specific | `hlslg40_triangle_topology` |
| Geometry | `TRIANGLE_ADJ` input topology and extent 6 | stage/model-specific | `hlslg50_triangleadj_topology` |
| Geometry | `POINT_OUT` uses `PointStream` | legalized | `hlslg40_pass_through` |
| Geometry | `LINE_OUT` uses `LineStream` | legalized | `hlslg40_restart` |
| Geometry | `TRIANGLE_OUT` uses `TriangleStream` | legalized | `hlslg50_triangle_topology` |
| Geometry | positive `Vertices=N` becomes `[maxvertexcount(N)]` | legalized | `hlslg40_pass_through` |
| Geometry | missing or zero maximum | rejected C6422 | `hlslg40_geometry_missing_vertices` |
| Geometry | maximum vertex limit 1024 | stage/model-specific | `hlslg50_boundary_max_vertices` |
| Geometry | maximum vertex one-over | rejected C6423 | `hlslg40_limit_max_vertices` |
| Geometry | 1024 total output component budget | stage/model-specific | `hlslg40_boundary_total_output` |
| Geometry | total output component one-over | rejected C6424 | `hlslg50_limit_total_output` |
| Geometry | `AttribArray<T>` reconstruction | legalized | `hlslg40_attrib_array_struct` |
| Geometry | per-primitive scalar system input | legalized | `hlslg40_system_values` |
| Geometry | `emitVertex` becomes complete-record `Append` | legalized | `hlslg40_heterogeneous_emit` |
| Geometry | `flatAttrib` uses path-correct shadow state | legalized | `hlslg50_conditional_flat` |
| Geometry | `restartStrip` becomes `RestartStrip` | legalized | `hlslg40_restart_only` |
| Geometry | geometry side effects preserve source order | legalized | `hlslg50_control_order` |
| Geometry | geometry helpers receive hidden stream state | legalized | `hlslg40_reachable_helper` |
| Geometry | multiple output streams | not exposed | `cg20_manifest` |
| Geometry | geometry invocations | not exposed | `cg20_manifest` |
| Geometry | hull, domain, and compute stages | not exposed | `cg20_manifest` |
| Validation | verified IR gates all source output | native | `hlsl_invariants_unit` |
| Validation | exact SM4 three-stage pipeline | native | `hlsl_sm4_pipeline_geometry` |
| Validation | exact SM5 three-stage pipeline | native | `hlsl_sm5_pipeline_geometry` |
| Validation | VS-to-GS type mismatch is detected | native | `hlsl_sm4_pipeline_vs_gs_type_mismatch` |
| Validation | GS-to-PS type mismatch is detected | native | `hlsl_sm5_pipeline_gs_ps_type_mismatch` |
| Validation | interpolation mismatch is detected | native | `hlsl_sm4_pipeline_gs_ps_interpolation_mismatch` |
| Validation | VS-to-GS semantic mismatch is detected | native | `hlsl_sm4_pipeline_vs_gs_semantic_mismatch` |
| Validation | GS-to-PS semantic mismatch is detected | native | `hlsl_sm5_pipeline_gs_ps_semantic_mismatch` |
| Validation | missing vertex-ID bridge is detected | native | `hlsl_sm4_pipeline_vertex_id_missing` |
| Validation | mismatched vertex-ID bridge is detected | native | `hlsl_sm5_pipeline_vertex_id_bridge_mismatch` |
| Validation | exact profile-to-`fxc` target mapping | native | `hlsl_validation_exact_target_contract` |
| Validation | invalid generated HLSL fails the external harness | native | `hlsl_validation_exact_target_contract` |
| Validation | required `fxc` mode fails clearly when absent | native | `hlsl_require_fxc_missing_sdk` |
| Validation | optional `fxc` mode omits tests when absent | native | `hlsl_optional_fxc_missing_sdk` |
| Validation | deterministic raw file and stdout output | native | `hlsl_modern_raw_output_determinism` |
| Validation | fresh-directory external validation | native | `hlsl_modern_fresh_directory` |
