#include "render/scene_renderer.h"

#include "core/config.h"
#include "core/math_utils.h"
#include "core/physics_metrics.h"
#include "core/state.h"
#include "core/universe.h"
#include "fx/dust_field.h"
#include "fx/effects.h"
#include "physics/moon_system.h"
#include "render/camera.h"
#include "render/cosmetic_scene.h"
#include "render/gl_api.h"
#include "ui/layout.h"

#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

namespace spacesim {

namespace {

const char *kLitVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat4 uNormalMatrix;
uniform vec3 uTidalAxis;
uniform float uTidalAmount;
out vec3 vNormal;
out vec3 vWorld;
out vec3 vLocal;
void main() {
    vec3 p = aPos;
    float along = dot(p, uTidalAxis);
    p += uTidalAxis * along * uTidalAmount;
    vec4 world = uModel * vec4(p, 1.0);
    vWorld = world.xyz;
    vLocal = aPos;
    vNormal = normalize(mat3(uNormalMatrix) * aNormal);
    gl_Position = uViewProj * world;
}
)GLSL";

// A deliberately small shader used when the procedural surface shader fails
// on a driver. It avoids inverse/transpose, loops, and texture sampling so it
// compiles on older or stricter GLSL implementations.
const char *kLitFallbackVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat4 uNormalMatrix;
out vec3 vNormal;
out vec3 vWorld;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorld = world.xyz;
    vNormal = normalize(mat3(uNormalMatrix) * aNormal);
    gl_Position = uViewProj * world;
}
)GLSL";

const char *kLitFallbackFragment = R"GLSL(
#version 330 core
in vec3 vNormal;
in vec3 vWorld;
out vec4 fragColor;
uniform vec3 uColor;
uniform vec3 uEmissive;
uniform vec3 uLightPos;
uniform float uLightStrength;
uniform float uAmbient;
void main() {
    vec3 n = normalize(vNormal);
    vec3 toLight = uLightPos - vWorld;
    float distanceToLight = max(length(toLight), 0.001);
    vec3 l = toLight / distanceToLight;
    float diffuse = max(dot(n, l), 0.0);
    float attenuation = 1.0 / (1.0 + 0.35 * distanceToLight * distanceToLight);
    float light = uAmbient + uLightStrength * (0.18 + 0.82 * diffuse) * attenuation;
    vec3 color = uColor * light + uEmissive;
    fragColor = vec4(color, 1.0);
}
)GLSL";

const char *kLitFragment = R"GLSL(
#version 330 core
in vec3 vNormal;
in vec3 vWorld;
in vec3 vLocal;
out vec4 fragColor;
uniform vec3 uColor;
uniform vec3 uEmissive;
uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform vec3 uAtmosphere;
uniform float uCloud;
uniform float uSeed;
uniform int uSurfaceKind;
uniform float uLightStrength;
uniform float uAmbient;
uniform float uTime;
uniform float uTemperature;

uniform float uRingShadow;
uniform vec3 uRingCenter;
uniform vec3 uRingNormal;
uniform float uRingInner;
uniform float uRingOuter;
uniform sampler2D uRingDensity;

float ss_hash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.11, 0.17, 0.13));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float ss_noise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(ss_hash(i + vec3(0,0,0)), ss_hash(i + vec3(1,0,0)), f.x),
                   mix(ss_hash(i + vec3(0,1,0)), ss_hash(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(ss_hash(i + vec3(0,0,1)), ss_hash(i + vec3(1,0,1)), f.x),
                   mix(ss_hash(i + vec3(0,1,1)), ss_hash(i + vec3(1,1,1)), f.x), f.y), f.z);
}

float ss_fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; ++i) {
        value += amplitude * ss_noise3(p);
        p = p * 2.03 + vec3(3.1, 1.7, 4.3);
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec3 n = normalize(vNormal);
    vec3 p = normalize(vLocal);
    float lat = p.y;
    float n1 = ss_fbm(p * 3.0 + vec3(uSeed));
    float n2 = ss_fbm(p * 8.0 + vec3(uSeed * 1.7));
    vec3 surface = uColor;

    if (uSurfaceKind == 0) {
        surface = mix(uColor * 0.72, uColor * 1.28, n1);
        float poles = smoothstep(0.82, 0.96, abs(lat));
        surface = mix(surface, vec3(0.86, 0.88, 0.90), poles * 0.55);
        if (uCloud > 0.01) {
            float clouds = smoothstep(0.50, 0.72,
                ss_fbm(p * 5.0 + vec3(uSeed + 2.0, 0.7, 3.4)));
            surface = mix(surface, uColor * 1.30 + vec3(0.08),
                          clouds * uCloud);
        }
    } else if (uSurfaceKind == 1) {
        float land = smoothstep(0.47, 0.58, n1);
        float ice = smoothstep(0.78, 0.92, abs(lat));
        vec3 ocean = vec3(0.04, 0.16, 0.48);
        vec3 ground = mix(vec3(0.10, 0.34, 0.12),
                          vec3(0.55, 0.44, 0.22), n2);
        surface = mix(ocean, ground, land);
        surface = mix(surface, vec3(0.90, 0.94, 1.0), ice * 0.9);
        float clouds = smoothstep(0.52, 0.70,
            ss_fbm(p * 6.0 + vec3(uSeed + 4.0, 1.3, 2.0)));
        surface = mix(surface, vec3(0.96, 0.97, 1.0), clouds * uCloud);
    } else if (uSurfaceKind == 2) {
        float band = sin(lat * 18.0 + n1 * 3.0 + uSeed);
        float band2 = sin(lat * 31.0 + n2 * 4.0 - uSeed);
        vec3 c1 = uColor * 0.72;
        vec3 c2 = min(uColor * 1.22, vec3(1.0));
        vec3 c3 = vec3(0.94, 0.90, 0.78);
        surface = mix(c1, c2, smoothstep(-1.0, 1.0, band));
        surface = mix(surface, c3, smoothstep(0.55, 0.92, band2) * 0.24);
        if (uSeed > 4.5 && uSeed < 5.5) {
            float spot = smoothstep(0.20, 0.0,
                length(vec2((p.x - 0.25) * 1.4, p.y - 0.12)));
            surface = mix(surface, vec3(0.62, 0.24, 0.12), spot * 0.75);
        }
    } else if (uSurfaceKind == 3) {
        float band = sin(lat * 12.0 + n1 * 2.0 + uSeed);
        surface = mix(uColor * 0.78, min(uColor * 1.18, vec3(1.0)),
                      smoothstep(-1.0, 1.0, band));
        surface = mix(surface, vec3(0.82, 0.94, 1.0), n2 * 0.14);
    } else if (uSurfaceKind == 4) {
        surface = mix(uColor * 0.62, min(uColor * 1.12, vec3(1.0)), n1);
        surface = mix(surface, vec3(0.78, 0.84, 0.90),
                      smoothstep(0.62, 0.82, n2) * 0.24);
    } else if (uSurfaceKind == 5) {
        // Two-scale animated granulation: broad convective cells plus fine
        // bright lanes. The local sphere coordinate rotates with the Sun, so
        // the pattern also rotates with the body.
        float coarse = ss_fbm(p * 9.0 + vec3(uSeed, uTime * 0.08, 0.0));
        float fine = ss_fbm(p * 34.0 + vec3(0.0, uTime * 0.34, 0.0));
        float granulation = smoothstep(
            0.28, 0.78, 0.58 * fine + 0.42 * coarse);
        vec3 hot = vec3(1.0, 1.0, 0.96);
        vec3 warm = vec3(1.0, 0.53, 0.07);
        surface = mix(warm, hot, 0.22 + 0.78 * granulation);
        surface += vec3(0.34, 0.28, 0.16) * pow(fine, 3.0) * 0.24;

        // Faculae: small bright active regions near the limb and around spots.
        float faculae = smoothstep(0.58, 0.82, fine) *
                        (0.35 + 0.65 * coarse);
        surface += vec3(0.18, 0.11, 0.035) * faculae * 0.45;

        // Sunspots: a soft penumbra around a much darker umbra. The sparse
        // threshold keeps them as small active regions instead of large
        // blotches that cover the photosphere.
        float spot_noise = ss_noise3(p * 7.0 + vec3(uSeed + 2.0, 1.0, 0.0));
        float penumbra = smoothstep(0.72, 0.82, spot_noise);
        float umbra = smoothstep(0.82, 0.90, spot_noise);
        surface = mix(surface, vec3(0.36, 0.11, 0.018), penumbra * 0.26);
        surface = mix(surface, vec3(0.055, 0.010, 0.001), umbra * 0.52);
    }

    vec3 toLight = uLightPos - vWorld;
    float distanceToLight = max(length(toLight), 0.001);
    vec3 l = toLight / distanceToLight;
    float diffuse = max(dot(n, l), 0.0);
    float attenuation = 1.0 / (1.0 + 0.35 * distanceToLight * distanceToLight);
    float light = uAmbient + uLightStrength * (0.18 + 0.82 * diffuse) * attenuation;

    if (uRingShadow > 0.5 && abs(dot(l, uRingNormal)) > 1.0e-4) {
        float t = dot(uRingCenter - vWorld, uRingNormal) / dot(l, uRingNormal);
        if (t > 0.0 && t < distanceToLight) {
            vec3 hit = vWorld + l * t;
            float r = length(hit - uRingCenter);
            if (r >= uRingInner && r <= uRingOuter) {
                float u = (r - uRingInner) / (uRingOuter - uRingInner);
                float density = texture(uRingDensity, vec2(u, 0.5)).a;
                light *= 1.0 - density * 0.88;
            }
        }
    }

    vec3 viewDir = normalize(uCameraPos - vWorld);
    float fresnel = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0);
    surface += uAtmosphere * fresnel * 0.55;

    if (uSurfaceKind == 5) {
        float mu = max(dot(n, viewDir), 0.0);
        float limb = pow(mu, 0.58);
        vec3 limbColor = mix(vec3(1.0, 0.22, 0.015),
                             vec3(1.0, 1.0, 0.96), limb);
        surface *= limbColor * (0.86 + 0.26 * limb);
        surface += vec3(0.30, 0.28, 0.22) * pow(mu, 5.0) * 0.16;
    } else if (uSurfaceKind != 5 && uSurfaceKind != 7) {
        float cold = smoothstep(180.0, 40.0, uTemperature);
        surface = mix(surface, vec3(0.64, 0.78, 0.96), cold * 0.24);
    }

    if (uSurfaceKind == 1 || uSurfaceKind == 4) {
        float specular = pow(max(dot(reflect(-l, n), viewDir), 0.0), 28.0);
        surface += vec3(0.18) * specular * uLightStrength;
    }

    vec3 color = surface * light + uEmissive;
    if (uSurfaceKind == 5) {
        float mu = max(dot(n, viewDir), 0.0);
        float chromosphere = pow(1.0 - mu, 3.0);
        color = surface * 1.16 + uEmissive * 0.03;
        color += vec3(1.0, 0.20, 0.035) * chromosphere * 0.16;
    }
    fragColor = vec4(color, 1.0);
}
)GLSL";

const char *kColorVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uViewProj;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)GLSL";

const char *kColorFragment = R"GLSL(
#version 330 core
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
)GLSL";

const char *kPointVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;
uniform mat4 uViewProj;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_PointSize = aSize;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)GLSL";

const int RING_PROFILE_SIZE = 2048;
const double RING_INNER_RATIO = 1.11;
const double RING_OUTER_RATIO = 2.34;

struct RingZone {
    double inner;
    double outer;
    float color[3];
    float density;
};

static const RingZone RING_ZONES[6] = {
    {1.110, 1.236, {0.50f, 0.48f, 0.45f}, 0.055f}, // D ring
    {1.239, 1.527, {0.68f, 0.65f, 0.60f}, 0.20f},  // C ring
    {1.527, 1.951, {1.00f, 0.89f, 0.70f}, 0.62f},  // B ring
    {1.951, 2.027, {0.38f, 0.36f, 0.34f}, 0.035f}, // Cassini Division
    {2.027, 2.269, {0.94f, 0.82f, 0.63f}, 0.46f},  // A ring
    {2.324, 2.336, {0.72f, 0.68f, 0.60f}, 0.12f},  // F ring
};

void ring_profile_at(double radius, float color[3], float &density) {
    color[0] = 0.0f;
    color[1] = 0.0f;
    color[2] = 0.0f;
    density = 0.0f;
    for (int i = 0; i < 6; ++i) {
        if (radius >= RING_ZONES[i].inner && radius < RING_ZONES[i].outer) {
            color[0] = RING_ZONES[i].color[0];
            color[1] = RING_ZONES[i].color[1];
            color[2] = RING_ZONES[i].color[2];
            density = RING_ZONES[i].density;
            break;
        }
    }
    if (density <= 0.0f) return;

    // Fine ringlets are concentrated in the B and A rings, with a small
    // amount in the C ring. The three frequencies keep the texture from
    // reading as a regular sine pattern at close range.
    if (radius > 1.24 && radius < 2.27) {
        // Frequencies are chosen so the ringlets remain resolvable at the
        // default camera distance and gain detail as the camera moves closer.
        double fine = 0.50 +
            0.24 * std::sin(radius * 260.0 + 0.7) +
            0.16 * std::sin(radius * 620.0 + 2.1) +
            0.10 * std::sin(radius * 1300.0 + 4.2) +
            0.05 * std::sin(radius * 3200.0 + 1.3);
        fine = clamp_double(fine, 0.0, 1.0);
        double depth = (radius > 1.527 && radius < 1.951) ? 0.52 : 0.34;
        density = static_cast<float>(density * (1.0 - depth + depth * fine));
        double color_shift = 0.90 + 0.10 * fine;
        color[0] = static_cast<float>(color[0] * color_shift);
        color[1] = static_cast<float>(color[1] * color_shift);
        color[2] = static_cast<float>(color[2] * color_shift);
    }

    // A few narrow structural gaps in addition to the major divisions.
    const double notches[7] = {1.635, 1.712, 1.792, 1.845, 2.074, 2.125, 2.205};
    for (int i = 0; i < 7; ++i) {
        double delta = radius - notches[i];
        if (delta > -0.006 && delta < 0.006) {
            double gap = 1.0 - std::fabs(delta) / 0.006;
            density = static_cast<float>(density * (1.0 - 0.45 * gap));
        }
    }

    // Major gaps: Cassini is represented by its own low-density zone, while
    // Encke and Keeler are narrow notches inside the A ring.
    if (radius > 2.213 && radius < 2.218) density *= 0.04f;
    if (radius > 2.264 && radius < 2.267) density *= 0.10f;
    if (radius > 1.236 && radius < 1.239) density = 0.0f;

    // Soft inner/outer edges for the resolved ring disc.
    if (radius < 1.113 || radius > 2.334) density = 0.0f;
}

const char *kPointFragment = R"GLSL(
#version 330 core
in vec4 vColor;
out vec4 fragColor;
void main() {
    vec2 d = gl_PointCoord - vec2(0.5);
    if (dot(d, d) > 0.25) discard;
    fragColor = vColor;
}
)GLSL";

const char *kGlowFragment = R"GLSL(
#version 330 core
in vec4 vColor;
out vec4 fragColor;
void main() {
    vec2 d = gl_PointCoord - vec2(0.5);
    float radius = length(d) * 2.0;
    if (radius > 1.0) discard;
    float falloff = exp(-radius * radius * 3.6) * (1.0 - radius);
    fragColor = vec4(vColor.rgb, vColor.a * falloff);
}
)GLSL";

const char *kRingVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uViewProj;
out vec3 vWorld;
out vec4 vColor;
void main() {
    vWorld = aPos;
    vColor = aColor;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)GLSL";

const char *kRingFragment = R"GLSL(
#version 330 core
in vec3 vWorld;
in vec4 vColor;
out vec4 fragColor;

uniform sampler2D uRingProfile;
uniform vec3 uRingCenter;
uniform vec3 uRingNormal;
uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform float uPlanetRadius;
uniform float uInnerRatio;
uniform float uOuterRatio;

void main() {
    vec3 relative = vWorld - uRingCenter;
    float radial = length(relative) / uPlanetRadius;
    if (radial < uInnerRatio || radial > uOuterRatio) discard;
    float t = (radial - uInnerRatio) / (uOuterRatio - uInnerRatio);
    vec4 profile = texture(uRingProfile, vec2(t, 0.5));
    float density = profile.a;
    float edge = smoothstep(0.0, 0.035, t) *
                 (1.0 - smoothstep(0.965, 1.0, t));
    density *= edge;
    float alpha = 1.0 - exp(-density * 2.4);

    vec3 normal = normalize(uRingNormal);
    vec3 toSun = uLightPos - vWorld;
    float sunDistance = length(toSun);
    if (sunDistance < 1.0e-5) {
        fragColor = vec4(profile.rgb, alpha * vColor.a);
        return;
    }
    vec3 lightDir = toSun / sunDistance;

    // Soft planet shadow: the ray from this ring fragment to the star either
    // grazes or intersects Saturn. The transition is deliberately wide enough
    // to avoid the old hard-edged shadow stripe.
    vec3 oc = uRingCenter - vWorld;
    float b = dot(oc, lightDir);
    float c = dot(oc, oc) - uPlanetRadius * uPlanetRadius;
    float discriminant = b * b - c;
    float shadow = 1.0;
    if (discriminant > 0.0 && b > 0.0 && b < sunDistance) {
        float axisDistance = sqrt(max(uPlanetRadius * uPlanetRadius -
                                      discriminant, 0.0));
        shadow = 0.10 + 0.90 * smoothstep(uPlanetRadius * 0.92,
                                          uPlanetRadius * 1.18,
                                          axisDistance);
    }

    vec3 viewDir = normalize(uCameraPos - vWorld);
    float sunElevation = abs(dot(normal, lightDir));
    float viewElevation = abs(dot(normal, viewDir));
    float forward = max(dot(viewDir, -lightDir), 0.0);
    float illumination = 0.58 +
                         0.20 * sunElevation +
                         0.15 * viewElevation +
                         0.25 * pow(forward, 3.0);
    vec3 color = profile.rgb * (0.62 + 0.58 * illumination);
    color *= mix(0.30, 1.0, shadow);
    float opticalShade = 0.70 + 0.30 * (1.0 - density);
    color *= opticalShade;
    // A faint grazing-angle sheen helps the rings read as a particle layer
    // instead of a painted sheet.
    float sheen = pow(1.0 - viewElevation, 2.0);
    color += profile.rgb * sheen * 0.06;

    fragColor = vec4(color, alpha * vColor.a);
}
)GLSL";

const char *kStarHaloFragment = R"GLSL(
#version 330 core
in vec3 vWorld;
in vec4 vColor;
out vec4 fragColor;
uniform vec3 uGlowCenter;
uniform float uGlowRadius;
void main() {
    float radius = length(vWorld - uGlowCenter) / max(uGlowRadius, 1.0e-6);
    if (radius > 1.0) discard;
    float falloff = 1.0 - radius;
    fragColor = vec4(vColor.rgb * vColor.a * falloff * 1.7, 1.0);
}
)GLSL";

char g_gl_log_path[512] = "gl_errors.log";

std::FILE *open_gl_log() {
    std::FILE *file = std::fopen("gl_errors.log", "w");
    if (file != nullptr) {
        std::snprintf(g_gl_log_path, sizeof(g_gl_log_path), "%s", "gl_errors.log");
        return file;
    }
    const char *temp = std::getenv("TEMP");
    if (temp == nullptr) temp = std::getenv("TMP");
    if (temp == nullptr) temp = "/tmp";
    std::snprintf(g_gl_log_path, sizeof(g_gl_log_path), "%s/space_simulator_gl.log",
                  temp);
    return std::fopen(g_gl_log_path, "w");
}

void log_shader(std::FILE *file, const char *name, const GlShader &shader) {
    if (file == nullptr) return;
    std::fprintf(file, "\n--- %s ---\n", name);
    if (shader.valid()) {
        std::fprintf(file, "OK\n");
    } else {
        std::fprintf(file, "FAILED\n%s\n", shader.last_log().c_str());
    }
}

void write_gl_log(const GlShader &lit, const GlShader &fallback,
                  const GlShader &color, const GlShader &point,
                  const GlShader &glow, const GlShader &ring,
                  const GlShader &halo) {
    std::FILE *file = open_gl_log();
    if (file == nullptr) return;

    auto gl_text = [](GLenum name) -> const char * {
        const GLubyte *value = glGetString(name);
        return value != nullptr ? reinterpret_cast<const char *>(value) : "(null)";
    };

    std::time_t now = std::time(nullptr);
    std::fprintf(file, "Space Simulator v3.5 OpenGL diagnostics\n");
    std::fprintf(file, "time: %s", std::ctime(&now));
    std::fprintf(file, "GL_VENDOR: %s\n", gl_text(GL_VENDOR));
    std::fprintf(file, "GL_RENDERER: %s\n", gl_text(GL_RENDERER));
    std::fprintf(file, "GL_VERSION: %s\n", gl_text(GL_VERSION));
    std::fprintf(file, "GL_SHADING_LANGUAGE_VERSION: %s\n",
                 gl_text(GL_SHADING_LANGUAGE_VERSION));

    GLint profile = 0;
#ifdef GL_CONTEXT_PROFILE_MASK
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
#endif
    const char *profile_name = "unknown";
#ifdef GL_CONTEXT_CORE_PROFILE_BIT
    if ((profile & GL_CONTEXT_CORE_PROFILE_BIT) != 0) profile_name = "core";
#endif
#ifdef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
    if ((profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0) {
        profile_name = "compatibility";
    }
#endif
    std::fprintf(file, "context profile: %s (0x%x)\n", profile_name,
                 static_cast<unsigned int>(profile));
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    std::fprintf(file, "MSAA samples: %d\n", samples);

    log_shader(file, "lit (procedural bodies)", lit);
    log_shader(file, "lit fallback", fallback);
    log_shader(file, "color lines", color);
    log_shader(file, "point sprites", point);
    log_shader(file, "glow sprites", glow);
    log_shader(file, "Saturn ring", ring);
    log_shader(file, "star halo", halo);
    std::fclose(file);

    std::fprintf(stderr, "OpenGL diagnostics written to %s\n", g_gl_log_path);
}

} // namespace

SceneRenderer::SceneRenderer()
    : status_(), use_fallback_lit_(false),
      sphere_vao_(0), sphere_vbo_(0), sphere_ebo_(0), sphere_index_count_(0),
      line_vao_(0), line_vbo_(0), point_vao_(0), point_vbo_(0), ring_texture_(0),
      light_index_(-1), light_position_(0.0, 0.0, 0.0),
      camera_position_(0.0, 0.0, 0.0), sim_time_(0.0), viewport_height_(900),
      label_count_(0), last_trail_time_(-1.0), trails_was_visible_(false),
      prediction_count_(0), prediction_selection_(-1),
      prediction_time_(-1.0) {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        trail_head_[i] = 0;
        trail_count_[i] = 0;
    }
}

SceneRenderer::~SceneRenderer() {
    if (sphere_ebo_ != 0) glapi::DeleteBuffers(1, &sphere_ebo_);
    if (sphere_vbo_ != 0) glapi::DeleteBuffers(1, &sphere_vbo_);
    if (sphere_vao_ != 0) glapi::DeleteVertexArrays(1, &sphere_vao_);
    if (line_vbo_ != 0) glapi::DeleteBuffers(1, &line_vbo_);
    if (line_vao_ != 0) glapi::DeleteVertexArrays(1, &line_vao_);
    if (point_vbo_ != 0) glapi::DeleteBuffers(1, &point_vbo_);
    if (point_vao_ != 0) glapi::DeleteVertexArrays(1, &point_vao_);
    if (ring_texture_ != 0) glDeleteTextures(1, &ring_texture_);
}

void SceneRenderer::init() {
    // SPACESIM_FORCE_LIT_FALLBACK is a diagnostic switch for verifying the
    // safe renderer on a machine where the procedural shader works.
    const bool force_fallback =
        std::getenv("SPACESIM_FORCE_LIT_FALLBACK") != nullptr;
    const bool lit_ok = !force_fallback &&
                        lit_shader_.build(kLitVertex, kLitFragment);
    const bool fallback_ok = lit_fallback_shader_.build(kLitFallbackVertex,
                                                        kLitFallbackFragment);
    if (!lit_ok) {
        use_fallback_lit_ = fallback_ok;
        // Keep the fallback silent on screen. The reason is already written to
        // stderr and gl_errors.log, and the simplified renderer still works.
        // Only surface an on-screen warning if no body shader can run at all.
        if (!fallback_ok) {
            status_ = "Body shaders failed - see gl_errors.log";
        }
        std::fprintf(stderr,
                     "WARNING: the procedural body shader is not active; using "
                     "the fallback renderer (success=%s).\n",
                     fallback_ok ? "yes" : "no");
    }

    const bool color_ok = color_shader_.build(kColorVertex, kColorFragment);
    const bool point_ok = point_shader_.build(kPointVertex, kPointFragment);
    const bool glow_ok = glow_shader_.build(kPointVertex, kGlowFragment);
    const bool ring_ok = ring_shader_.build(kRingVertex, kRingFragment);
    const bool halo_ok = star_halo_shader_.build(kRingVertex, kStarHaloFragment);

    if (status_.empty() &&
        (!color_ok || !point_ok || !glow_ok || !ring_ok || !halo_ok)) {
        status_ = "Some effects disabled - see gl_errors.log";
    }

    write_gl_log(lit_shader_, lit_fallback_shader_, color_shader_,
                 point_shader_, glow_shader_, ring_shader_, star_halo_shader_);

    create_sphere();

    glapi::GenVertexArrays(1, &line_vao_);
    glapi::GenBuffers(1, &line_vbo_);
    glapi::BindVertexArray(line_vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, line_vbo_);
    glapi::BufferData(GL_ARRAY_BUFFER, sizeof(LineVertex) * 24000, nullptr,
                      GL_DYNAMIC_DRAW);
    glapi::EnableVertexAttribArray(0);
    glapi::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                               reinterpret_cast<void *>(0));
    glapi::EnableVertexAttribArray(1);
    glapi::VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LineVertex),
                               reinterpret_cast<void *>(sizeof(float) * 3));
    glapi::BindVertexArray(0);

    glapi::GenVertexArrays(1, &point_vao_);
    glapi::GenBuffers(1, &point_vbo_);
    glapi::BindVertexArray(point_vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, point_vbo_);
    glapi::BufferData(GL_ARRAY_BUFFER, sizeof(PointVertex) * 8192, nullptr,
                      GL_DYNAMIC_DRAW);
    glapi::EnableVertexAttribArray(0);
    glapi::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PointVertex),
                               reinterpret_cast<void *>(0));
    glapi::EnableVertexAttribArray(1);
    glapi::VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(PointVertex),
                               reinterpret_cast<void *>(sizeof(float) * 3));
    glapi::EnableVertexAttribArray(2);
    glapi::VertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(PointVertex),
                               reinterpret_cast<void *>(sizeof(float) * 7));
    glapi::BindVertexArray(0);

    glEnable(GL_PROGRAM_POINT_SIZE);

    std::vector<unsigned char> profile(RING_PROFILE_SIZE * 4);
    for (int i = 0; i < RING_PROFILE_SIZE; ++i) {
        double t = static_cast<double>(i) /
                   static_cast<double>(RING_PROFILE_SIZE - 1);
        double radius = RING_INNER_RATIO +
                        (RING_OUTER_RATIO - RING_INNER_RATIO) * t;
        float color[3] = {0.0f, 0.0f, 0.0f};
        float density = 0.0f;
        ring_profile_at(radius, color, density);
        profile[i * 4 + 0] = static_cast<unsigned char>(
            clamp_double(color[0], 0.0, 1.0) * 255.0);
        profile[i * 4 + 1] = static_cast<unsigned char>(
            clamp_double(color[1], 0.0, 1.0) * 255.0);
        profile[i * 4 + 2] = static_cast<unsigned char>(
            clamp_double(color[2], 0.0, 1.0) * 255.0);
        profile[i * 4 + 3] = static_cast<unsigned char>(
            clamp_double(density, 0.0, 1.0) * 255.0);
    }
    glGenTextures(1, &ring_texture_);
    glBindTexture(GL_TEXTURE_2D, ring_texture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 RING_PROFILE_SIZE, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, profile.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void SceneRenderer::create_sphere() {
    const int slices = 32;
    const int stacks = 20;
    struct Vertex {
        float px, py, pz;
        float nx, ny, nz;
    };
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    for (int stack = 0; stack <= stacks; ++stack) {
        double phi = PI * static_cast<double>(stack) / stacks;
        double y = std::cos(phi);
        double radius = std::sin(phi);
        for (int slice = 0; slice <= slices; ++slice) {
            double theta = 2.0 * PI * static_cast<double>(slice) / slices;
            double x = radius * std::cos(theta);
            double z = radius * std::sin(theta);
            vertices.push_back({static_cast<float>(x), static_cast<float>(y),
                                static_cast<float>(z),
                                static_cast<float>(x), static_cast<float>(y),
                                static_cast<float>(z)});
        }
    }
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            unsigned int a = static_cast<unsigned int>(stack * (slices + 1) + slice);
            unsigned int b = a + static_cast<unsigned int>(slices + 1);
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);
            indices.push_back(b);
            indices.push_back(b + 1);
            indices.push_back(a + 1);
        }
    }
    sphere_index_count_ = static_cast<int>(indices.size());

    glapi::GenVertexArrays(1, &sphere_vao_);
    glapi::GenBuffers(1, &sphere_vbo_);
    glapi::BindVertexArray(sphere_vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, sphere_vbo_);
    glapi::BufferData(GL_ARRAY_BUFFER,
                      static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                      vertices.data(), GL_STATIC_DRAW);
    glapi::EnableVertexAttribArray(0);
    glapi::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               reinterpret_cast<void *>(0));
    glapi::EnableVertexAttribArray(1);
    glapi::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               reinterpret_cast<void *>(sizeof(float) * 3));
    glapi::GenBuffers(1, &sphere_ebo_);
    glapi::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ebo_);
    glapi::BufferData(GL_ELEMENT_ARRAY_BUFFER,
                      static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                      indices.data(), GL_STATIC_DRAW);
    glapi::BindVertexArray(0);
}

int SceneRenderer::labelCount() const { return label_count_; }
const ScreenLabel *SceneRenderer::labels() const { return labels_.data(); }

void SceneRenderer::reset_trails() {
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        trail_head_[i] = 0;
        trail_count_[i] = 0;
    }
    last_trail_time_ = -1.0;
    prediction_count_ = 0;
    prediction_selection_ = -1;
    prediction_time_ = -1.0;
}

void SceneRenderer::draw_sphere(const CelestialBody &body, const Vec3 &position,
                                float emissive, const double tidal_axis[3],
                                double tidal_amount, float temperature_glow) {
    const bool using_fallback = use_fallback_lit_ && lit_fallback_shader_.valid();
    const GlShader &shader = using_fallback ? lit_fallback_shader_ : lit_shader_;
    if (!shader.valid()) return;
    shader.use();

    double radius = body.radius;
    double flatten = clamp_double(body.flattening, 0.0, 0.35);
    Mat4 model = Mat4::translation(position.x, position.y, position.z) *
                 Mat4::rotation_x(body.axial_tilt_deg) *
                 Mat4::scale(radius, radius * (1.0 - flatten), radius) *
                 Mat4::rotation_y(body.spin_angle);
    // Normal matrix = inverse-transpose of the model's upper 3x3. Computing it
    // here avoids inverse()/transpose() in the shader, which some Windows GLSL
    // compilers reject.
    Mat4 normal_matrix =
        Mat4::rotation_x(body.axial_tilt_deg) *
        Mat4::scale(1.0 / radius, 1.0 / (radius * (1.0 - flatten)), 1.0 / radius) *
        Mat4::rotation_y(body.spin_angle);
    shader.set_mat4("uModel", model.m);
    shader.set_mat4("uNormalMatrix", normal_matrix.m);
    shader.set_vec3("uColor", body.color[0], body.color[1], body.color[2]);
    // The simplified shader has no solar branch, so make stars properly bright.
    float emissive_amount = (using_fallback && body.type == BodyType::Star)
                                ? 0.95f : emissive;
    shader.set_vec3("uEmissive", body.color[0] * emissive_amount,
                    body.color[1] * emissive_amount,
                    body.color[2] * emissive_amount);
    // The tidal axis is in world space, but the vertex shader deforms the
    // unit sphere in local space. Rotate it back through the body's tilt and
    // spin before uploading it.
    Vec3 local_tidal_axis = normalize(
        transform_point(Mat4::rotation_y(-body.spin_angle) *
                        Mat4::rotation_x(-body.axial_tilt_deg),
                        Vec3(tidal_axis[0], tidal_axis[1], tidal_axis[2])));
    shader.set_vec3("uTidalAxis",
                    static_cast<float>(local_tidal_axis.x),
                    static_cast<float>(local_tidal_axis.y),
                    static_cast<float>(local_tidal_axis.z));
    shader.set_float("uTidalAmount", static_cast<float>(tidal_amount));
    shader.set_float("uLightStrength", light_index_ >= 0 ? 1.0f : 0.0f);
    shader.set_float("uAmbient", 0.14f);
    shader.set_vec3("uLightPos", static_cast<float>(light_position_.x),
                    static_cast<float>(light_position_.y),
                    static_cast<float>(light_position_.z));
    shader.set_vec3("uCameraPos",
                    static_cast<float>(camera_position_.x),
                    static_cast<float>(camera_position_.y),
                    static_cast<float>(camera_position_.z));
    shader.set_vec3("uAtmosphere", body.atmosphere_color[0],
                    body.atmosphere_color[1], body.atmosphere_color[2]);
    shader.set_float("uCloud", static_cast<float>(body.cloud_amount));
    shader.set_float("uSeed", static_cast<float>(body.procedural_seed));
    shader.set_int("uSurfaceKind", static_cast<int>(body.surface_kind));
    shader.set_float("uTime", static_cast<float>(sim_time_));
    shader.set_float("uTemperature", static_cast<float>(body.temperature_k));

    bool saturn_rings = (body.type == BodyType::Planet &&
                         std::strcmp(body.name, "Saturn") == 0);
    shader.set_float("uRingShadow", saturn_rings ? 1.0f : 0.0f);
    if (saturn_rings) {
        double tilt = body.axial_tilt_deg * PI / 180.0;
        double c = std::cos(tilt);
        double s = std::sin(tilt);
        shader.set_vec3("uRingCenter",
                        static_cast<float>(position.x),
                        static_cast<float>(position.y),
                        static_cast<float>(position.z));
        shader.set_vec3("uRingNormal", 0.0f,
                        static_cast<float>(c), static_cast<float>(s));
        shader.set_float("uRingInner",
                         static_cast<float>(body.radius * 1.11));
        shader.set_float("uRingOuter",
                         static_cast<float>(body.radius * 2.34));
        shader.set_int("uRingDensity", 1);
        glapi::ActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ring_texture_);
    }
    if (temperature_glow > 0.0f) {
        shader.set_vec3("uEmissive",
                        body.color[0] * emissive + temperature_glow * 0.85f,
                        body.color[1] * emissive + temperature_glow * 0.16f,
                        body.color[2] * emissive + temperature_glow * 0.04f);
    }
    glapi::BindVertexArray(sphere_vao_);
    glDrawElements(GL_TRIANGLES, sphere_index_count_, GL_UNSIGNED_INT, nullptr);
    glapi::BindVertexArray(0);
    glapi::ActiveTexture(GL_TEXTURE0);
}

void SceneRenderer::draw_line_vertices(const LineVertex *vertices, int count,
                                       GLenum mode) {
    if (count <= 0 || !color_shader_.valid()) return;
    if (count > 24000) count = 24000;
    color_shader_.use();
    glapi::BindVertexArray(line_vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, line_vbo_);
    glapi::BufferSubData(GL_ARRAY_BUFFER, 0,
                         static_cast<GLsizeiptr>(sizeof(LineVertex) * count),
                         vertices);
    glDrawArrays(mode, 0, count);
    glapi::BindVertexArray(0);
}

void SceneRenderer::draw_point_vertices(const PointVertex *vertices, int count) {
    if (count <= 0 || !point_shader_.valid()) return;
    if (count > 8192) count = 8192;
    point_shader_.use();
    glapi::BindVertexArray(point_vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, point_vbo_);
    glapi::BufferSubData(GL_ARRAY_BUFFER, 0,
                         static_cast<GLsizeiptr>(sizeof(PointVertex) * count),
                         vertices);
    glDrawArrays(GL_POINTS, 0, count);
    glapi::BindVertexArray(0);
}

void SceneRenderer::draw_glow_vertices(const PointVertex *vertices, int count) {
    if (count <= 0 || !glow_shader_.valid()) return;
    if (count > 8192) count = 8192;
    glow_shader_.use();
    glapi::BindVertexArray(point_vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, point_vbo_);
    glapi::BufferSubData(GL_ARRAY_BUFFER, 0,
                         static_cast<GLsizeiptr>(sizeof(PointVertex) * count),
                         vertices);
    glDrawArrays(GL_POINTS, 0, count);
    glapi::BindVertexArray(0);
}

void SceneRenderer::draw_ring_vertices(const LineVertex *vertices, int count) {
    if (count <= 0 || !ring_shader_.valid()) return;
    if (count > 24000) count = 24000;
    ring_shader_.use();
    glapi::BindVertexArray(line_vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, line_vbo_);
    glapi::BufferSubData(GL_ARRAY_BUFFER, 0,
                         static_cast<GLsizeiptr>(sizeof(LineVertex) * count),
                         vertices);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glapi::BindVertexArray(0);
}

void SceneRenderer::draw_star_halo(const LineVertex *vertices, int count,
                                   const Vec3 &center, double radius) {
    if (count <= 0 || !star_halo_shader_.valid()) return;
    star_halo_shader_.use();
    star_halo_shader_.set_vec3("uGlowCenter", static_cast<float>(center.x),
                               static_cast<float>(center.y),
                               static_cast<float>(center.z));
    star_halo_shader_.set_float("uGlowRadius", static_cast<float>(radius));
    glapi::BindVertexArray(line_vao_);
    glapi::BindBuffer(GL_ARRAY_BUFFER, line_vbo_);
    glapi::BufferSubData(GL_ARRAY_BUFFER, 0,
                         static_cast<GLsizeiptr>(sizeof(LineVertex) * count),
                         vertices);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glapi::BindVertexArray(0);
}

int SceneRenderer::find_light_source(const Universe &universe) const {
    // Use the most luminous star rather than a hard-coded name. This keeps
    // scene lighting consistent with the thermal model, which already sums
    // the flux from every star.
    int best = -1;
    double best_luminosity = -1.0;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        if (body.type != BodyType::Star || body.mass <= 0.0) continue;
        double luminosity = body.mass * body.mass * body.mass *
                            std::sqrt(body.mass);
        if (luminosity > best_luminosity) {
            best_luminosity = luminosity;
            best = i;
        }
    }
    return best;
}

void SceneRenderer::set_light(const Universe &universe) {
    light_index_ = find_light_source(universe);
    if (light_index_ >= 0) {
        const CelestialBody &sun = universe.body(light_index_);
        light_position_ = Vec3(sun.x, sun.y, sun.z);
    } else {
        light_position_ = Vec3(0.0, 0.0, 0.0);
    }
}

void SceneRenderer::draw_orbits(const Universe &universe,
                                const SelectionState &selection) {
    int default_primary = light_index_;
    if (default_primary < 0) default_primary = universe.heaviestBody();

    bool close_focus = false;
    if (selection.follow_selected &&
        selection.selected_body_index >= 0 &&
        selection.selected_body_index < MAX_OBJECTS &&
        universe.isActive(selection.selected_body_index)) {
        const CelestialBody &focus_body =
            universe.body(selection.selected_body_index);
        double focus_position[3];
        MoonSystem::visualPosition(universe, selection.selected_body_index,
                                   focus_position);
        double focus_distance = length(camera_position_ -
            Vec3(focus_position[0], focus_position[1], focus_position[2]));
        close_focus = focus_distance <
                      std::max(0.25, focus_body.radius * 20.0);
    }

    LineVertex orbit[160];
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        if (close_focus && i != selection.selected_body_index) continue;
        const CelestialBody &body = universe.body(i);
        const MoonOrbit &moon = universe.moon(i);

        int primary = -1;
        double visual_scale = 1.0;
        if (moon.parent_index >= 0 && moon.parent_index < MAX_OBJECTS &&
            universe.isActive(moon.parent_index)) {
            primary = moon.parent_index;
            visual_scale = moon.visual_scale > 0.0 ? moon.visual_scale : 1.0;
        } else if (body.type == BodyType::Planet ||
                   body.type == BodyType::Asteroid ||
                   body.type == BodyType::Comet) {
            primary = default_primary;
        } else {
            continue;
        }
        if (primary < 0 || primary == i) continue;

        if (moon.parent_index >= 0) {
            const CelestialBody &parent = universe.body(moon.parent_index);
            double camera_distance = length(camera_position_ -
                                            Vec3(parent.x, parent.y, parent.z));
            if (camera_distance < std::max(0.08, parent.radius * 8.0)) continue;
        }

        if (i == selection.selected_body_index) {
            double camera_distance = length(camera_position_ -
                                            Vec3(body.x, body.y, body.z));
            double hide_distance = std::max(0.05, body.radius * 20.0);
            if (camera_distance < hide_distance) continue;
        }

        const CelestialBody &p = universe.body(primary);
        Vec3 rv(body.x - p.x, body.y - p.y, body.z - p.z);
        Vec3 vv(body.vx - p.vx, body.vy - p.vy, body.vz - p.vz);
        double r = length(rv);
        if (r < 1.0e-9) continue;
        double mu = G_CONSTANT * (body.mass + p.mass);
        if (mu <= 0.0) continue;
        Vec3 h = cross(rv, vv);
        double h_len = length(h);
        if (h_len < 1.0e-12) continue;
        double energy = 0.5 * dot(vv, vv) - mu / r;
        if (energy >= 0.0) continue;

        Vec3 eVec = cross(vv, h) / mu - rv / r;
        double eccentricity = length(eVec);
        if (eccentricity >= 0.999) continue;

        Vec3 hHat = h / h_len;
        Vec3 eHat = (eccentricity > 1.0e-8) ? normalize(eVec) : normalize(rv);
        Vec3 qHat = normalize(cross(hHat, eHat));
        double semi_latus = h_len * h_len / mu;

        double center[3];
        MoonSystem::visualPosition(universe, primary, center);

        bool selected = (i == selection.selected_body_index);
        float alpha = selected ? 0.90f : 0.30f;
        float cr = 0.42f, cg = 0.56f, cb = 0.78f;
        if (body.type == BodyType::Asteroid) {
            cr = 0.62f; cg = 0.52f; cb = 0.36f;
            alpha = selected ? 0.85f : 0.22f;
        } else if (body.type == BodyType::Comet) {
            cr = 0.38f; cg = 0.72f; cb = 0.88f;
            alpha = selected ? 0.85f : 0.26f;
        } else if (moon.parent_index >= 0) {
            cr = 0.55f; cg = 0.64f; cb = 0.80f;
            alpha = selected ? 0.80f : 0.24f;
        }
        if (selected) {
            cr = 0.78f; cg = 0.90f; cb = 1.00f;
        }

        int count = 0;
        const int samples = 128;
        for (int s = 0; s <= samples; ++s) {
            double anomaly = 2.0 * PI * static_cast<double>(s) / samples;
            double denominator = 1.0 + eccentricity * std::cos(anomaly);
            if (denominator <= 1.0e-6) continue;
            double radius = semi_latus / denominator;
            Vec3 relative = eHat * (radius * std::cos(anomaly)) +
                            qHat * (radius * std::sin(anomaly));
            Vec3 position = Vec3(center[0], center[1], center[2]) +
                            relative * visual_scale;
            orbit[count++] = {static_cast<float>(position.x),
                              static_cast<float>(position.y),
                              static_cast<float>(position.z),
                              cr, cg, cb, alpha};
        }
        if (count >= 2) draw_line_vertices(orbit, count, GL_LINE_STRIP);
    }
}

void SceneRenderer::draw_starfield(const CosmeticScene &cosmetics) {
    PointVertex points[STAR_COUNT];
    int count = 0;
    for (int i = 0; i < cosmetics.starCount(); ++i) {
        const CosmeticScene::StarPoint &star = cosmetics.stars()[i];
        points[count++] = {star.x, star.y, star.z,
                           star.brightness, star.brightness, star.brightness, 1.0f,
                           1.2f + star.brightness * 1.4f};
    }
    glDisable(GL_DEPTH_TEST);
    draw_point_vertices(points, count);
    glEnable(GL_DEPTH_TEST);
}

void SceneRenderer::draw_belt(const CosmeticScene &cosmetics) {
    std::vector<PointVertex> points;
    points.reserve(cosmetics.beltCount());
    for (int i = 0; i < cosmetics.beltCount(); ++i) {
        const CosmeticScene::BeltParticle &particle = cosmetics.belt()[i];
        float shade = particle.brightness;
        float warm = 0.86f + 0.14f * static_cast<float>((i % 7) / 6.0);
        points.push_back({
            static_cast<float>(particle.x + light_position_.x),
            static_cast<float>(particle.y + light_position_.y),
            static_cast<float>(particle.z + light_position_.z),
            shade * warm, shade * 0.90f * warm, shade * 0.78f * warm,
            0.72f + 0.25f * particle.brightness, particle.size});
    }
    draw_point_vertices(points.data(), static_cast<int>(points.size()));
}

void SceneRenderer::draw_dust(const DustField &dust) {
    std::vector<PointVertex> points;
    points.reserve(dust.count());
    for (int i = 0; i < dust.count(); ++i) {
        const DustParticle &p = dust.particles()[i];
        if (!p.active) continue;
        points.push_back({
            static_cast<float>(p.x), static_cast<float>(p.y),
            static_cast<float>(p.z),
            0.60f + p.brightness * 0.25f,
            0.55f + p.brightness * 0.22f,
            0.48f + p.brightness * 0.18f,
            0.55f + p.brightness * 0.30f,
            p.size});
    }
    draw_point_vertices(points.data(), static_cast<int>(points.size()));
}

void SceneRenderer::draw_saturn_ring(const Universe &universe) {
    int saturn = universe.findByName("Saturn");
    if (saturn < 0 || !universe.isActive(saturn)) return;
    const CelestialBody &s = universe.body(saturn);
    double tilt = s.axial_tilt_deg * PI / 180.0;
    double c = std::cos(tilt);
    double sn = std::sin(tilt);
    Vec3 axis(0.0, c, sn);
    Vec3 u(1.0, 0.0, 0.0);
    Vec3 v = normalize(cross(axis, u));
    Vec3 center(s.x, s.y, s.z);

    const int angular = 192;
    const int radial = 8;
    std::vector<LineVertex> triangles;
    triangles.reserve(angular * radial * 6);

    for (int ri = 0; ri < radial; ++ri) {
        double t0 = static_cast<double>(ri) / radial;
        double t1 = static_cast<double>(ri + 1) / radial;
        double r0 = s.radius * (RING_INNER_RATIO +
                    (RING_OUTER_RATIO - RING_INNER_RATIO) * t0);
        double r1 = s.radius * (RING_INNER_RATIO +
                    (RING_OUTER_RATIO - RING_INNER_RATIO) * t1);
        for (int ai = 0; ai < angular; ++ai) {
            double a0 = 2.0 * PI * static_cast<double>(ai) / angular;
            double a1 = 2.0 * PI * static_cast<double>(ai + 1) / angular;
            Vec3 p00 = center + u * (r0 * std::cos(a0)) +
                       v * (r0 * std::sin(a0));
            Vec3 p01 = center + u * (r0 * std::cos(a1)) +
                       v * (r0 * std::sin(a1));
            Vec3 p10 = center + u * (r1 * std::cos(a0)) +
                       v * (r1 * std::sin(a0));
            Vec3 p11 = center + u * (r1 * std::cos(a1)) +
                       v * (r1 * std::sin(a1));
            auto vertex = [](const Vec3 &p) {
                return LineVertex{static_cast<float>(p.x),
                                  static_cast<float>(p.y),
                                  static_cast<float>(p.z),
                                  1.0f, 1.0f, 1.0f, 1.0f};
            };
            triangles.push_back(vertex(p00));
            triangles.push_back(vertex(p10));
            triangles.push_back(vertex(p11));
            triangles.push_back(vertex(p00));
            triangles.push_back(vertex(p11));
            triangles.push_back(vertex(p01));
        }
    }

    ring_shader_.use();
    ring_shader_.set_vec3("uRingCenter", static_cast<float>(center.x),
                          static_cast<float>(center.y),
                          static_cast<float>(center.z));
    ring_shader_.set_vec3("uRingNormal", static_cast<float>(axis.x),
                          static_cast<float>(axis.y),
                          static_cast<float>(axis.z));
    ring_shader_.set_vec3("uLightPos", static_cast<float>(light_position_.x),
                          static_cast<float>(light_position_.y),
                          static_cast<float>(light_position_.z));
    ring_shader_.set_vec3("uCameraPos", static_cast<float>(camera_position_.x),
                          static_cast<float>(camera_position_.y),
                          static_cast<float>(camera_position_.z));
    ring_shader_.set_float("uPlanetRadius", static_cast<float>(s.radius));
    ring_shader_.set_float("uInnerRatio", static_cast<float>(RING_INNER_RATIO));
    ring_shader_.set_float("uOuterRatio", static_cast<float>(RING_OUTER_RATIO));
    ring_shader_.set_int("uRingProfile", 0);
    glapi::ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ring_texture_);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    draw_ring_vertices(triangles.data(), static_cast<int>(triangles.size()));
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SceneRenderer::draw_star_glow(const Universe &universe) {
    std::vector<PointVertex> wind;
    std::vector<LineVertex> prominences;

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        if (body.type != BodyType::Star) continue;

        double pulse = 0.92 + 0.08 * std::sin(sim_time_ * 1.6 + i * 0.7);
        Vec3 star_position(body.x, body.y, body.z);
        double camera_distance = length(camera_position_ - star_position);
        if (camera_distance < 1.0e-4) camera_distance = 1.0e-4;
        double pixel_scale = static_cast<double>(viewport_height_) /
            (2.0 * camera_distance * std::tan(CAMERA_FOV * PI / 360.0));
        double base_radius = body.radius * pixel_scale;

        Vec3 view = normalize(camera_position_ - star_position);
        if (length(view) < 1.0e-6) view = Vec3(0.0, 0.0, 1.0);
        Vec3 right = normalize(cross(view, Vec3(0.0, 1.0, 0.0)));
        if (length(right) < 1.0e-6) right = Vec3(1.0, 0.0, 0.0);
        Vec3 up = normalize(cross(right, view));

        struct HaloLayer {
            double factor;
            double min_pixels;
            double max_pixels;
            float r, g, b, a;
        };
        const HaloLayer layers[4] = {
            {1.25, 20.0, 320.0, 1.00f, 1.00f, 0.97f, 1.10f},
            {1.80, 30.0, 460.0, 1.00f, 0.97f, 0.80f, 0.60f},
            {2.60, 48.0, 640.0, 1.00f, 0.84f, 0.48f, 0.24f},
            {3.80, 72.0, 850.0, 1.00f, 0.62f, 0.24f, 0.080f},
        };
        for (int layer = 0; layer < 4; ++layer) {
            double pixel_radius = clamp_double(base_radius * layers[layer].factor,
                                               layers[layer].min_pixels,
                                               layers[layer].max_pixels);
            double world_radius = pixel_radius / pixel_scale;
            Vec3 lower_left = star_position -
                              right * world_radius - up * world_radius;
            Vec3 lower_right = star_position +
                               right * world_radius - up * world_radius;
            Vec3 upper_right = star_position +
                               right * world_radius + up * world_radius;
            Vec3 upper_left = star_position -
                              right * world_radius + up * world_radius;
            auto vertex = [&](const Vec3 &p) {
                return LineVertex{static_cast<float>(p.x),
                                  static_cast<float>(p.y),
                                  static_cast<float>(p.z),
                                  layers[layer].r, layers[layer].g,
                                  layers[layer].b,
                                  layers[layer].a *
                                  static_cast<float>(pulse)};
            };
            LineVertex quad[6] = {
                vertex(lower_left), vertex(lower_right),
                vertex(upper_right), vertex(lower_left),
                vertex(upper_right), vertex(upper_left)
            };
            draw_star_halo(quad, 6, star_position, world_radius);
        }

        // Solar-wind particles stream outward from each star.
        for (int k = 0; k < 24; ++k) {
            double angle = 2.0 * PI * static_cast<double>(k) / 24.0 +
                           sim_time_ * 0.18;
            double distance = 0.05 + std::fmod(sim_time_ * 0.055 +
                                               static_cast<double>(k) * 0.013,
                                               0.28);
            double px = body.x + std::cos(angle) * distance;
            double pz = body.z + std::sin(angle) * distance;
            float fade = static_cast<float>(1.0 - distance / 0.36);
            wind.push_back({static_cast<float>(px),
                            static_cast<float>(body.y + 0.01 * std::sin(angle * 2.0 + sim_time_)),
                            static_cast<float>(pz),
                            body.color[0], body.color[1], body.color[2],
                            0.12f * fade, 1.6f});
        }

        // A few looping prominence arcs near the limb.
        for (int k = 0; k < 4; ++k) {
            double center_angle = static_cast<double>(k) * PI * 0.5 +
                                  sim_time_ * 0.08;
            LineVertex arc[9];
            for (int s = 0; s <= 8; ++s) {
                double a = center_angle - 0.22 + 0.44 * static_cast<double>(s) / 8.0;
                double rr = body.radius * (1.12 + 0.08 *
                    std::sin(static_cast<double>(s) * PI));
                arc[s] = {static_cast<float>(body.x + rr * std::cos(a)),
                          static_cast<float>(body.y + body.radius * 0.18),
                          static_cast<float>(body.z + rr * std::sin(a)),
                          1.0f, 0.30f, 0.08f, 0.42f};
            }
            prominences.insert(prominences.end(), arc, arc + 9);
        }
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    if (!wind.empty()) {
        draw_glow_vertices(wind.data(), static_cast<int>(wind.size()));
    }
    for (int i = 0; i < static_cast<int>(prominences.size()); i += 9) {
        draw_line_vertices(&prominences[i], 9, GL_LINE_STRIP);
    }
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

void SceneRenderer::draw_comet_tails(const Universe &universe) {
    LineVertex ion[2];
    LineVertex dust[9];
    PointVertex coma[1];
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        if (body.type != BodyType::Comet) continue;
        double length = body.tail_length > 0.0 ? body.tail_length : 0.08;
        double speed = std::sqrt(body.vx * body.vx + body.vy * body.vy +
                                 body.vz * body.vz);
        double vx = speed > 1.0e-9 ? body.vx / speed : 0.0;
        double vy = speed > 1.0e-9 ? body.vy / speed : 0.0;
        double vz = speed > 1.0e-9 ? body.vz / speed : 0.0;

        ion[0] = {static_cast<float>(body.x), static_cast<float>(body.y),
                  static_cast<float>(body.z), 0.65f, 0.92f, 1.0f, 0.80f};
        ion[1] = {static_cast<float>(body.x + body.tail_dir[0] * length),
                  static_cast<float>(body.y + body.tail_dir[1] * length),
                  static_cast<float>(body.z + body.tail_dir[2] * length),
                  0.30f, 0.62f, 1.0f, 0.02f};

        for (int s = 0; s <= 8; ++s) {
            double t = static_cast<double>(s) / 8.0;
            double dust_length = length * 0.85;
            double lag = 0.35 * t * t * length;
            dust[s] = {static_cast<float>(body.x + body.tail_dir[0] * dust_length * t -
                                          vx * lag),
                       static_cast<float>(body.y + body.tail_dir[1] * dust_length * t -
                                          vy * lag),
                       static_cast<float>(body.z + body.tail_dir[2] * dust_length * t -
                                          vz * lag),
                       1.0f, 0.78f, 0.45f,
                       static_cast<float>(0.48 * (1.0 - t))};
        }

        coma[0] = {static_cast<float>(body.x), static_cast<float>(body.y),
                   static_cast<float>(body.z), 0.65f, 0.92f, 1.0f, 0.28f,
                   static_cast<float>(10.0 + body.coma_radius * 900.0)};

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        draw_line_vertices(ion, 2, GL_LINES);
        draw_line_vertices(dust, 9, GL_LINE_STRIP);
        glDepthMask(GL_FALSE);
        draw_point_vertices(coma, 1);
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }
}

void SceneRenderer::draw_meteor_streaks(const Universe &universe) {
    LineVertex line[2];
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        if (body.type != BodyType::Meteor) continue;
        double speed = std::sqrt(body.vx * body.vx + body.vy * body.vy +
                                 body.vz * body.vz);
        if (speed < 1.0e-6) continue;
        double length = std::min(0.06, speed * 0.012);
        line[0] = {static_cast<float>(body.x), static_cast<float>(body.y),
                   static_cast<float>(body.z), 1.0f, 0.9f, 0.6f, 0.6f};
        line[1] = {static_cast<float>(body.x - body.vx / speed * length),
                   static_cast<float>(body.y - body.vy / speed * length),
                   static_cast<float>(body.z - body.vz / speed * length),
                   1.0f, 0.55f, 0.15f, 0.0f};
        draw_line_vertices(line, 2, GL_LINES);
    }
}

void SceneRenderer::draw_black_hole(const CelestialBody &body,
                                    const Vec3 &position) {
    double horizon = body.horizon_radius > 0.0 ? body.horizon_radius : body.radius;
    if (horizon <= 0.0) return;

    double tilt = body.axial_tilt_deg * PI / 180.0;
    Vec3 axis(0.0, std::cos(tilt), std::sin(tilt));
    Vec3 u(1.0, 0.0, 0.0);
    Vec3 v = normalize(cross(axis, u));
    Vec3 view_dir = normalize(camera_position_ - position);
    Vec3 right = normalize(cross(view_dir, Vec3(0.0, 1.0, 0.0)));
    Vec3 up = cross(right, view_dir);

    std::vector<LineVertex> disk;
    const int angular = 160;
    const int radial = 2;
    // Match the Equation Lab: accretion starts at the ISCO (3 r_s), and the
    // photon ring is drawn at the photon sphere (1.5 r_s).
    double inner = horizon * 3.0;
    double outer = horizon * 6.0;
    for (int ri = 0; ri < radial; ++ri) {
        double t0 = static_cast<double>(ri) / radial;
        double t1 = static_cast<double>(ri + 1) / radial;
        double r0 = inner + (outer - inner) * t0;
        double r1 = inner + (outer - inner) * t1;
        float mix0 = static_cast<float>(t0);
        float mix1 = static_cast<float>(t1);
        for (int ai = 0; ai < angular; ++ai) {
            double a0 = 2.0 * PI * static_cast<double>(ai) / angular;
            double a1 = 2.0 * PI * static_cast<double>(ai + 1) / angular;
            Vec3 p00 = position + u * (r0 * std::cos(a0)) + v * (r0 * std::sin(a0));
            Vec3 p01 = position + u * (r0 * std::cos(a1)) + v * (r0 * std::sin(a1));
            Vec3 p10 = position + u * (r1 * std::cos(a0)) + v * (r1 * std::sin(a0));
            Vec3 p11 = position + u * (r1 * std::cos(a1)) + v * (r1 * std::sin(a1));
            Vec3 tangent = normalize(cross(axis, p00 - position));
            double doppler = 0.62 + 0.38 * std::max(0.0, dot(tangent, view_dir));
            auto vertex = [&](const Vec3 &p, float mixv, float side) {
                float hue = 1.0f - mixv;
                return LineVertex{static_cast<float>(p.x),
                                  static_cast<float>(p.y),
                                  static_cast<float>(p.z),
                                  (0.92f + 0.08f * hue) * side,
                                  (0.32f + 0.62f * hue) * side,
                                  (0.08f + 0.88f * hue) * side,
                                  0.28f + 0.34f * (1.0f - mixv)};
            };
            disk.push_back(vertex(p00, mix0, static_cast<float>(doppler)));
            disk.push_back(vertex(p10, mix1, static_cast<float>(doppler)));
            disk.push_back(vertex(p11, mix1, static_cast<float>(doppler)));
            disk.push_back(vertex(p00, mix0, static_cast<float>(doppler)));
            disk.push_back(vertex(p11, mix1, static_cast<float>(doppler)));
            disk.push_back(vertex(p01, mix0, static_cast<float>(doppler)));
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    draw_line_vertices(disk.data(), static_cast<int>(disk.size()), GL_TRIANGLES);

    LineVertex ring[65];
    double photon_radius = horizon * 1.5;
    for (int i = 0; i <= 64; ++i) {
        double angle = 2.0 * PI * static_cast<double>(i) / 64.0;
        Vec3 p = position + right * (photon_radius * std::cos(angle)) +
                 up * (photon_radius * std::sin(angle));
        ring[i] = {static_cast<float>(p.x), static_cast<float>(p.y),
                   static_cast<float>(p.z),
                   0.95f, 0.82f, 1.0f, 0.85f};
    }
    draw_line_vertices(ring, 65, GL_LINE_STRIP);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

void SceneRenderer::draw_collision_debug(const Universe &universe,
                                         bool physical_collisions) {
    LineVertex ring[65];
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        double radius = effective_collision_radius(body, physical_collisions);
        if (radius <= 0.0) continue;
        double position[3];
        MoonSystem::visualPosition(universe, i, position);
        for (int plane = 0; plane < 2; ++plane) {
            for (int s = 0; s <= 64; ++s) {
                double angle = 2.0 * PI * static_cast<double>(s) / 64.0;
                double a = radius * std::cos(angle);
                double b = radius * std::sin(angle);
                double x = position[0] + a;
                double y = position[1] + (plane == 0 ? b : 0.0);
                double z = position[2] + (plane == 0 ? 0.0 : b);
                float color = physical_collisions ? 1.0f : 0.35f;
                ring[s] = {static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(z),
                           color, 0.80f, 1.0f, 0.24f};
            }
            draw_line_vertices(ring, 65, GL_LINE_STRIP);
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SceneRenderer::draw_selection(const Universe &universe,
                                   const SelectionState &selection) {
    int index = selection.selected_body_index;
    if (index < 0 || index >= MAX_OBJECTS || !universe.isActive(index)) return;
    const CelestialBody &body = universe.body(index);
    double position[3];
    MoonSystem::visualPosition(universe, index, position);
    double camera_distance = length(camera_position_ -
                                    Vec3(position[0], position[1], position[2]));
    if (camera_distance < std::max(0.08, body.radius * 8.0)) return;
    double radius = body.radius * 1.25 + 0.003;
    LineVertex ring[65];
    for (int s = 0; s <= 64; ++s) {
        double angle = 2.0 * PI * static_cast<double>(s) / 64.0;
        ring[s] = {static_cast<float>(position[0] + radius * std::cos(angle)),
                   static_cast<float>(position[1]),
                   static_cast<float>(position[2] + radius * std::sin(angle)),
                   1.0f, 1.0f, 1.0f, 0.85f};
    }
    draw_line_vertices(ring, 65, GL_LINE_STRIP);
    for (int s = 0; s <= 64; ++s) {
        double angle = 2.0 * PI * static_cast<double>(s) / 64.0;
        ring[s] = {static_cast<float>(position[0]),
                   static_cast<float>(position[1] + radius * std::cos(angle)),
                   static_cast<float>(position[2] + radius * std::sin(angle)),
                   0.75f, 0.9f, 1.0f, 0.55f};
    }
    draw_line_vertices(ring, 65, GL_LINE_STRIP);
}

void SceneRenderer::draw_hover(const Universe &universe, int index) {
    if (index < 0 || index >= MAX_OBJECTS || !universe.isActive(index)) return;
    const CelestialBody &body = universe.body(index);
    double position[3];
    MoonSystem::visualPosition(universe, index, position);
    double camera_distance = length(camera_position_ -
                                    Vec3(position[0], position[1], position[2]));
    if (camera_distance < std::max(0.08, body.radius * 8.0)) return;
    double radius = body.radius * 1.35 + 0.003;
    LineVertex ring[65];
    for (int s = 0; s <= 64; ++s) {
        double angle = 2.0 * PI * static_cast<double>(s) / 64.0;
        ring[s] = {static_cast<float>(position[0] + radius * std::cos(angle)),
                   static_cast<float>(position[1]),
                   static_cast<float>(position[2] + radius * std::sin(angle)),
                   0.55f, 0.82f, 1.0f, 0.28f};
    }
    draw_line_vertices(ring, 65, GL_LINE_STRIP);
}

void SceneRenderer::draw_spawn_marker(const UiState &ui) {
    if (!ui.spawn.cursor_valid) return;
    LineVertex ring[33];
    for (int s = 0; s <= 32; ++s) {
        double angle = 2.0 * PI * static_cast<double>(s) / 32.0;
        ring[s] = {static_cast<float>(ui.spawn.cursor_x + 0.012 * std::cos(angle)),
                   0.0f,
                   static_cast<float>(ui.spawn.cursor_z + 0.012 * std::sin(angle)),
                   0.8f, 0.9f, 1.0f, 0.75f};
    }
    draw_line_vertices(ring, 33, GL_LINE_STRIP);
}

void SceneRenderer::draw_gravity_grid(const Universe &universe) {
    const int N = 40;
    const double span = 1.35;
    double heights[N + 1][N + 1];
    for (int iz = 0; iz <= N; ++iz) {
        for (int ix = 0; ix <= N; ++ix) {
            double x = -span + 2.0 * span * static_cast<double>(ix) / N;
            double z = -span + 2.0 * span * static_cast<double>(iz) / N;
            double potential = 0.0;
            for (int i = 0; i < MAX_OBJECTS; ++i) {
                if (!universe.isActive(i)) continue;
                const CelestialBody &body = universe.body(i);
                double dx = x - body.x;
                double dy = -body.y;
                double dz = z - body.z;
                double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                potential -= G_CONSTANT * body.mass / (distance + 0.02);
            }
            heights[iz][ix] = -0.12 - std::min(0.85, std::abs(potential) * 0.075);
        }
    }
    std::vector<LineVertex> lines;
    lines.reserve(N * N * 4);
    for (int iz = 0; iz <= N; ++iz) {
        for (int ix = 0; ix < N; ++ix) {
            double x0 = -span + 2.0 * span * static_cast<double>(ix) / N;
            double x1 = -span + 2.0 * span * static_cast<double>(ix + 1) / N;
            double z = -span + 2.0 * span * static_cast<double>(iz) / N;
            double y0 = heights[iz][ix];
            double y1 = heights[iz][ix + 1];
            float c = static_cast<float>(0.18 + 0.55 * std::min(1.0, -y0));
            lines.push_back({static_cast<float>(x0), static_cast<float>(y0),
                             static_cast<float>(z), c * 0.55f, c * 0.75f, c, 0.30f});
            lines.push_back({static_cast<float>(x1), static_cast<float>(y1),
                             static_cast<float>(z), c * 0.55f, c * 0.75f, c, 0.30f});
        }
    }
    for (int ix = 0; ix <= N; ++ix) {
        for (int iz = 0; iz < N; ++iz) {
            double z0 = -span + 2.0 * span * static_cast<double>(iz) / N;
            double z1 = -span + 2.0 * span * static_cast<double>(iz + 1) / N;
            double x = -span + 2.0 * span * static_cast<double>(ix) / N;
            double y0 = heights[iz][ix];
            double y1 = heights[iz + 1][ix];
            float c = static_cast<float>(0.18 + 0.55 * std::min(1.0, -y0));
            lines.push_back({static_cast<float>(x), static_cast<float>(y0),
                             static_cast<float>(z0), c * 0.55f, c * 0.75f, c, 0.30f});
            lines.push_back({static_cast<float>(x), static_cast<float>(y1),
                             static_cast<float>(z1), c * 0.55f, c * 0.75f, c, 0.30f});
        }
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    draw_line_vertices(lines.data(), static_cast<int>(lines.size()), GL_LINES);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SceneRenderer::update_trails(const Universe &universe, const UiState &ui) {
    if (ui.sim_time < last_trail_time_ - 0.001) reset_trails();
    if (last_trail_time_ >= 0.0 && ui.sim_time - last_trail_time_ < 0.010) return;
    last_trail_time_ = ui.sim_time;
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) {
            trail_count_[i] = 0;
            continue;
        }
        double position[3];
        MoonSystem::visualPosition(universe, i, position);
        int head = trail_head_[i];
        trails_[i][head] = Vec3(position[0], position[1], position[2]);
        trail_head_[i] = (head + 1) % 96;
        if (trail_count_[i] < 96) ++trail_count_[i];
    }
}

void SceneRenderer::draw_trails(const Universe &universe, const UiState &ui) {
    if (!ui.show_trails) {
        if (trails_was_visible_) reset_trails();
        trails_was_visible_ = false;
        return;
    }
    if (!trails_was_visible_) {
        reset_trails();
        trails_was_visible_ = true;
    }
    update_trails(universe, ui);
    LineVertex line[96];
    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        int count = trail_count_[i];
        if (count < 2) continue;
        const CelestialBody &body = universe.body(i);
        int head = trail_head_[i];
        for (int n = 0; n < count; ++n) {
            int index = (head - count + n + 96) % 96;
            const Vec3 &p = trails_[i][index];
            float fade = static_cast<float>(n) / static_cast<float>(count - 1);
            line[n] = {static_cast<float>(p.x), static_cast<float>(p.y),
                       static_cast<float>(p.z),
                       body.color[0], body.color[1], body.color[2],
                       0.06f + 0.32f * fade};
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        draw_line_vertices(line, count, GL_LINE_STRIP);
        glDisable(GL_BLEND);
    }
}

void SceneRenderer::draw_prediction(const Universe &universe,
                                    const SelectionState &selection,
                                    const UiState &ui) {
    if (!ui.show_prediction) return;
    int selected = selection.selected_body_index;
    if (selected < 0 || selected >= MAX_OBJECTS ||
        !universe.isActive(selected)) {
        prediction_count_ = 0;
        prediction_selection_ = -1;
        return;
    }

    bool stale = (selected != prediction_selection_) ||
                 prediction_time_ < 0.0 ||
                 sim_time_ < prediction_time_ ||
                 sim_time_ - prediction_time_ >= 0.12;
    if (stale) {
        const CelestialBody &body = universe.body(selected);
        const MoonOrbit &moon = universe.moon(selected);
        bool moon_path = (moon.parent_index >= 0 &&
                          moon.parent_index < MAX_OBJECTS &&
                          universe.isActive(moon.parent_index));
        int parent = moon_path ? moon.parent_index : -1;
        Vec3 parent_physical;
        Vec3 parent_visual;
        if (moon_path) {
            const CelestialBody &parent_body = universe.body(parent);
            parent_physical = Vec3(parent_body.x, parent_body.y, parent_body.z);
            double position[3];
            MoonSystem::visualPosition(universe, parent, position);
            parent_visual = Vec3(position[0], position[1], position[2]);
        }

        Vec3 p(body.x, body.y, body.z);
        Vec3 v(body.vx, body.vy, body.vz);
        int count = 0;
        const double dt = 0.012;
        for (int step = 0; step < 600 && count < 320; ++step) {
            Vec3 acceleration(0.0, 0.0, 0.0);
            for (int i = 0; i < MAX_OBJECTS; ++i) {
                if (!universe.isActive(i) || i == selected) continue;
                const CelestialBody &other = universe.body(i);
                Vec3 d = Vec3(other.x, other.y, other.z) - p;
                double r = length(d);
                if (r < 1.0e-9) continue;
                double epsilon = (moon_path && i == parent)
                    ? MOON_SOFTENING : SOFTENING;
                double softened = r + epsilon;
                double factor = G_CONSTANT * other.mass /
                                (softened * softened * r);
                acceleration = acceleration + d * factor;
            }
            v = v + acceleration * dt;
            p = p + v * dt;

            if (step % 2 == 0) {
                Vec3 rendered = p;
                if (moon_path) {
                    double scale = moon.visual_scale > 0.0
                        ? moon.visual_scale : 1.0;
                    rendered = parent_visual + (p - parent_physical) * scale;
                }
                prediction_points_[count++] = rendered;
            }
            if (p.x * p.x + p.y * p.y + p.z * p.z > 25.0) break;
        }
        prediction_count_ = count;
        prediction_selection_ = selected;
        prediction_time_ = sim_time_;
    }

    if (prediction_count_ < 2) return;
    LineVertex line[320];
    for (int i = 0; i < prediction_count_; ++i) {
        const Vec3 &p = prediction_points_[i];
        float fade = 1.0f - static_cast<float>(i) /
                     static_cast<float>(prediction_count_ - 1);
        line[i] = {static_cast<float>(p.x), static_cast<float>(p.y),
                   static_cast<float>(p.z),
                   0.35f, 0.85f, 1.0f, 0.12f + 0.58f * fade};
    }
    glEnable(GL_BLEND);
    draw_line_vertices(line, prediction_count_, GL_LINE_STRIP);
    glDisable(GL_BLEND);
}

void SceneRenderer::draw_effects(const Effects &effects) {
    if (effects.particle_count() > 0) {
        std::vector<PointVertex> points;
        points.reserve(effects.particle_count());
        for (int i = 0; i < effects.particle_count(); ++i) {
            const EffectParticle &particle = effects.particles()[i];
            float fade = static_cast<float>(particle.life / particle.max_life);
            points.push_back({static_cast<float>(particle.x),
                              static_cast<float>(particle.y),
                              static_cast<float>(particle.z),
                              particle.color[0], particle.color[1],
                              particle.color[2], fade,
                              particle.size * (0.5f + 0.5f * fade)});
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        draw_point_vertices(points.data(), static_cast<int>(points.size()));
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }

    for (int i = 0; i < effects.shockwave_count(); ++i) {
        const EffectShockwave &wave = effects.shockwaves()[i];
        float fade = static_cast<float>(wave.life / wave.max_life);
        Vec3 normal = normalize(Vec3(wave.nx, wave.ny, wave.nz));
        Vec3 helper = (std::fabs(normal.y) < 0.9) ? Vec3(0.0, 1.0, 0.0)
                                                  : Vec3(1.0, 0.0, 0.0);
        Vec3 u = normalize(cross(normal, helper));
        Vec3 v = normalize(cross(normal, u));
        LineVertex ring[65];
        for (int s = 0; s <= 64; ++s) {
            double angle = 2.0 * PI * static_cast<double>(s) / 64.0;
            Vec3 p = Vec3(wave.x, wave.y, wave.z) +
                     u * (wave.radius * std::cos(angle)) +
                     v * (wave.radius * std::sin(angle));
            ring[s] = {static_cast<float>(p.x), static_cast<float>(p.y),
                       static_cast<float>(p.z),
                       wave.color[0], wave.color[1], wave.color[2], fade * 0.8f};
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        draw_line_vertices(ring, 65, GL_LINE_STRIP);
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }
}

void SceneRenderer::collect_labels(const Universe &universe,
                                   const OrbitCamera &camera,
                                   const SelectionState &selection,
                                   const UiState &ui) {
    label_count_ = 0;
    if (!ui.show_labels) return;

    bool close_focus = false;
    if (selection.follow_selected &&
        selection.selected_body_index >= 0 &&
        selection.selected_body_index < MAX_OBJECTS &&
        universe.isActive(selection.selected_body_index)) {
        const CelestialBody &focus_body =
            universe.body(selection.selected_body_index);
        double focus_position[3];
        MoonSystem::visualPosition(universe, selection.selected_body_index,
                                   focus_position);
        Vec3 eye = camera.eye(universe, selection);
        double focus_distance = length(eye - Vec3(focus_position[0],
                                                  focus_position[1],
                                                  focus_position[2]));
        close_focus = focus_distance <
                      std::max(0.25, focus_body.radius * 20.0);
    }

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        if (close_focus && i != selection.selected_body_index) continue;
        if ((body.type == BodyType::Asteroid || body.type == BodyType::Meteor ||
             body.type == BodyType::Moon) &&
            i != selection.selected_body_index) {
            continue;
        }
        double position[3];
        MoonSystem::visualPosition(universe, i, position);
        double sx = 0.0, sy = 0.0, sz = 0.0;
        double label_world[3] = {position[0], position[1] + body.radius + 0.020,
                                 position[2]};
        if (!camera.project(universe, selection, label_world, ui, sx, sy, sz)) {
            continue;
        }
        if (sx < 0.0 || sx >= ui.sim_width || sy < 120.0 ||
            sy > static_cast<double>(ui.win_height - 190)) {
            continue;
        }
        ScreenLabel &label = labels_[label_count_++];
        label.x = static_cast<int>(sx);
        label.y = static_cast<int>(sy);
        label.selected = (i == selection.selected_body_index);
        label.priority = label.selected ? 2 : (body.type == BodyType::Star ? 0 : 1);
        copy_name(label.text, body.name);
        if (label.selected) {
            label.color[0] = 1.0f;
            label.color[1] = 0.95f;
            label.color[2] = 0.60f;
        } else {
            label.color[0] = 0.62f;
            label.color[1] = 0.68f;
            label.color[2] = 0.80f;
        }
        if (label_count_ >= MAX_OBJECTS) break;
    }
}

void SceneRenderer::render(Universe &universe, OrbitCamera &camera,
                           const CosmeticScene &cosmetics, const DustField &dust,
                           const Effects &effects, const SelectionState &selection,
                           UiState &ui, bool physical_collisions) {
    update_sim_width(ui);
    viewport_height_ = ui.win_height;
    glViewport(0, 0, ui.sim_width, ui.win_height);
    glClearColor(0.005f, 0.008f, 0.020f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    set_light(universe);
    sim_time_ = ui.sim_time;
    camera_position_ = camera.eye(universe, selection);
    Vec3 eye = camera_position_;
    Vec3 target = camera.target(universe, selection);
    double shake = effects.shake_amount();
    if (shake > 0.0) {
        eye.x += (random_unit() - 0.5) * shake;
        eye.y += (random_unit() - 0.5) * shake;
        eye.z += (random_unit() - 0.5) * shake;
    }
    Mat4 view = Mat4::look_at(eye, target, Vec3(0.0, 1.0, 0.0));
    Mat4 projection = camera.projection(static_cast<double>(ui.sim_width) /
                                        static_cast<double>(ui.win_height));
    Mat4 view_projection = projection * view;

    color_shader_.use();
    color_shader_.set_mat4("uViewProj", view_projection.m);
    point_shader_.use();
    point_shader_.set_mat4("uViewProj", view_projection.m);
    glow_shader_.use();
    glow_shader_.set_mat4("uViewProj", view_projection.m);
    ring_shader_.use();
    ring_shader_.set_mat4("uViewProj", view_projection.m);
    star_halo_shader_.use();
    star_halo_shader_.set_mat4("uViewProj", view_projection.m);
    lit_shader_.use();
    lit_shader_.set_mat4("uViewProj", view_projection.m);
    lit_fallback_shader_.use();
    lit_fallback_shader_.set_mat4("uViewProj", view_projection.m);

    draw_starfield(cosmetics);
    if (ui.show_gravity_grid) draw_gravity_grid(universe);
    draw_orbits(universe, selection);
    draw_trails(universe, ui);
    draw_prediction(universe, selection, ui);
    draw_belt(cosmetics);
    if (ui.show_dust) draw_dust(dust);

    for (int i = 0; i < MAX_OBJECTS; ++i) {
        if (!universe.isActive(i)) continue;
        const CelestialBody &body = universe.body(i);
        double position[3];
        MoonSystem::visualPosition(universe, i, position);

        float emissive = 0.0f;
        float temperature_glow = 0.0f;
        double tidal_amount = 0.0;
        double tidal_axis[3] = {1.0, 0.0, 0.0};

        if (body.type == BodyType::Star) {
            emissive = 0.42f;
        } else if (body.type == BodyType::BlackHole) {
            emissive = 0.0f;
        } else {
            if (ui.show_thermal && body.temperature_k > 320.0) {
                temperature_glow = static_cast<float>(
                    clamp_double((body.temperature_k - 320.0) / 1500.0, 0.0, 1.0));
            }
        }
        if (ui.show_tidal) {
            tidal_amount = clamp_double(body.tidal_ratio * 1000.0, 0.0, 0.18);
            tidal_axis[0] = body.tidal_axis[0];
            tidal_axis[1] = body.tidal_axis[1];
            tidal_axis[2] = body.tidal_axis[2];
        }

        draw_sphere(body, Vec3(position[0], position[1], position[2]),
                    emissive, tidal_axis, tidal_amount, temperature_glow);

        if (temperature_glow > 0.04f) {
            PointVertex heat = {
                static_cast<float>(position[0]), static_cast<float>(position[1]),
                static_cast<float>(position[2]),
                1.0f, 0.42f, 0.12f,
                0.10f + temperature_glow * 0.18f,
                static_cast<float>(12.0 + body.radius * 1600.0 * temperature_glow)};
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            draw_point_vertices(&heat, 1);
            glDepthMask(GL_TRUE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_BLEND);
        }

        if (body.type == BodyType::BlackHole) {
            draw_black_hole(body, Vec3(position[0], position[1], position[2]));
            if (body.collapse_flash > 0.01) {
                PointVertex flash = {
                    static_cast<float>(position[0]),
                    static_cast<float>(position[1]),
                    static_cast<float>(position[2]),
                    0.85f, 0.60f, 1.0f,
                    static_cast<float>(body.collapse_flash * 0.8),
                    static_cast<float>(40.0 + body.collapse_flash * 260.0)};
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDepthMask(GL_FALSE);
                draw_point_vertices(&flash, 1);
                glDepthMask(GL_TRUE);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_BLEND);
            }
        }
    }

    draw_saturn_ring(universe);
    draw_star_glow(universe);
    draw_comet_tails(universe);
    draw_meteor_streaks(universe);
    draw_effects(effects);
    if (ui.show_collision_debug) {
        draw_collision_debug(universe, physical_collisions);
    }
    draw_selection(universe, selection);
    if (ui.hover_body_index >= 0 &&
        ui.hover_body_index != selection.selected_body_index) {
        draw_hover(universe, ui.hover_body_index);
    }
    draw_spawn_marker(ui);

    collect_labels(universe, camera, selection, ui);
    glDisable(GL_DEPTH_TEST);
}

} // namespace spacesim
