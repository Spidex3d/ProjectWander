#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// TextureBox materials
uniform sampler2D uGrassAlbedo;
uniform sampler2D uRockAlbedo;

// Lighting
uniform vec3 uLightDirection;

// Material blending
uniform float uRockStart;
uniform float uRockEnd;
uniform float uBlendNoise;

// Triplanar settings
uniform float uTextureScale;
uniform float uTriplanarSharpness;


// ============================================
// Triplanar texture sampling
// ============================================

vec3 SampleTriplanar(
    sampler2D tex,
    vec3 worldPos,
    vec3 normal
)
{
    // Calculate blending weights
    vec3 weights = abs(normal);

    // Make projections more distinct
    weights = pow(
        weights,
        vec3(uTriplanarSharpness)
    );

    // Normalise weights
    weights /= max(
        weights.x + weights.y + weights.z,
        0.0001
    );

    // World-space texture coordinates
    vec3 scaledPos = worldPos * uTextureScale;

    // Project from three directions
    vec3 xProjection = texture(
        tex,
        scaledPos.yz
    ).rgb;

    vec3 yProjection = texture(
        tex,
        scaledPos.xz
    ).rgb;

    vec3 zProjection = texture(
        tex,
        scaledPos.xy
    ).rgb;

    // Blend projections
    return
        xProjection * weights.x +
        yProjection * weights.y +
        zProjection * weights.z;
}


// ============================================
// Main shader
// ============================================

void main()
{
    vec3 normal = normalize(Normal);

    // Sample grass using triplanar mapping
    vec3 grassColor = SampleTriplanar(
        uGrassAlbedo,
        FragPos,
        normal
    );

    // Sample rock using triplanar mapping
    vec3 rockColor = SampleTriplanar(
        uRockAlbedo,
        FragPos,
        normal
    );

    // ========================================
    // Grass / rock blending
    // ========================================

    float slope = 1.0 - clamp(normal.y, 0.0, 1.0);

    // Add variation to rock boundaries
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

    // Combine grass and rock
    vec3 terrainColor = mix(
        grassColor,
        rockColor,
        rockBlend
    );

    // ========================================
    // Directional sunlight
    // ========================================

    vec3 lightDir = normalize(uLightDirection);

    float diffuse = max(
        dot(normal, lightDir),
        0.0
    );

    float ambient = 0.30;

    float lighting =
        ambient +
        diffuse * 0.70;

    vec3 finalColor =
        terrainColor * lighting;

    FragColor = vec4(
        finalColor,
        1.0
    );
}



//
//in vec3 FragPos;
//in vec3 Normal;
//in vec2 TexCoord;
//
//out vec4 FragColor;
//
//// Terrain materials
//uniform sampler2D uGrassAlbedo;
//uniform sampler2D uRockAlbedo;
//
//// Material blending controls
//uniform float uRockStart;
//uniform float uRockEnd;
//uniform float uBlendNoise;
//
//// Sunlight
//uniform vec3 uLightDirection;
//
//void main()
//{
//    // Sample both textures
//    vec3 grassColor =
//        texture(uGrassAlbedo, TexCoord).rgb;
//
//    vec3 rockColor =
//        texture(uRockAlbedo, TexCoord).rgb;
//
//    // Calculate terrain slope
//    vec3 normal = normalize(Normal);
//
////    float slope = 1.0 - normal.y;
//// Calculate slope
//float slope = 1.0 - clamp(normal.y, 0.0, 1.0);
//
//// Low-frequency variation using world position
//float variation =
//    sin(FragPos.x * 0.07) *
//    cos(FragPos.z * 0.09);
//
//// Distort the transition boundary
//float adjustedSlope =
//    slope + variation * uBlendNoise;
//
//// Calculate rock coverage
//float rockBlend = smoothstep(
//    uRockStart,
//    uRockEnd,
//    adjustedSlope
//);
//
//// Ensure values remain valid
//rockBlend = clamp(rockBlend, 0.0, 1.0);
////    // Blend grass and rock based on slope
////    float rockBlend = smoothstep(
////        0.15,
////        0.25,
////        slope
////    );
//
//    // Combine materials
//    vec3 terrainColor = mix(
//        grassColor,
//        rockColor,
//        rockBlend
//    );
//
//    // Basic directional sunlight
//    vec3 lightDir = normalize(uLightDirection);
//
//    float diffuse = max(
//        dot(normal, lightDir),
//        0.0
//    );
//
//    float ambient = 0.35;
//
//    float lighting = ambient + diffuse * 0.65;
//
//    FragColor = vec4(
//        terrainColor * lighting,
//        1.0
//    );
//}
//