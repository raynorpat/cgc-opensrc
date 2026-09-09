"""Regenerate the portable Metal coverage corpus; Python is not needed by CTest.
Expected pixels are independent mathematical witnesses, never compiler output.
"""
from pathlib import Path
import re
root = Path(__file__).parent
previous = set(re.findall(r"add_msl_coverage(?:_rejection)?\((\w+)", (root / "cases.cmake").read_text())) if (root / "cases.cmake").exists() else set()
cases = []
created = set()
def add(name, source, pixels=None, profile='mslf', reject=None):
    created.add(name + '.cg')
    (root / (name + '.cg')).write_text(source + '\n')
    if reject:
        cases.append(f'add_msl_coverage_rejection({name} {profile} {reject})')
    else:
        suffix = ' "' + ','.join(str(x) for x in pixels) + '"' if pixels else ''
        cases.append(f'add_msl_coverage({name} {profile}{suffix})')
def body(name, code, pixels):
    add(name, 'float4 main(void) : COLOR0 {\n' + code + '\n}', pixels)
# Every scalar/vector family and width, fed from uniforms to prevent folding.
for kind in ['bool', 'int', 'uint', 'float', 'half', 'fixed']:
    for width in range(1, 5):
        ty = kind + (str(width) if width > 1 else '')
        val = 'v' if width == 1 else 'v.x'
        add(f'type_{ty}', f'float4 main(uniform {ty} v) : COLOR0 {{ return float4(float({val}), 0, 0, 1); }}', [1 if kind == 'bool' else 2,0,0,1])
for ty in ['double','long','short','char','ulong','ushort','uchar','double2','float2x3']:
    add('reject_type_'+ty, f'float4 main(uniform {ty} v) : COLOR0 {{ return float4(float(v[0][0]) ,0,0,1); }}' if 'x' in ty else f'float4 main(uniform {ty} v) : COLOR0 {{ return float4(float(v.x),0,0,1); }}' if ty=='double2' else f'float4 main(uniform {ty} v) : COLOR0 {{ return float4(float(v),0,0,1); }}', reject='C6601')
# Float intrinsic allowlist, all widths. Values avoid singularities/discontinuities.
ops = {
'abs': ('abs(-x)',2), 'min': ('min(x, x+1)',2), 'max': ('max(x,x+1)',3),
'clamp': ('clamp(x,x-1,x+1)',2), 'saturate': ('saturate(x)',1),
'floor': ('floor(x+0.25)',2), 'ceil': ('ceil(x+0.25)',3), 'frac': ('frac(x+0.25)',.25),
'fmod': ('fmod(x, x+1)',2), 'sqrt': ('sqrt(x*x)',2), 'rsqrt': ('rsqrt(x*x)',.5),
'pow': ('pow(x,x)',4), 'exp': ('exp(x-x)',1), 'exp2': ('exp2(x)',4),
'log': ('log(x/x)',0), 'log2': ('log2(x)',1), 'sin': ('sin(x-x)',0),
'cos': ('cos(x-x)',1), 'tan': ('tan(x-x)',0), 'asin': ('asin(x-x)',0),
'acos': ('acos(x/x)',0), 'atan': ('atan(x-x)',0), 'atan2': ('atan2(x-x,x)',0),
'lerp': ('lerp(x,x+2,x/4)',3), 'step': ('step(x-1,x)',1),
'smoothstep': ('smoothstep(x-1,x+1,x)',.5), 'ddx': ('ddx(x)',0), 'ddy': ('ddy(x)',0),
}
for op,(expr,value) in ops.items():
    for n in range(1,5):
        ty = 'float' + (str(n) if n>1 else '')
        add(f'intrinsic_{op}{n}', f'float4 main(uniform float value) : COLOR0 {{ {ty} x=value; {ty} y={expr}; return float4({"y.x" if n>1 else "y"},0,0,1); }}', [value,0,0,1])
for n in range(2,5):
    ty=f'float{n}'
    for op,expr,value in [('dot','dot(x,x)',4*n),('length','length(x)',2*n**.5),('distance','distance(x,x*2)',2*n**.5),('normalize','normalize(x).x',1/n**.5),('any','float(any(x>0))',1),('all','float(all(x>0))',1)]:
        add(f'intrinsic_{op}{n}',f'float4 main(uniform float value):COLOR0 {{ {ty} x=value; return float4({expr},0,0,1); }}',[value,0,0,1])
body('intrinsic_geometry','float3 a=float3(1,0,0), b=float3(0,1,0); return float4(cross(a,b).z, reflect(a,b).x, refract(a,b,0.5).x,1);',[1,1,.5,1])
body('signedness','uint u=4294967295u; int i=int(u); bool2 b=int2(-1,2)<int2(0,1); return float4(i, float(u>>31), float(b.x),float(b.y));',[-1,1,1,0])
body('swizzle_overlap','float4 v=float4(1,2,3,4); v.xy=v.yx; v.yz+=v.zy; return v;',[2,4,4,4])
add('logical_effects','''bool bump(inout int i) { i++; return true; }
float4 main(void):COLOR0 { int i=0; bool a=false && bump(i); bool b=true || bump(i); return float4(i,float(a),float(b),1); }''',[0,0,1,1])
add('copy_indices','''void set_values(inout float a, out float b) { a+=10; b=20; }
float4 main(void):COLOR0 { float a[3]; a[0]=1; a[1]=2; a[2]=3; int i=0; set_values(a[i++],a[i++]); return float4(a[0],a[1],a[2],i); }''',[11,20,3,2])
add('copy_nested','''float change(inout float a) { a+=1; return a*2; }
float sum(float a,float b) { return a+b; }
float4 main(void):COLOR0 { float a=1; float b=3; float r=sum(change(a),change(b)); return float4(a,b,r,1); }''',[2,4,12,1])
add('forward_overload','''float fn(float a); float fn(int a) { return 7; }
float4 main(void):COLOR0 { return float4(fn(float(2)),fn(int(2)),0,1); }
float fn(float a) { if(a>1) return a+1; return 0; }''',[3,7,0,1])
for mode in ['exact','wildcard','open']:
    funcs='float choose(void) { return 1; }\n'
    if mode!='open': funcs+='vs float choose(void) { return 2; }\nps float choose(void) { return 3; }\n'
    if mode=='exact': funcs+='mslv float choose(void) { return 4; }\nmslf float choose(void) { return 5; }\n'
    add('overload_'+mode,funcs+'float4 main(void):COLOR0 { return float4(choose(),0,0,1); }',[{'exact':5,'wildcard':3,'open':1}[mode],0,0,1])
    add('overload_vertex_'+mode,funcs+'struct O { float4 p:POSITION; float4 c:COLOR0; }; O main(float4 p:ATTRIB0) { O o; o.p=p; o.c=float4(choose(),0,0,1); return o; }',profile='mslv')
add('unreachable_unsupported','double unused(double x) { return x; }\nfloat4 main(void):COLOR0 { return float4(1,0,0,1); }',[1,0,0,1])
add('reserved_names','float fragment(float thread) { return thread+1; }\nfloat4 main(void):COLOR0 { float metal=2, vertex=3, cg_v_0=4; return float4(fragment(metal),vertex,cg_v_0,1); }',[3,3,4,1])
for n in range(2,5):
    nums=','.join(str(i) for i in range(1,n*n+1))
    body(f'matrix_{n}',f'float{n}x{n} m=float{n}x{n}({nums}); float{n}x{n} t=transpose(m); m[0][1]++; return float4(m[0][1], t[0][1], m[{n-1}][{n-1}],m[1][0]);',[3,n+1,n*n,n+1])
add('matrix_product','float4 main(void):COLOR0 { float2x2 a=float2x2(1,2,3,4); float2x2 c=a*a; return float4(c[0][0],c[0][1],c[1][0],c[1][1]); }',[1,4,9,16])
body('matrix_operators','float2x2 a=float2x2(1,2,3,4), b=float2x2(5,6,7,8); float2x2 d=mul(a,b); return float4(d[0][0],d[1][1],d[0][1],d[1][0]);',[19,50,22,43])
add('matrix_constructor_effects','float next(inout int n) { return ++n; }\nfloat4 main(void):COLOR0 { int n=0; float2x2 m=float2x2(next(n),next(n),next(n),next(n)); return float4(m[0][0],m[0][1],m[1][0],m[1][1]); }',[1,2,3,4])
body('matrix_row_effects','float2x2 m=float2x2(1,2,3,4); int i=0; float2 r=m[i++]; return float4(r.x,r.y,i,1);',[1,2,1,1])
add('aggregate_copy','''struct Inner { float x[2]; }; struct Outer { Inner a; float y; };
Outer copy(Outer a) { a.a.x[0]+=10; return a; }
float4 main(void):COLOR0 { Outer a; a.a.x[0]=1; a.a.x[1]=2; a.y=3; Outer b; b=copy(a); return float4(a.a.x[0],b.a.x[0],b.a.x[1],b.y); }''',[1,11,2,3])
# Boundary and binding fixtures; no rendering of oversized interfaces.
for n in [16,17]:
    fields=' '.join(f'float a{i}:ATTRIB{i};' for i in range(min(n,16))) + (' float extra:POSITION;' if n==17 else '')
    add(f'attributes_{n}',f'struct I {{ {fields} }}; float4 main(I i):POSITION {{ return float4(i.a0,0,0,1); }}',profile='mslv',reject='C6605' if n==17 else None)
for n in [256,257]:
    add(f'uniform_slots_{n}',f'float4 main(uniform float v[{n}]):COLOR0 {{ return float4(v[{n-1}],0,0,1); }}',reject='C6605' if n==257 else None)
for n in [16,17]:
    params=', '.join(f'uniform sampler2D s{i}' for i in range(n))
    add(f'resources_{n}',f'float4 main({params}):COLOR0 {{ return tex2D(s0,float2(0,0)); }}',reject='C6605' if n==17 else None)
negative={
'duplicate_alias':('struct O { float4 a:HPOS; float4 b:POSITION; }; O main(float4 p:ATTRIB0) { O o; o.a=p; o.b=p; return o; }','mslv','C6603'),
'duplicate_suffix':('float4 main(float a:TEXCOORD0,float b:texcoord00):COLOR0 { return float4(a,b,0,1); }','mslf','C6603'),
'missing_position':('float4 main(float4 p:ATTRIB0):COLOR0 { return p; }','mslv','C6603'),
'color1':('float4 main(void):COLOR1 { return float4(1,0,0,1); }','mslf','C6603'),
'position_type':('float3 main(float3 p:ATTRIB0):POSITION { return p; }','mslv','C6603'),
'depth_type':('float2 main(void):DEPTH { return float2(0,1); }','mslf','C6603'),
'matrix_interface':('float4 main(float2x2 m:TEXCOORD0):COLOR0 { return float4(m[0][0],m[0][1],m[1][0],m[1][1]); }','mslf','C6603'),
'system_semantic':('float4 main(float4 p:SV_POSITION):COLOR0 { return p; }','mslf','C6603'),
'vertex_derivative':('float4 main(float4 p:ATTRIB0):POSITION { return ddx(p); }','mslv','C6602'),
'duplicate_resource':('float4 main(uniform sampler2D a:TEXUNIT0,uniform sampler2D b:TEXUNIT0):COLOR0 { return tex2D(a,float2(0,0))+tex2D(b,float2(0,0)); }','mslf','C6604'),
'resource_array':('float4 main(uniform sampler2D a[2]):COLOR0 { return tex2D(a[0],float2(0,0)); }','mslf','C1151'),
'resource_record':('struct R { sampler2D t; }; float4 main(uniform R r):COLOR0 { return tex2D(r.t,float2(0,0)); }','mslf','C1151'),
'resource_return':('sampler2D get(uniform sampler2D t) { return t; } float4 main(uniform sampler2D t):COLOR0 { return tex2D(get(t),float2(0,0)); }','mslf','C1152'),
'resource_out':('void get(out sampler2D t) { return; } float4 main(uniform sampler2D t):COLOR0 { get(t); return float4(1,0,0,1); }','mslf','C1153'),
'resource_3d':('float4 main(uniform sampler3D t):COLOR0 { return float4(1,0,0,1); }','mslf','C6601'),
'numeric_binding':('float4 main(uniform float v:C0):COLOR0 { return float4(v,0,0,1); }','mslf','C6604'),
'global_write':('float v; float4 main(void):COLOR0 { v=1; return float4(v,0,0,1); }','mslf','C6608'),
'nested_type':('float bad(double x) { return float(x); } float wrap(float x) { return bad(x); } float4 main(uniform float x):COLOR0 { return float4(wrap(x),0,0,1); }','mslf','C6601'),

}
for name,(src,profile,code) in negative.items(): add('reject_'+name,src,profile=profile,reject=code)

add('metadata_order_a', """uniform float z;
uniform float unused;
float ignored(void) { return unused; }
float use_global(void) { return z; }
float4 main(uniform float z, uniform float a):COLOR0 { return float4(z,a,use_global(),1); }""")
add('metadata_order_b', """uniform float unused;
float ignored(void) { return unused; }
uniform float z;
float use_global(void) { return z; }
float4 main(uniform float a, uniform float z):COLOR0 { return float4(z,a,use_global(),1); }""")
add('metadata_resources_a','float4 main(uniform sampler2D z, uniform sampler2D fixedSlot:TEXUNIT0, uniform samplerCUBE a):COLOR0 { return tex2D(z,float2(0,0)); }')
add('metadata_resources_b','float4 main(uniform samplerCUBE a, uniform sampler2D z, uniform sampler2D fixedSlot:TEXUNIT0):COLOR0 { return tex2D(z,float2(0,0)); }')
add('metadata_attributes_a','float4 main(float4 p:POSITION, float4 n:NORMAL, float4 explicitValue:ATTRIB0):POSITION { return p+n+explicitValue; }',profile='mslv')
add('metadata_attributes_b','float4 main(float4 explicitValue:ATTRIB0, float4 n:NORMAL, float4 p:POSITION):POSITION { return p+n+explicitValue; }',profile='mslv')
add('metadata_defaults','float4 main(uniform float v=0.1f, uniform half2 h=half2(0.5h,0.25h), uniform bool b=true, uniform uint u=4294967295u):COLOR0 { return float4(v,float(h.x),float(b),float(u)); }')
add('metadata_nested','struct I { bool2 flags; half2 tint; }; struct O { I inner[2]; float3x3 matrix; }; float4 main(uniform O data):COLOR0 { return float4(float(data.inner[1].flags.x),float(data.inner[1].tint.x),data.matrix[0][1],1); }')
add('branch_effects','float next(inout int n) { n++; return n; } float4 main(void):COLOR0 { int n=0; float x; if(n==0) x=next(n); else x=next(n)+10; while(n<3) { n++; if(n==2) continue; x+=n; } return float4(x,n,0,1); }',[4,3,0,1])
add('matrix_row_store_effects','float4 main(void):COLOR0 { float2x2 m=float2x2(1,2,3,4); int n=0; m[n++]=float2(8,9); return float4(m[0][0],m[0][1],n,m[1][1]); }',[8,9,1,4])
add('matrix_alias_selector','float4 main(void):COLOR0 { float2x2 m=float2x2(1,2,3,4); m._m00_m01=m._m01_m00; return float4(m[0][0],m[0][1],m[1][0],m[1][1]); }',[2,1,3,4])
add('loop_condition_effects','bool more(inout int n) { n++; return n<4; } float4 main(void):COLOR0 { int n=0,s=0; while(more(n)) { s+=n; } return float4(n,s,0,1); }',[4,6,0,1])
add('literal_roundtrip','float4 main(void):COLOR0 { return float4(0.1f,1.00000011920928955078125f,1.1754943508222875e-38f,-0.0f); }',[.1,1.0000001192092896,1.1754943508222875e-38,0])
add('entry_out','void main(out float4 color:COLOR0, out float depth:DEPTH) { color=float4(1,0,0,1); depth=0.25; }')

for n in range(1,5):
    for kind in ['int','uint']:
        ty=kind+(str(n) if n>1 else '')
        for op,expr,result in [('min','min(x,x+1)',2),('max','max(x,x+1)',3),('clamp','clamp(x,x-1,x+1)',2)]+([('abs','abs(-x)',2)] if kind=='int' else []):
            src=f'float4 main(uniform float value):COLOR0 {{ {ty} x=int(value); {ty} y={expr}; return float4(float({"y.x" if n>1 else "y"}),0,0,1); }}'
            add(f'integer_{op}_{ty}',src,[result,0,0,1])
for n in range(2,5):
    add(f'catalog_mul{n}',f'float4 main(uniform float value):COLOR0 {{ float{n} x=value; return float4(mul(x,x),0,0,1); }}',[4*n,0,0,1])

add('reject_uniform_out','void change(inout float x) { x++; } float4 main(uniform float x):COLOR0 { change(x); return float4(x,0,0,1); }',reject='C6608')
add('reject_register_syntax','float4 main(uniform float v:register(c0)):COLOR0 { return float4(v,0,0,1); }',reject='C1300')
add('uniform_array_local_copy','float4 main(uniform float v[2]):COLOR0 { float a[2]; a=v; a[0]+=1; return float4(a[0],v[0],a[1],1); }',[3,2,.25,1])
add('uniform_local_inout','void change(inout float v) { v+=2; } float4 main(uniform float x):COLOR0 { float v=x; change(v); return float4(v,x,0,1); }',[4,2,0,1])
add('all_aliases','struct O { float4 p:HPOS; float4 a:DIFFUSE; float4 b:SPECULAR; float f:FOGCOORD; }; O main(float4 p:attrib00) { O o; o.p=p; o.a=float4(1,0,0,1); o.b=o.a; o.f=0; return o; }',profile='mslv')

add('matrix_row_inout','void change(inout float2 v) { v=v.yx; } float4 main(void):COLOR0 { float2x2 m=float2x2(1,2,3,4); int i=0; change(m[i++]); return float4(m[0][0],m[0][1],i,m[1][0]); }',[2,1,1,3])
add('copy_alias','void set_values(out float a,out float b) { a=3; b=4; } float4 main(void):COLOR0 { float a=1; set_values(a,a); return float4(a,0,0,1); }',[4,0,0,1])
for n in range(1,5):
    ty='bool'+(str(n) if n>1 else '')
    for op in ['any','all']:
        add(f'boolean_{op}{n}',f'float4 main(uniform {ty} x):COLOR0 {{ return float4(float({op}(x)),0,0,1); }}',[1,0,0,1])
for n in [2,4]:
    ty=f'float{n}'; vals=','.join(['0']*(n-1)+['1'])
    add(f'intrinsic_reflect_refract{n}',f'float4 main(void):COLOR0 {{ {ty} x={ty}({vals}); {ty} r=reflect(x,x); {ty} t=refract(x,-x,0.5); return float4(r[{n-1}],t[{n-1}],0,1); }}',[-1,1,0,1])
for name,expression in [('projected','tex2Dproj(t,float4(0,0,0,1))'),('bias','tex2Dbias(t,float4(0,0,0,1))'),('gradient','tex2D(t,float2(0,0),float2(1,0),float2(0,1))')]:
    add('reject_texture_'+name,f'float4 main(uniform sampler2D t):COLOR0 {{ return {expression}; }}',reject='C1008' if name=='bias' else 'C6602')

add('row_compound','float4 main(void):COLOR0 { float2x2 m=float2x2(1,2,3,4); int i=0; m[i++]+=m[1]; return float4(m[0][0],m[0][1],i,m[1][0]); }',[4,6,1,3])
add('minimal_dependencies','uniform float a; uniform float b; float only_a(float x) { return a*x; } float no_global(float x) { return x+1; } float transitive(float x) { return only_a(no_global(x)); } float4 main(void):COLOR0 { return float4(transitive(2),b,0,1); }',[6,.25,0,1])

add('aggregate_defaults','struct R { float a; float2 b; }; float4 main(uniform R r = {2, {3,4}}, uniform float a[2] = {5,6}):COLOR0 { return float4(r.a,r.b.x,a[0],a[1]); }',[2,3,5,6])
add('global_constants','const float a=2; const float2 b=float2(3,4); float4 main(void):COLOR0 { return float4(a,b.x,b.y,1); }',[2,3,4,1])
for profile in ['mslv','mslf']:
    src='#ifndef PROFILE_'+profile.upper()+'\n#error missing profile macro\n#endif\n'
    src+= 'float4 main(float4 p:ATTRIB0):POSITION { return p; }' if profile=='mslv' else 'float4 main(void):COLOR0 { return float4(1,0,0,1); }'
    add('macro_'+profile,src,profile=profile)

add('local_initializers','struct R { float a; float2 b; }; float4 main(void):COLOR0 { R r = {2,{3,4}}; float a[2]={5,6}; R copy=r; return float4(copy.a,copy.b.x,a[0],a[1]); }',[2,3,5,6])
add('matrix_compound','float4 main(void):COLOR0 { float2x2 a=float2x2(1,2,3,4); a*=a; return float4(a[0][0],a[0][1],a[1][0],a[1][1]); }',[1,4,9,16])
add('matrix_selector_effects','float next(inout int i) { return ++i; } float4 main(void):COLOR0 { int i=0; float2x2 a=float2x2(1,2,3,4); a._m00_m01=float2(next(i),next(i)); return float4(a[0][0],a[0][1],i,1); }',[1,2,2,1])

for kind in ['sampler1D','samplerRECT']:
    add('reject_'+kind,f'float4 main(uniform {kind} t):COLOR0 {{ return float4(1,0,0,1); }}',reject='C6601')
add('reject_dynamic_sampler','float4 main(uniform sampler2D a,uniform sampler2D b,uniform bool c):COLOR0 { return tex2D(c?a:b,float2(0,0)); }',reject='C1154')
add('reject_comparison','float4 main(uniform sampler2D a):COLOR0 { return shadow2D(a,float3(0,0,1)); }',reject='C1008')
add('reject_geometry','float4 main(float4 p:ATTRIB0):POSITION { emitVertex(p:POSITION); return p; }',profile='mslv',reject='C6317')

add('nested_initializers','struct I { float a; float b[2]; }; struct O { I i; float c; }; float4 main(void):COLOR0 { O o={{2,{3,4}},5}; return float4(o.i.a,o.i.b[0],o.i.b[1],o.c); }',[2,3,4,5])
add('global_aggregate_constant','struct I { float a; float b[2]; }; const I original={2,{3,4}}; const I data=original; float4 main(void):COLOR0 { return float4(data.a,data.b[0],data.b[1],1); }',[2,3,4,1])
add('matrix_scalar_arithmetic','float4 main(uniform float v):COLOR0 { float2x2 a=float2x2(1,2,3,4); float2x2 b=(a+v)/v; float2x2 c=v-a; return float4(b[0][0],b[1][0],c[0][1],c[1][1]); }',[1.5,2.5,0,-2])
add('matrix_binary_effects','float2x2 next(inout int i) { i++; return float2x2(i,i+1,i+2,i+3); } float4 main(void):COLOR0 { int i=0; float2x2 a=next(i)*next(i); return float4(a[0][0],a[0][1],a[1][1],i); }',[2,6,20,2])
add('dependent_constants','const float a=2; const float b=a+3; float4 main(void):COLOR0 { return float4(a,b,0,1); }',[2,5,0,1])
# Remove only files generated by this script which no longer have manifest entries.
for name in previous:
    path=root/(name+'.cg')
    if path.name not in created and path.exists(): path.unlink()
(root/'cases.cmake').write_text('# Generated by generate.py; expectations are independent witnesses.\n'+'\n'.join(cases)+'\n')
