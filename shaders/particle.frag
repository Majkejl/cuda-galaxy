#version 460 core

layout(location = 1) in float weight;

out vec4 FragColor;

void main() {
    vec2 uv = gl_PointCoord.xy;
    float dist = distance(uv, vec2(0.5));    
    if (dist > 0.5) discard;
    
    // 1. Core Star Intensity
    // Map distance to a sharp, bright core (e.g., dist < 0.1)

    float core = 1.0 - smoothstep(0.0, 0.15, dist);
    
    // 2. Diffuse Glow
    // Map distance to a smooth halo (e.g., dist < 0.5)
    float glow = 1.0 - smoothstep(0.15, 0.5, dist);
    
    // Combine core and glow for a bright star effect
    float intensity = core * 1.5 + glow * 0.5;
    
    // Output final color (e.g., bright cyan-white star)
    vec3 starColor = mix(vec3(1.,0.94,0.6), vec3(0.9,0.99,1.), weight) * intensity;
    
    FragColor = vec4(starColor, intensity);
}