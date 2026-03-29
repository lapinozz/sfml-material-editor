#pragma once

#include "ViewportScopeGuard.hpp"
#include "archetypes.hpp"
#include "code-generator.hpp"
#include "expression.hpp"
#include "mls/material.hpp"

#include <array>
#include <format>

namespace NoiseNode_funcs
{

static inline const auto mod289 = CodeGen::makeGeneric({
    .id = "mod289",
    .returnType = Types::none,
    .params = {{"x", Types::none}},
    .body = {"return x - floor(x * (1.0 / 289.0)) * 289.0;"},
});

static inline const auto permute = CodeGen::makeGeneric({
    .id = "permute",
    .returnType = Types::none,
    .params = {{"x", Types::none}},
    .body = {"return $PRE$mod289$POST$(((x * 34.0) + 10.0) * x);"},
    .dependencies = {"mod289$POST$"},
});

static inline const auto taylorInvSqrt = CodeGen::makeGeneric({
    .id = "taylorInvSqrt",
    .returnType = Types::none,
    .params = {{"r", Types::none}},
    .body = {"return 1.79284291400159 - 0.85373472095314 * r;"},
});

static inline const auto fade = CodeGen::makeGeneric({
    .id = "fade",
    .returnType = Types::none,
    .params = {{"t", Types::none}},
    .body = {"return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);"},
});

static inline const CodeGen::Function safeMod_vec3 = {
    .id = "safeMod_vec3",
    .returnType = Types::vec3,
    .params = {{"v", Types::vec3}, {"d", Types::vec3}},
    .body = {"return vec3(d.x <= 0 ? v.x : mod(v.x, d.x), d.y <= 0 ? v.y : mod(v.y, d.y), d.z <= 0 ? v.z : mod(v.z, "
             "d.z));"},
};

static inline const CodeGen::Function safeMod_vec4 = {
    .id = "safeMod_vec4",
    .returnType = Types::vec4,
    .params = {{"v", Types::vec4}, {"d", Types::vec4}},
    .body = {"return vec4(d.x <= 0 ? v.x : mod(v.x, d.x), d.y <= 0 ? v.y : mod(v.y, d.y), d.z <= 0 ? v.z : mod(v.z, "
             "d.z), d.w <= 0 ? v.w : mod(v.w, d.w));"},
};

static inline const CodeGen::Function perlin2d = {
    .id = "perlin2d",
    .returnType = Types::scalar,
    .params = {{"offset", Types::vec2}, {"P", Types::vec2}, {"rep", Types::vec2}},
    .body = StringUtils::splitLines(R"==(
vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);

Pi = mlsf_safeMod_vec4(Pi, rep.xyxy) + offset.xyxy; // To create noise with explicit period
Pi = mlsf_mod289_vec4(Pi); // To avoid truncation effects in permutation
vec4 ix = Pi.xzxz;
vec4 iy = Pi.yyww;
vec4 fx = Pf.xzxz;
vec4 fy = Pf.yyww;

vec4 i = mlsf_permute_vec4(mlsf_permute_vec4(ix) + iy);

vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0;
vec4 gy = abs(gx) - 0.5;
vec4 tx = floor(gx + 0.5);
gx = gx - tx;

vec2 g00 = vec2(gx.x, gy.x);
vec2 g10 = vec2(gx.y, gy.y);
vec2 g01 = vec2(gx.z, gy.z);
vec2 g11 = vec2(gx.w, gy.w);

vec4 norm = mlsf_taylorInvSqrt_vec4(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));

float n00 = norm.x * dot(g00, vec2(fx.x, fy.x));
float n10 = norm.y * dot(g10, vec2(fx.y, fy.y));
float n01 = norm.z * dot(g01, vec2(fx.z, fy.z));
float n11 = norm.w * dot(g11, vec2(fx.w, fy.w));

vec2 fade_xy = mlsf_fade_vec2(Pf.xy);
vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
return 2.3 * n_xy;)==",
                                    true),
    .dependencies{
        "permute_vec4",
        "taylorInvSqrt_vec4",
        "fade_vec2",
        "mod289_vec4",
        "safeMod_vec4",
    },
};

static inline const CodeGen::Function perlin3d = {
    .id = "perlin3d",
    .returnType = Types::scalar,
    .params = {{"offset", Types::vec3}, {"P", Types::vec3}, {"rep", Types::vec3}},
    .body = StringUtils::splitLines(R"==(
vec3 Pi0 = mlsf_safeMod_vec3(floor(P), rep) + offset; // Integer part, modulo period
vec3 Pi1 = mlsf_safeMod_vec3(Pi0 + vec3(1.0), rep); // Integer part + 1, mod period
Pi0 = mlsf_mod289_vec3(Pi0);
Pi1 = mlsf_mod289_vec3(Pi1);
vec3 Pf0 = fract(P); // Fractional part for interpolation
vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
vec4 iy = vec4(Pi0.yy, Pi1.yy);
vec4 iz0 = Pi0.zzzz;
vec4 iz1 = Pi1.zzzz;

vec4 ixy = mlsf_permute_vec4(mlsf_permute_vec4(ix) + iy);
vec4 ixy0 = mlsf_permute_vec4(ixy + iz0);
vec4 ixy1 = mlsf_permute_vec4(ixy + iz1);

vec4 gx0 = ixy0 * (1.0 / 7.0);
vec4 gy0 = fract(floor(gx0) * (1.0 / 7.0)) - 0.5;
gx0 = fract(gx0);
vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
vec4 sz0 = step(gz0, vec4(0.0));
gx0 -= sz0 * (step(0.0, gx0) - 0.5);
gy0 -= sz0 * (step(0.0, gy0) - 0.5);

vec4 gx1 = ixy1 * (1.0 / 7.0);
vec4 gy1 = fract(floor(gx1) * (1.0 / 7.0)) - 0.5;
gx1 = fract(gx1);
vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
vec4 sz1 = step(gz1, vec4(0.0));
gx1 -= sz1 * (step(0.0, gx1) - 0.5);
gy1 -= sz1 * (step(0.0, gy1) - 0.5);

vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

vec4 norm0 = mlsf_taylorInvSqrt_vec4(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
vec4 norm1 = mlsf_taylorInvSqrt_vec4(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));

float n000 = norm0.x * dot(g000, Pf0);
float n010 = norm0.y * dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
float n100 = norm0.z * dot(g100, vec3(Pf1.x, Pf0.yz));
float n110 = norm0.w * dot(g110, vec3(Pf1.xy, Pf0.z));
float n001 = norm1.x * dot(g001, vec3(Pf0.xy, Pf1.z));
float n011 = norm1.y * dot(g011, vec3(Pf0.x, Pf1.yz));
float n101 = norm1.z * dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
float n111 = norm1.w * dot(g111, Pf1);

vec3 fade_xyz = mlsf_fade_vec3(Pf0);
vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
return 2.2 * n_xyz;
)==",
                                    true),
    .dependencies{
        "permute_vec4",
        "taylorInvSqrt_vec4",
        "fade_vec3",
        "mod289_vec3",
        "safeMod_vec3",
    },
};

static inline const CodeGen::Function perlin4d = {
    .id = "perlin4d",
    .returnType = Types::scalar,
    .params = {{"offset", Types::vec4}, {"P", Types::vec4}, {"rep", Types::vec4}},
    .body = StringUtils::splitLines(R"==(
vec4 Pi0 = mlsf_safeMod_vec4(floor(P), rep) + offset; // Integer part modulo rep
vec4 Pi1 = mlsf_safeMod_vec4(Pi0 + 1.0, rep); // Integer part + 1 mod rep
Pi0 = mlsf_mod289_vec4(Pi0);
Pi1 = mlsf_mod289_vec4(Pi1);
vec4 Pf0 = fract(P); // Fractional part for interpolation
vec4 Pf1 = Pf0 - 1.0; // Fractional part - 1.0
vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
vec4 iy = vec4(Pi0.yy, Pi1.yy);
vec4 iz0 = vec4(Pi0.zzzz);
vec4 iz1 = vec4(Pi1.zzzz);
vec4 iw0 = vec4(Pi0.wwww);
vec4 iw1 = vec4(Pi1.wwww);

vec4 ixy = mlsf_permute_vec4(mlsf_permute_vec4(ix) + iy);
vec4 ixy0 = mlsf_permute_vec4(ixy + iz0);
vec4 ixy1 = mlsf_permute_vec4(ixy + iz1);
vec4 ixy00 = mlsf_permute_vec4(ixy0 + iw0);
vec4 ixy01 = mlsf_permute_vec4(ixy0 + iw1);
vec4 ixy10 = mlsf_permute_vec4(ixy1 + iw0);
vec4 ixy11 = mlsf_permute_vec4(ixy1 + iw1);

vec4 gx00 = ixy00 * (1.0 / 7.0);
vec4 gy00 = floor(gx00) * (1.0 / 7.0);
vec4 gz00 = floor(gy00) * (1.0 / 6.0);
gx00 = fract(gx00) - 0.5;
gy00 = fract(gy00) - 0.5;
gz00 = fract(gz00) - 0.5;
vec4 gw00 = vec4(0.75) - abs(gx00) - abs(gy00) - abs(gz00);
vec4 sw00 = step(gw00, vec4(0.0));
gx00 -= sw00 * (step(0.0, gx00) - 0.5);
gy00 -= sw00 * (step(0.0, gy00) - 0.5);

vec4 gx01 = ixy01 * (1.0 / 7.0);
vec4 gy01 = floor(gx01) * (1.0 / 7.0);
vec4 gz01 = floor(gy01) * (1.0 / 6.0);
gx01 = fract(gx01) - 0.5;
gy01 = fract(gy01) - 0.5;
gz01 = fract(gz01) - 0.5;
vec4 gw01 = vec4(0.75) - abs(gx01) - abs(gy01) - abs(gz01);
vec4 sw01 = step(gw01, vec4(0.0));
gx01 -= sw01 * (step(0.0, gx01) - 0.5);
gy01 -= sw01 * (step(0.0, gy01) - 0.5);

vec4 gx10 = ixy10 * (1.0 / 7.0);
vec4 gy10 = floor(gx10) * (1.0 / 7.0);
vec4 gz10 = floor(gy10) * (1.0 / 6.0);
gx10 = fract(gx10) - 0.5;
gy10 = fract(gy10) - 0.5;
gz10 = fract(gz10) - 0.5;
vec4 gw10 = vec4(0.75) - abs(gx10) - abs(gy10) - abs(gz10);
vec4 sw10 = step(gw10, vec4(0.0));
gx10 -= sw10 * (step(0.0, gx10) - 0.5);
gy10 -= sw10 * (step(0.0, gy10) - 0.5);

vec4 gx11 = ixy11 * (1.0 / 7.0);
vec4 gy11 = floor(gx11) * (1.0 / 7.0);
vec4 gz11 = floor(gy11) * (1.0 / 6.0);
gx11 = fract(gx11) - 0.5;
gy11 = fract(gy11) - 0.5;
gz11 = fract(gz11) - 0.5;
vec4 gw11 = vec4(0.75) - abs(gx11) - abs(gy11) - abs(gz11);
vec4 sw11 = step(gw11, vec4(0.0));
gx11 -= sw11 * (step(0.0, gx11) - 0.5);
gy11 -= sw11 * (step(0.0, gy11) - 0.5);

vec4 g0000 = vec4(gx00.x,gy00.x,gz00.x,gw00.x);
vec4 g1000 = vec4(gx00.y,gy00.y,gz00.y,gw00.y);
vec4 g0100 = vec4(gx00.z,gy00.z,gz00.z,gw00.z);
vec4 g1100 = vec4(gx00.w,gy00.w,gz00.w,gw00.w);
vec4 g0010 = vec4(gx10.x,gy10.x,gz10.x,gw10.x);
vec4 g1010 = vec4(gx10.y,gy10.y,gz10.y,gw10.y);
vec4 g0110 = vec4(gx10.z,gy10.z,gz10.z,gw10.z);
vec4 g1110 = vec4(gx10.w,gy10.w,gz10.w,gw10.w);
vec4 g0001 = vec4(gx01.x,gy01.x,gz01.x,gw01.x);
vec4 g1001 = vec4(gx01.y,gy01.y,gz01.y,gw01.y);
vec4 g0101 = vec4(gx01.z,gy01.z,gz01.z,gw01.z);
vec4 g1101 = vec4(gx01.w,gy01.w,gz01.w,gw01.w);
vec4 g0011 = vec4(gx11.x,gy11.x,gz11.x,gw11.x);
vec4 g1011 = vec4(gx11.y,gy11.y,gz11.y,gw11.y);
vec4 g0111 = vec4(gx11.z,gy11.z,gz11.z,gw11.z);
vec4 g1111 = vec4(gx11.w,gy11.w,gz11.w,gw11.w);

vec4 norm00 = mlsf_taylorInvSqrt_vec4(vec4(dot(g0000, g0000), dot(g0100, g0100), dot(g1000, g1000), dot(g1100, g1100)));
vec4 norm01 = mlsf_taylorInvSqrt_vec4(vec4(dot(g0001, g0001), dot(g0101, g0101), dot(g1001, g1001), dot(g1101, g1101)));
vec4 norm10 = mlsf_taylorInvSqrt_vec4(vec4(dot(g0010, g0010), dot(g0110, g0110), dot(g1010, g1010), dot(g1110, g1110)));
vec4 norm11 = mlsf_taylorInvSqrt_vec4(vec4(dot(g0011, g0011), dot(g0111, g0111), dot(g1011, g1011), dot(g1111, g1111)));

float n0000 = norm00.x * dot(g0000, Pf0);
float n0100 = norm00.y * dot(g0100, vec4(Pf0.x, Pf1.y, Pf0.zw));
float n1000 = norm00.z * dot(g1000, vec4(Pf1.x, Pf0.yzw));
float n1100 = norm00.w * dot(g1100, vec4(Pf1.xy, Pf0.zw));
float n0010 = norm10.x * dot(g0010, vec4(Pf0.xy, Pf1.z, Pf0.w));
float n0110 = norm10.y * dot(g0110, vec4(Pf0.x, Pf1.yz, Pf0.w));
float n1010 = norm10.z * dot(g1010, vec4(Pf1.x, Pf0.y, Pf1.z, Pf0.w));
float n1110 = norm10.w * dot(g1110, vec4(Pf1.xyz, Pf0.w));
float n0001 = norm01.x * dot(g0001, vec4(Pf0.xyz, Pf1.w));
float n0101 = norm01.y * dot(g0101, vec4(Pf0.x, Pf1.y, Pf0.z, Pf1.w));
float n1001 = norm01.z * dot(g1001, vec4(Pf1.x, Pf0.yz, Pf1.w));
float n1101 = norm01.w * dot(g1101, vec4(Pf1.xy, Pf0.z, Pf1.w));
float n0011 = norm11.x * dot(g0011, vec4(Pf0.xy, Pf1.zw));
float n0111 = norm11.y * dot(g0111, vec4(Pf0.x, Pf1.yzw));
float n1011 = norm11.z * dot(g1011, vec4(Pf1.x, Pf0.y, Pf1.zw));
float n1111 = norm11.w * dot(g1111, Pf1);

vec4 fade_xyzw = mlsf_fade_vec4(Pf0);
vec4 n_0w = mix(vec4(n0000, n1000, n0100, n1100), vec4(n0001, n1001, n0101, n1101), fade_xyzw.w);
vec4 n_1w = mix(vec4(n0010, n1010, n0110, n1110), vec4(n0011, n1011, n0111, n1111), fade_xyzw.w);
vec4 n_zw = mix(n_0w, n_1w, fade_xyzw.z);
vec2 n_yzw = mix(n_zw.xy, n_zw.zw, fade_xyzw.y);
float n_xyzw = mix(n_yzw.x, n_yzw.y, fade_xyzw.x);
return 2.2 * n_xyzw;
)==",
                                    true),
    .dependencies{
        "permute_vec4",
        "taylorInvSqrt_vec4",
        "fade_vec4",
        "mod289_vec4",
        "safeMod_vec4",
    },
};

CodeGen::Function makeNoiseFunction(const CodeGen::Function& noiseStepFunction)
{
    const auto genType = noiseStepFunction.params[1].type;

    CodeGen::Function func = {
        .id = noiseStepFunction.id + "Loop",
        .returnType = Types::scalar,
        .params = {{"seed", Types::scalar}, {"P", genType}, {"rep", genType}, {"octaves", Types::scalar}, {"octaveMask", genType}},
        .body = StringUtils::splitLines(R"==(
vec4 offset = vec4(mlsf_rand_f(seed + 84.192), mlsf_rand_f(seed + 73.931), mlsf_rand_f(seed + 20.481), mlsf_rand_f(seed + 75.694));  

int MAX_OCTAVES = 6;
float gain = 0.5; // Persistence/gain
float lacunarity = 2.0; // Frequency multiplier

float result = 0.0;
float frequency = 1.0;
float amplitude = 1.0;

vec4 step = vec4(1.7, 2.5, 9.2, 6.6);

for(int i = 0; i < MAX_OCTAVES; i++)
{
    result += mlsf_$FUNC$(offset.$SWIZZLE$, mix(P * frequency, P, octaveMask)- step.$SWIZZLE$ * float(i), mix(rep * frequency, rep, octaveMask)) * amplitude;

    if(float(i) >= octaves)
    {
        break;
    }

    amplitude *= gain;
    frequency *= lacunarity;
}

return result;
)==",
                                        true),
        .dependencies{
            noiseStepFunction.id,
            "rand_f",
        },
    };

    std::array<std::string, 4> swizzles{"x", "xy", "xyz", "xyzw"};
    const auto replaceStrings = [&](std::string str, int x)
    {
        str = StringUtils::replaceAll(str, "$FUNC$", noiseStepFunction.id);
        str = StringUtils::replaceAll(str, "$SWIZZLE$", swizzles[x]);
        return str;
    };

    int arrity;
    if (genType == Types::vec2)
    {
        arrity = 2;
    }
    else if (genType == Types::vec3)
    {
        arrity = 3;
    }
    else if (genType == Types::vec4)
    {
        arrity = 4;
    }
    else
    {
        assert(false);
    }

    for (auto& line : func.body)
    {
        line = replaceStrings(line, arrity - 1);
    }

    return func;
}

static inline const CodeGen::Function perlin2dLoop = makeNoiseFunction(perlin2d);
static inline const CodeGen::Function perlin3dLoop = makeNoiseFunction(perlin3d);
static inline const CodeGen::Function perlin4dLoop = makeNoiseFunction(perlin4d);

} // namespace NoiseNode_funcs