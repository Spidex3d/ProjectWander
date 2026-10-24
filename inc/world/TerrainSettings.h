#pragma once

struct TerrainSettings
{
    // ====================================
    // World generation
    // ====================================

    int WorldSeed = 12345;

    // ====================================
    // Rolling hills
    // ====================================

    float HillHeight = 15.0f;
    float NoiseFrequency = 0.01f;
    int NoiseOctaves = 5;

    // ====================================
    // Mountains
    // ====================================

    float MountainHeight = 4.0f;
    float RidgeSharpness = 2.0f;

    // Mountain distribution
    float MountainFrequency = 0.001f;
    float MountainStart = 0.35f;
    float MountainEnd = 0.75f;
};