#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// ============================================
// TEXTUREBOX MATERIALS
// ============================================

uniform sampler2D uGrassAlbedo;
uniform sampler2D uRockAlbedo;

uniform sampler2D uGrassNormal;
uniform sampler2D uRockNormal;

// ============================================
// LIGHTING
// ============================================

uniform vec3 uLightDirection;

// ============================================
// MATERIAL BLENDING
// ============================================

uniform float uRockStart;
uniform float uRockEnd;
uniform float uBlendNoise;

// ============================================
// TRIPLANAR SETTINGS
// ============================================

uniform float uTextureScale;
uniform float uTriplanarSharpness;

// Normal map strength
uniform float uGrassNormalStrength;
uniform float uRockNormalStrength;


// ============================================
// TRIPLANAR WEIGHTS
// ============================================

vec3 GetTriplanarWeights(vec3 normal)
{
    vec3 weights = abs(normal);

    weights = pow(
        weights,
        vec3(max(uTriplanarSharpness, 0.1))
    );

    weights /= max(
        weights.x + weights.y + weights.z,
        0.0001
    );

    return weights;
}


// ============================================
// PROJECTION COORDINATES
// ============================================

// Mirroring each projection appropriately
// also makes its normal-map orientation consistent.

void GetProjectionUVs(
    vec3 position,
    vec3 geometricNormal,
    out vec2 uvX,
    out vec2 uvY,
    out vec2 uvZ
)
{
    vec3 p = position * uTextureScale;

    vec3 s = vec3(
        geometricNormal.x >= 0.0 ? 1.0 : -1.0,
        geometricNormal.y >= 0.0 ? 1.0 : -1.0,
        geometricNormal.z >= 0.0 ? 1.0 : -1.0
    );

    uvX = vec2(-s.x * p.z, p.y);
    uvY = vec2(p.x, -s.y * p.z);
    uvZ = vec2(s.z * p.x, p.y);
}


// ============================================
// TRIPLANAR COLOUR
// ============================================

vec3 SampleTriplanarColor(
    sampler2D tex,
    vec2 uvX,
    vec2 uvY,
    vec2 uvZ,
    vec3 weights
)
{
    vec3 colorX = texture(tex, uvX).rgb;
    vec3 colorY = texture(tex, uvY).rgb;
    vec3 colorZ = texture(tex, uvZ).rgb;

    return
        colorX * weights.x +
        colorY * weights.y +
        colorZ * weights.z;
}


// ============================================
// UNPACK NORMAL MAP
// ============================================

vec3 UnpackNormal(vec3 rgb, float strength)
{
    vec3 n = rgb * 2.0 - 1.0;

    // Only scale sideways detail.
    // Zero strength gives a flat normal.
    n.xy *= max(strength, 0.0);

    return normalize(n);
}


// ============================================
// TRIPLANAR NORMAL MAPPING
// ============================================

vec3 SampleTriplanarNormal(
    sampler2D normalTexture,
    vec3 geometricNormal,
    vec2 uvX,
    vec2 uvY,
    vec2 uvZ,
    vec3 weights,
    float strength
)
{
    vec3 nx = UnpackNormal(
        texture(normalTexture, uvX).rgb,
        strength
    );

    vec3 ny = UnpackNormal(
        texture(normalTexture, uvY).rgb,
        strength
    );

    vec3 nz = UnpackNormal(
        texture(normalTexture, uvZ).rgb,
        strength
    );

    // Projection axis signs
    float sx = geometricNormal.x >= 0.0 ? 1.0 : -1.0;
    float sy = geometricNormal.y >= 0.0 ? 1.0 : -1.0;
    float sz = geometricNormal.z >= 0.0 ? 1.0 : -1.0;

    // Transform tangent-space normals into world space.
    //
    // X projection:
    // Tangent   = (0, 0, -sx)
    // Bitangent = (0, 1, 0)
    // Normal    = (sx, 0, 0)

    vec3 worldX = vec3(
        sx * nx.z,
        nx.y,
        -sx * nx.x
    );

    // Y projection:
    // Tangent   = (1, 0, 0)
    // Bitangent = (0, 0, -sy)
    // Normal    = (0, sy, 0)

    vec3 worldY = vec3(
        ny.x,
        sy * ny.z,
        -sy * ny.y
    );

    // Z projection:
    // Tangent   = (sz, 0, 0)
    // Bitangent = (0, 1, 0)
    // Normal    = (0, 0, sz)

    vec3 worldZ = vec3(
        sz * nz.x,
        nz.y,
        sz * nz.z
    );

    vec3 blended =
        worldX * weights.x +
        worldY * weights.y +
        worldZ * weights.z;

    // Keep the interpolated terrain normal when the
    // normal-map detail strength is zero.
    vec3 detailOffset =
        blended - vec3(
            sx * weights.x,
            sy * weights.y,
            sz * weights.z
        );

    return normalize(
        geometricNormal + detailOffset
    );
}


// ============================================
// MAIN
// ============================================

void main()
{
    // Original terrain geometry normal
    vec3 terrainNormal = normalize(Normal);

    // Triplanar projection weights
    vec3 weights = GetTriplanarWeights(
        terrainNormal
    );

    vec2 uvX;
    vec2 uvY;
    vec2 uvZ;

    GetProjectionUVs(
        FragPos,
        terrainNormal,
        uvX,
        uvY,
        uvZ
    );

    // ========================================
    // SAMPLE COLOUR TEXTURES
    // ========================================

    vec3 grassColor = SampleTriplanarColor(
        uGrassAlbedo,
        uvX, uvY, uvZ,
        weights
    );

    vec3 rockColor = SampleTriplanarColor(
        uRockAlbedo,
        uvX, uvY, uvZ,
        weights
    );

    // ========================================
    // GRASS / ROCK BLEND
    // ========================================

    float slope =
        1.0 - clamp(terrainNormal.y, 0.0, 1.0);

    float variation =
        sin(FragPos.x * 0.07) *
        cos(FragPos.z * 0.09);

    float adjustedSlope =
        slope + variation * uBlendNoise;

    float rockBlend = smoothstep(
        uRockStart,
        uRockEnd,
        adjustedSlope
    );

    rockBlend = clamp(rockBlend, 0.0, 1.0);

    vec3 terrainColor = mix(
        grassColor,
        rockColor,
        rockBlend
    );

    // ========================================
    // SAMPLE NORMAL MAPS
    // ========================================

    vec3 grassNormal = SampleTriplanarNormal(
        uGrassNormal,
        terrainNormal,
        uvX, uvY, uvZ,
        weights,
        uGrassNormalStrength
    );

    vec3 rockNormal = SampleTriplanarNormal(
        uRockNormal,
        terrainNormal,
        uvX, uvY, uvZ,
        weights,
        uRockNormalStrength
    );

    // Blend material normals
    vec3 finalNormal = normalize(
        mix(
            grassNormal,
            rockNormal,
            rockBlend
        )
    );

    // ========================================
    // DIRECTIONAL LIGHTING
    // ========================================

    vec3 lightDir = normalize(uLightDirection);

    // Broad terrain lighting
    float baseDiffuse = max(
        dot(terrainNormal, lightDir),
        0.0
    );

    // Fine material detail
    float detailDiffuse = max(
        dot(finalNormal, lightDir),
        0.0
    );

    // Keep the original landscape shading,
    // while adding normal-map detail.
    float diffuse = mix(
        baseDiffuse,
        detailDiffuse,
        0.75
    );

    float ambient = 0.35;

    float lighting =
        ambient +
        diffuse * 0.65;

    FragColor = vec4(
        terrainColor * lighting,
        1.0
    );
}



//in vec3 FragPos;
//in vec3 Normal;
//in vec2 TexCoord;
//
//out vec4 FragColor;
//
//// TextureBox materials
//uniform sampler2D uGrassAlbedo;
//uniform sampler2D uRockAlbedo;
//
//// Lighting
//uniform vec3 uLightDirection;
//
//// Material blending
//uniform float uRockStart;
//uniform float uRockEnd;
//uniform float uBlendNoise;
//
//// Triplanar settings
//uniform float uTextureScale;
//uniform float uTriplanarSharpness;
//
//
//// ============================================
//// Triplanar texture sampling
//// ============================================
//
//vec3 SampleTriplanar(
//    sampler2D tex,
//    vec3 worldPos,
//    vec3 normal
//)
//{
//    // Calculate blending weights
//    vec3 weights = abs(normal);
//
//    // Make projections more distinct
//    weights = pow(
//        weights,
//        vec3(uTriplanarSharpness)
//    );
//
//    // Normalise weights
//    weights /= max(
//        weights.x + weights.y + weights.z,
//        0.0001
//    );
//
//    // World-space texture coordinates
//    vec3 scaledPos = worldPos * uTextureScale;
//
//    // Project from three directions
//    vec3 xProjection = texture(
//        tex,
//        scaledPos.yz
//    ).rgb;
//
//    vec3 yProjection = texture(
//        tex,
//        scaledPos.xz
//    ).rgb;
//
//    vec3 zProjection = texture(
//        tex,
//        scaledPos.xy
//    ).rgb;
//
//    // Blend projections
//    return
//        xProjection * weights.x +
//        yProjection * weights.y +
//        zProjection * weights.z;
//}
//
//
//// ============================================
//// Main shader
//// ============================================
//
//void main()
//{
//    vec3 normal = normalize(Normal);
//
//    // Sample grass using triplanar mapping
//    vec3 grassColor = SampleTriplanar(
//        uGrassAlbedo,
//        FragPos,
//        normal
//    );
//
//    // Sample rock using triplanar mapping
//    vec3 rockColor = SampleTriplanar(
//        uRockAlbedo,
//        FragPos,
//        normal
//    );
//
//    // ========================================
//    // Grass / rock blending
//    // ========================================
//
//    float slope = 1.0 - clamp(normal.y, 0.0, 1.0);
//
//    // Add variation to rock boundaries
//    float variation =
//        sin(FragPos.x * 0.07) *
//        cos(FragPos.z * 0.09);
//
//    float adjustedSlope =
//        slope + variation * uBlendNoise;
//
//    float rockBlend = smoothstep(
//        uRockStart,
//        uRockEnd,
//        adjustedSlope
//    );
//
//    rockBlend = clamp(rockBlend, 0.0, 1.0);
//
//    // Combine grass and rock
//    vec3 terrainColor = mix(
//        grassColor,
//        rockColor,
//        rockBlend
//    );
//
//    // ========================================
//    // Directional sunlight
//    // ========================================
//
//    vec3 lightDir = normalize(uLightDirection);
//
//    float diffuse = max(
//        dot(normal, lightDir),
//        0.0
//    );
//
//    float ambient = 0.30;
//
//    float lighting =
//        ambient +
//        diffuse * 0.70;
//
//    vec3 finalColor =
//        terrainColor * lighting;
//
//    FragColor = vec4(
//        finalColor,
//        1.0
//    );
//}


