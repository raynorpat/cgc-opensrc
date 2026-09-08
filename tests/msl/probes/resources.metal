#include <metal_stdlib>
using namespace metal;
struct Uniforms { float4 tint; float4 params; };
struct Input { float4 p [[attribute(0)]]; };
struct Varyings {
    float4 p [[position]];
    float2 uv [[user(cg_TEXCOORD0)]];
    int index [[user(cg_TEXCOORD1), flat]];
};
struct Output { float4 color [[color(0)]]; float depth [[depth(any)]]; };
void adjust(thread float &value) { value += 0.25f; }
float4 sample_2d(texture2d<float> image, sampler state, float2 uv) {
    return image.sample(state, uv);
}
float4 sample_cube(texturecube<float> image, sampler state, float3 uv,
                   constant Uniforms &u) {
    return image.sample(state, uv, level(u.params.x));
}
vertex Varyings resource_v(Input x [[stage_in]],
                          constant Uniforms &u [[buffer(0)]],
                          texture2d<float> image [[texture(0)]],
                          sampler state [[sampler(0)]]) {
    Varyings r;
    r.p = x.p + image.sample(state, x.p.xy, level(u.params.x));
    r.uv = x.p.xy; r.index = int(u.params.y); return r;
}
fragment Output resource_f(Varyings x [[stage_in]],
                           constant Uniforms &u [[buffer(0)]],
                           texture2d<float> image [[texture(0)]],
                           sampler state [[sampler(0)]]) {
    Output r; float depth = u.params.z; adjust(depth);
    r.color = sample_2d(image, state, x.uv) * u.tint + float(x.index);
    r.depth = depth; return r;
}
fragment float4 cube_f(Varyings x [[stage_in]],
                       constant Uniforms &u [[buffer(0)]],
                       texturecube<float> image [[texture(0)]],
                       sampler state [[sampler(0)]]) {
    return sample_cube(image, state, float3(x.uv, 1.0f), u);
}
