#version 460 core


in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// Rock texture
uniform sampler2D uAlbedo;

// Sunlight
uniform vec3 uLightDirection;

void main()
{
    // Sample rock texture
    vec3 albedo = texture(uAlbedo, TexCoord).rgb;

    // Surface normal
    vec3 normal = normalize(Normal);

    // Directional lighting
    vec3 lightDir = normalize(uLightDirection);

    float diffuse = max(
        dot(normal, lightDir),
        0.0
    );

    float ambient = 0.35;

    float lighting = ambient + diffuse * 0.65;

    vec3 finalColor = albedo * lighting;

    FragColor = vec4(finalColor, 1.0);
}




//in vec3 FragPos;
//in vec3 Normal;
//in vec2 TexCoord;
//
//out vec4 FragColor;
//
//uniform vec3 uChunkColor;
//
//void main()
//{
//    // Sunlight direction
//    vec3 lightDir = normalize(
//        vec3(0.5, 1.0, 0.3)
//    );
//
//    // Normalise surface normal
//    vec3 normal = normalize(Normal);
//
//    // Diffuse lighting
//    float diffuse = max(
//        dot(normal, lightDir),
//        0.0
//    );
//
//    // Ambient light
//    float ambient = 0.35;
//
//    // Final lighting
//    float lighting = ambient + diffuse * 0.65;
//
//    // Terrain colour
//    vec3 terrainColor = vec3(
//        0.25,
//        0.55,
//        0.25
//    );
//
//    FragColor = vec4(
//        terrainColor * lighting,
//        1.0
//    );
//}





//
//#version 460 core
//
//out vec4 FragColor;
//
//uniform vec3 uChunkColor;
//
//void main()
//{
//    FragColor = vec4(uChunkColor, 1.0);
//}

//#version 460 core
//
//out vec4 FragColor;
//
//void main()
//{
//    FragColor = vec4(0.25, 0.55, 0.25, 1.0);
//}