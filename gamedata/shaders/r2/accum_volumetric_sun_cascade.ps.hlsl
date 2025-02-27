//	Allow dynamic branching usage for HW PCF support
#ifdef USE_HWSMAP_PCF
    #define SUNSHAFTS_DYNAMIC
#endif //	USE_HWSMAP_PCF

#include "common.hlsli"
#include "shadow.hlsli"

#define RAY_PATH 2.0h
#define JITTER_TEXTURE_SIZE 64.0f

#define JITTER_SUN_SHAFTS

#ifdef SUN_SHAFTS_QUALITY
    #if SUN_SHAFTS_QUALITY == 1
        #define FILTER_LOW
        #define RAY_SAMPLES 20
    #elif SUN_SHAFTS_QUALITY == 2
        #define FILTER_LOW
        #define RAY_SAMPLES 30
    #elif SUN_SHAFTS_QUALITY == 3
        #define FILTER_LOW
        #define RAY_SAMPLES 40
    #endif
#endif

float4 volume_range; //	x - near plane, y - far plane
float4 sun_shafts_intensity;

uniform float4 screen_res;
// #ifdef	USE_BRANCHING
//	"If" in loop

float4 main(float2 tc : TEXCOORD0) : COLOR
{
#ifndef SUN_SHAFTS_QUALITY
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
#else //	SUN_SHAFTS_QUALITY

    float3 P = tex2D(s_position, tc).xyz;
#ifndef JITTER_SUN_SHAFTS
    //	Fixed ray length, fixed step dencity
    //	float3	direction = (RAY_PATH/RAY_SAMPLES)*normalize(P);
    //	Variable ray length, variable step dencity
    float3 direction = P / RAY_SAMPLES;
#else //	JITTER_SUN_SHAFTS
    //	Variable ray length, variable step dencity, use jittering
    float4 J0 = tex2D(jitter0, float4(screen_res.x / JITTER_TEXTURE_SIZE * tc, 0.0f, 0.0f)); // tcJ);
    float coeff = (RAY_SAMPLES - 1.0f * J0.x) / (RAY_SAMPLES * RAY_SAMPLES);
    float3 direction = P * coeff;
#endif //	JITTER_SUN_SHAFTS

    float depth = P.z;
    float deltaDepth = direction.z;

    float4 current = mul(m_shadow, float4(P, 1.0f));
    float4 delta = mul(m_shadow, float4(direction, 0.0f));

    float res = 0.0f;
    float max_density = sun_shafts_intensity;
    float density = max_density / RAY_SAMPLES;

    if (depth < 0.0001)
    {
        res = max_density;
    }

    #ifndef SUNSHAFTS_DYNAMIC
    for (int i = 0; i < RAY_SAMPLES; ++i)
    {
        if (depth > 0.3)
        #ifndef FILTER_LOW
            res += density * shadow_volumetric(current);
        #else //	FILTER_LOW
            res += density * sample_hw_pcf(current, float4(0, 0, 0, 0));
        #endif //	FILTER_LOW
        depth -= deltaDepth;
        current -= delta;
    }
    #else
    int n = (int)((P.z - 0.3) / deltaDepth);
    for (int i = 0; i < n; ++i)
    {
        #ifndef FILTER_LOW
        res += density * shadow_volumetric(current);
        #else //	FILTER_LOW
        res += density * sample_hw_pcf(current, float4(0, 0, 0, 0));
        #endif //	FILTER_LOW
        depth -= deltaDepth;
        current -= delta;
    }
    #endif

    //	float fSturation = -dot(Ldynamic_dir, float3(0,0,1));
    float fSturation = -Ldynamic_dir.z;

    //	Normalize dot product to
    fSturation = 0.5 * fSturation + 0.5;
    //	Map saturation to 0.2..1
    fSturation = 0.80 * fSturation + 0.20;

    float fog = saturate(length(P.xyz) * fog_params.w + fog_params.x);
    res = lerp(res, max_density, fog);
    res *= fSturation;

    return res * Ldynamic_color * 1.0;
#endif //	SUN_SHAFTS_QUALITY
}
