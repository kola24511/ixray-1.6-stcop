//--------------------------------------------------------------------------------------
// Gather pattern
//--------------------------------------------------------------------------------------

static float g_fHDAOZDispScale = 0.1f; // SSAO param

#ifdef USE_HDAO_CODE

static float g_fSSAORejectRadius = 0.43f; // SSAO param
static float g_fSSAOIntensity = 0.85f; // SSAO param
static float g_fSSAOAcceptRadius = 0.0001f; // SSAO param
static float g_fSSAOAcceptAngle = 0.98f; // Used by the ValleyAngle function to determine shallow valleys

    // Gather defines
    #define RING_0 (1)
    #define RING_1 (2)
    #define RING_2 (3)
    #define NUM_RING_0_GATHERS (2)
    #define NUM_RING_1_GATHERS (6)
    #define NUM_RING_2_GATHERS (12)

    #if SSAO_QUALITY == 3
static const int g_iNumRingGathers = NUM_RING_2_GATHERS;
static const int g_iNumRings = RING_2;
static const int g_iNumNormals = NUM_RING_0_GATHERS;
    #elif SSAO_QUALITY == 2
static const int g_iNumRingGathers = NUM_RING_1_GATHERS;
static const int g_iNumRings = RING_1;
static const int g_iNumNormals = NUM_RING_0_GATHERS;
    #elif SSAO_QUALITY == 1
static const int g_iNumRingGathers = NUM_RING_0_GATHERS;
static const int g_iNumRings = RING_0;
static const int g_iNumNormals = 0;
    #endif

// Ring sample pattern
static const float2 g_f2SSAORingPattern[NUM_RING_2_GATHERS] =
{
    // Ring 0
    {1, -1},
    {0, 1},

    // Ring 1
    {0, 3},
    {2, 1},
    {3, -1},
    {1, -3},

    // Ring 2
    {1, -5},
    {3, -3},
    {5, -1},
    {4, 1},
    {2, 3},
    {0, 5},
};

// Ring weights
static const float4 g_f4SSAORingWeight[NUM_RING_2_GATHERS] =
{
    // Ring 0 (Sum = 5.30864)
    {1.00000, 0.50000, 0.44721, 0.70711},
    {0.50000, 0.44721, 0.70711, 1.00000},

    // Ring 1 (Sum = 6.08746)
    {0.30000, 0.29104, 0.37947, 0.40000},
    {0.42426, 0.33282, 0.37947, 0.53666},
    {0.40000, 0.30000, 0.29104, 0.37947},
    {0.53666, 0.42426, 0.33282, 0.37947},

    // Ring 2 (Sum = 6.53067)
    {0.31530, 0.29069, 0.24140, 0.25495},
    {0.36056, 0.29069, 0.26000, 0.30641},
    {0.26000, 0.21667, 0.21372, 0.25495},
    {0.29069, 0.24140, 0.25495, 0.31530},
    {0.29069, 0.26000, 0.30641, 0.36056},
    {0.21667, 0.21372, 0.25495, 0.26000},
};

static const float g_fRingWeightsTotal[RING_2] =
{
    5.30864,
    11.39610,
    17.92677,
};

//----------------------------------------------------------------------------------------
// Helper function to Gather samples in 10.1 and 10.0 modes
//----------------------------------------------------------------------------------------

float4 GatherZSamples(float2 f2TexCoord)
{
    float4 f4Ret, f4Add;

    float2 f2InvRTSize = (1.0f).xx / pos_decompression_params2.xy;

    #ifndef SM_5
    f4Ret.x = g_txDepth.Load(int3(f2TexCoord * g_f2RTSize, 0), int2(1, 0)).x;
    f4Ret.y = g_txDepth.Load(int3(f2TexCoord * g_f2RTSize, 0), int2(1, 1)).x;
    f4Ret.z = g_txDepth.Load(int3(f2TexCoord * g_f2RTSize, 0), int2(0, 1)).x;
    f4Ret.w = g_txDepth.Load(int3(f2TexCoord * g_f2RTSize, 0), int2(0, 0)).x;
    #else // !SM_5
    f4Ret = g_txDepth.GatherRed(smp_nofilter, f2TexCoord);
    #endif // SM_5

    f4Ret = depth_unpack.x / (f4Ret - depth_unpack.y);

    #ifndef SM_5
    f4Add.x = 1.0f - 2.0f * g_txNormal.Load(int3(f2TexCoord * g_f2RTSize, 0), int2(1, 0)).z;
    f4Add.y = 1.0f - 2.0f * g_txNormal.Load(int3(f2TexCoord * g_f2RTSize, 0), int2(1, 1)).z;
    f4Add.z = 1.0f - 2.0f * g_txNormal.Load(int3(f2TexCoord * g_f2RTSize, 0), int2(0, 1)).z;
    f4Add.w = 1.0f - 2.0f * g_txNormal.Load(int3(f2TexCoord * g_f2RTSize, 0), int2(0, 0)).z;
    #else // !SM_5
    f4Add = 1.0f - 2.0f * g_txNormal.GatherBlue(smp_nofilter, f2TexCoord);
    #endif // SM_5

    f4Ret += g_fHDAOZDispScale * f4Add;

    return f4Ret;
}

    // Used by the valley angle function
    #define NUM_NORMAL_LOADS (4)
static const int2 g_i2HDAONormalLoadPattern[NUM_NORMAL_LOADS] =
{
    {0, -9},
    {6, -6},
    {10, 0},
    {8, 9},
};

    #ifdef SSAO_QUALITY
//=================================================================================================================================
// Computes the general valley angle
//=================================================================================================================================
float HDAOValleyAngle(uint2 u2ScreenCoord)
{
    float3 f3N1;
    float3 f3N2;
    float fDot;
    float fSummedDot = 0.0f;
    int2 i2MirrorPattern;
    int2 i2OffsetScreenCoord;
    int2 i2MirrorOffsetScreenCoord;

    float3 N = normalize(1.0f - 2.0f * g_txNormal.Load(int3(u2ScreenCoord, 0), 0).xyz);

    for (int iNormal = 0; iNormal < NUM_NORMAL_LOADS; iNormal++)
    {
        i2MirrorPattern = g_i2HDAONormalLoadPattern[iNormal] * int2(-1, -1);
        i2OffsetScreenCoord = u2ScreenCoord + g_i2HDAONormalLoadPattern[iNormal];
        i2MirrorOffsetScreenCoord = u2ScreenCoord + i2MirrorPattern;

        // Clamp our test to screen coordinates
        i2OffsetScreenCoord = (i2OffsetScreenCoord > (g_f2RTSize - float2(1.0f, 1.0f))) ? (g_f2RTSize - float2(1.0f, 1.0f)) : (i2OffsetScreenCoord);
        i2MirrorOffsetScreenCoord = (i2MirrorOffsetScreenCoord > (g_f2RTSize - float2(1.0f, 1.0f))) ? (g_f2RTSize - float2(1.0f, 1.0f)) : (i2MirrorOffsetScreenCoord);
        i2OffsetScreenCoord = (i2OffsetScreenCoord < 0) ? (0) : (i2OffsetScreenCoord);
        i2MirrorOffsetScreenCoord = (i2MirrorOffsetScreenCoord < 0) ? (0) : (i2MirrorOffsetScreenCoord);

        f3N1.xyz = normalize(1.0f - 2.0f * g_txNormal.Load(int3(i2OffsetScreenCoord, 0), 0).xyz);
        f3N2.xyz = normalize(1.0f - 2.0f * g_txNormal.Load(int3(i2MirrorOffsetScreenCoord, 0), 0).xyz);

        fDot = dot(f3N1, N);

        fSummedDot += (fDot > g_fSSAOAcceptAngle) ? (0.0f) : (1.0f - (abs(fDot) * 0.25f));
        fDot = dot(f3N2, N);
        fSummedDot += (fDot > g_fSSAOAcceptAngle) ? (0.0f) : (1.0f - (abs(fDot) * 0.25f));
    }

    fSummedDot /= 8.0f;
    fSummedDot += 0.5f;
    fSummedDot = (fSummedDot <= 0.5f) ? (fSummedDot / 10.0f) : (fSummedDot);

    return fSummedDot;
}
    #endif

float calc_hdao(float3 P, float3 N, float2 tc, float2 tcJ, float4 pos2d)
    #ifndef SSAO_QUALITY
{
    return 1.0f;
}
    #else
{
    // Locals
    int2 i2ScreenCoord;
    float2 f2ScreenCoord;
    float2 f2MirrorScreenCoord;
    float2 f2TexCoord;
    float2 f2MirrorTexCoord;
    float2 f2InvRTSize;
    float4 f4PosZ;
    float ZDisp;
    float3 f3D0;
    float3 f3D1;
    float4 f4Diff;
    float4 f4Compare[2];
    float4 f4Occlusion = 0.0f;
    float fOcclusion = 0.0f;

    // Compute integer screen coord, and store off the inverse of the RT Size
    f2InvRTSize = (1.0f).xx / (g_f2RTSize.xy);
    f2ScreenCoord = tc * (g_f2RTSize.xy);
    i2ScreenCoord = int2(f2ScreenCoord);

    // Get the general valley angle, to scale the result by
    float fDot = HDAOValleyAngle(i2ScreenCoord);

    ZDisp = P.z + g_fHDAOZDispScale * N.z;

        // For Gather we need to snap the screen coords
        #ifdef SM_4_1
    f2ScreenCoord = float2(i2ScreenCoord);
        #endif

    // Sample the center pixel for camera Z
    f2TexCoord = float2(f2ScreenCoord * f2InvRTSize);

    // Loop through each gather location, and compare with its mirrored location
    [unroll]
    for (int iGather = 0; iGather < g_iNumRingGathers; ++iGather)
    {
        f2MirrorScreenCoord = (g_f2SSAORingPattern[iGather] + float2(1.0f, 1.0f)) * float2(-1.0f, -1.0f);

        #ifdef SM_4_1
        f2TexCoord = float2((f2ScreenCoord + (g_f2SSAORingPattern[iGather] + float2(1.0f, 1.0f))) * f2InvRTSize);
        f2MirrorTexCoord = float2((f2ScreenCoord + (f2MirrorScreenCoord + float2(1.0f, 1.0f))) * f2InvRTSize);
        #else
        f2TexCoord = float2((f2ScreenCoord + g_f2SSAORingPattern[iGather]) * f2InvRTSize);
        f2MirrorTexCoord = float2((f2ScreenCoord + (f2MirrorScreenCoord)) * f2InvRTSize);
        #endif

        f4PosZ = GatherZSamples(f2TexCoord);
        f4Diff = ZDisp.xxxx - f4PosZ;
        f4Compare[0] = (f4Diff < g_fSSAORejectRadius.xxxx) ? (1.0f) : (0.0f);
        f4Compare[0] *= (f4Diff > g_fSSAOAcceptRadius.xxxx) ? (1.0f) : (0.0f);

        f4PosZ = GatherZSamples(f2MirrorTexCoord);
        f4Diff = ZDisp.xxxx - f4PosZ;
        f4Compare[1] = (f4Diff < g_fSSAORejectRadius.xxxx) ? (1.0f) : (0.0f);
        f4Compare[1] *= (f4Diff > g_fSSAOAcceptRadius.xxxx) ? (1.0f) : (0.0f);

        f4Occlusion.xyzw += g_f4SSAORingWeight[iGather].xyzw * (f4Compare[0].xyzw * f4Compare[1].zwxy);
    }

    fOcclusion = dot(f4Occlusion, g_fSSAOIntensity.xxxx) / (2.0f * g_fRingWeightsTotal[g_iNumRings - 1]);
    fOcclusion *= fDot;
    fOcclusion *= P.z < 0.5f ? 0.0f : lerp(0.0f, 1.0f, saturate(P.z - 0.5f));
    fOcclusion = 1.0f - saturate(fOcclusion);
    return fOcclusion;
}
    #endif
#endif

