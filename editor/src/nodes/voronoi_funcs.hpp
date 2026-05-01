#pragma once

#include "code-generator.hpp"
#include "mls/material.hpp"

#include <array>
#include <format>

namespace VoronoiNode_funcs
{

    /*
  vec2 mlsf_rand2_vec2( vec2 p ) {
    return fract(sin(vec2(dot(p + vec2(802.941),vec2(17.1,311.7)),dot(p + vec2(802.941),vec2(29.5,183.3))))*4358.5453);
}
float mlsf_rand_vec2( vec2 p ) {
    return fract(sin(dot(p + vec2(429.42, 0242.4231), vec4(12.9898, 78.233, 45.164, 94.673).xy)) * 43758.5453);
}

vec3 mlsf_rand3_float(float v ) {
    float a = fract(sin(dot(v, vec4(84.728, 85.040, 44.166, 51.316).x)) * 26283.45416);
    float b = fract(sin(dot(v, vec4(58.910, 34.142, 26.222, 16.950).x)) * 29288.86063);
    float c = fract(sin(dot(v, vec4(99.266, 84.077, 48.461, 34.265).x)) * 98482.98482);
    return vec3(a, b, c);
}

vec3 voronoiDistance( in vec2 x, float t )
{
    vec2 origin = floor(x);
    vec2 offset = fract(x);

    vec2 bestCell;
    vec2 bestPivot;
    
    float bestDist = 8.0;
    for(int j =- 1; j <= 1; j++)
    {
        for(int i = -1; i <= 1; i++)
        {
            vec2 cell = vec2(i, j);
            vec2 pivot = mod(mlsf_rand2_vec2(origin + cell), 1.);

            pivot = 0.5 + 0.25 * sin(t + 6.2831 * pivot) + 0.25 * cos(t * 0.8491 + 4.3191 * pivot);

            pivot = cell + pivot - offset;

            float dist = dot(pivot, pivot);

            if(dist < bestDist)
            {
                bestDist = dist;
                bestPivot = pivot;
                bestCell = cell;
            }
        }
    }
    
    float bestBorder = 8.0;
    for(int j = -2; j <= 2; j++)
    {
        for(int i = -2; i <= 2; i++)
        {
            if(i == 0 && j == 0)
            {
                continue;
            }

            
            vec2 cell = bestCell + vec2(i, j);
            vec2 pivot = mod(mlsf_rand2_vec2(origin + cell), 1.);

            pivot = 0.5 + 0.25 * sin(t + 6.2831 * pivot) + 0.25 * cos(t * 0.8491 + 4.3191 * pivot);

            pivot = cell + pivot - offset;

            float dist = dot(0.5 * (bestPivot + pivot), normalize(pivot - bestPivot));

            bestBorder = min(bestBorder, dist);
        }
    }
    
    float cellId = mlsf_rand_vec2(origin + bestCell);

    return vec3(cellId, bestDist, bestBorder);
}

float getBorder( in vec2 p, float t )
{
    float d = voronoiDistance( p, t).z;

    return 1.0 - smoothstep(0.02, 0.04, d);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / 150.;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));
    
    float t = iTime * 0.5 * 1.;
    
    float d = voronoiDistance(uv.xy, t).z;
    
    float max = 0.55;
    d = d / max;
    
    d = d + iTime * 0.25;
    
    float lines = 4. * 2.;
    
    float lineIndex = floor(d / (1./lines));
    d = mod(d, 1./lines) * lines;
    
    float b = getBorder(uv.xy, t);
    
    
    col = vec3(d);
    
    col = vec3(mod(mlsf_rand2_vec2(vec2(0., lineIndex + 1.)), vec2(1.)), 1.) * mod(lineIndex, 2.);

    //col = mix(col, vec3(1., 149./255., 0.), b);
    col = col + vec3(1., 149./255., 0.) * b;
    
    //col = vec3(voronoiDistance(uv.xy, t).x);
    
    //col = mlsf_rand3_float(voronoiDistance(uv.xy, t).x);

// Output to screen
    fragColor = vec4(col,1.0);
}
}
*/

static inline const CodeGen::Function voronoi = {
    .id = "voronoi",
    .returnType = Types::vec3,
    .params = {{"pos", Types::vec2}, {"t", Types::scalar}},
    .body = StringUtils::splitLines(R"==(
vec2 origin = floor(pos);
vec2 offset = fract(pos);

vec2 bestCell;
vec2 bestPivot;
    
float bestDist = 8.0;
for(int j =- 1; j <= 1; j++)
{
    for(int i = -1; i <= 1; i++)
    {
        vec2 cell = vec2(i, j);
        vec2 pivot = mlsf_rand2_vec2(origin + cell);

        pivot = 0.5 + 0.4 * sin(t + 9041. * pivot);

        pivot = cell + pivot - offset;

        float dist = dot(pivot, pivot);

        if(dist < bestDist)
        {
            bestDist = dist;
            bestPivot = pivot;
            bestCell = cell;
        }
    }
}
    
float bestBorder = 8.0;
for(int j = -2; j <= 2; j++)
{
    for(int i = -2; i <= 2; i++)
    {
        if(i == 0 && j == 0)
        {
            continue;
        }
            
        vec2 cell = bestCell + vec2(i, j);
        vec2 pivot = mlsf_rand2_vec2(origin + cell);

        pivot = 0.5 + 0.4 * sin(t + 9041. * pivot);

        pivot = cell + pivot - offset;

        float dist = dot(0.5 * (bestPivot + pivot), normalize(pivot - bestPivot));

        bestBorder = min(bestBorder, dist);
    }
}
    
float cellId = mlsf_rand_vec2(origin + bestCell);

return vec3(cellId, bestDist, bestBorder);
)==",
                                    true),
    .dependencies{
        "rand_vec2",
        "rand2_vec2",
    },
};

} // namespace NoiseNode_funcs