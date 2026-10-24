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

    // ====================================
    // World boundaries
    // ====================================

    float BoundaryMountainHeight = 180.0f;
    float BoundaryStart = 384.0f;
    float BoundaryEnd = 640.0f;

    // ====================================
    // Valley settings
    // ====================================

    // 0 = Original terrain
    // 1 = Maximum flattening
    float ValleyFlatness = 0.85f;

    // Half-width of the flatter valley floor
    float ValleyWidth = 150.0f;

    // Distance where the original hills return
    float ValleyBlendEnd = 370.0f;
};