#include <metal_stdlib>
using namespace metal;
struct Input { float4 p [[attribute(0)]]; };
struct Varyings {
    float4 p [[position]];
    float2 uv [[user(cg_TEXCOORD0)]];
};
vertex Varyings probe_v(Input x [[stage_in]]) {
    Varyings r; r.p = x.p; r.uv = x.p.xy; return r;
}
fragment float4 probe_f(Varyings x [[stage_in]]) {
    return float4(x.uv, 0.0f, 1.0f);
}
