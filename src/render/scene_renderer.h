#ifndef SPACESIM_RENDER_SCENE_RENDERER_H
#define SPACESIM_RENDER_SCENE_RENDERER_H

#include "core/body.h"
#include "core/config.h"
#include "core/math3d.h"
#include "render/gl_shader.h"
#include "render/labels.h"

#include <array>
#include <string>

namespace spacesim {

class Universe;
class OrbitCamera;
class CosmeticScene;
class Effects;
class DustField;
struct SelectionState;
struct UiState;

class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    void init();
    void reset_trails();
    void render(Universe &universe, OrbitCamera &camera,
                const CosmeticScene &cosmetics, const DustField &dust,
                const Effects &effects, const SelectionState &selection,
                UiState &ui, bool physical_collisions);

    int labelCount() const;
    const ScreenLabel *labels() const;
    const char *rendererStatus() const { return status_.c_str(); }

private:
    struct LineVertex {
        float x, y, z;
        float r, g, b, a;
    };

    struct PointVertex {
        float x, y, z;
        float r, g, b, a;
        float size;
    };

    void create_sphere();
    void draw_sphere(const CelestialBody &body, const Vec3 &position,
                     float emissive, const double tidal_axis[3],
                     double tidal_amount, float temperature_glow);
    void draw_line_vertices(const LineVertex *vertices, int count, GLenum mode);
    void draw_point_vertices(const PointVertex *vertices, int count);
    void draw_glow_vertices(const PointVertex *vertices, int count);
    void draw_ring_vertices(const LineVertex *vertices, int count);
    void draw_star_halo(const LineVertex *vertices, int count,
                        const Vec3 &center, double radius);
    void draw_orbits(const Universe &universe, const SelectionState &selection);
    void draw_starfield(const CosmeticScene &cosmetics);
    void draw_belt(const CosmeticScene &cosmetics);
    void draw_dust(const DustField &dust);
    void draw_saturn_ring(const Universe &universe);
    void draw_star_glow(const Universe &universe);
    void draw_comet_tails(const Universe &universe);
    void draw_meteor_streaks(const Universe &universe);
    void draw_black_hole(const CelestialBody &body, const Vec3 &position);
    void draw_collision_debug(const Universe &universe, bool physical_collisions);
    void draw_selection(const Universe &universe, const SelectionState &selection);
    void draw_hover(const Universe &universe, int index);
    void draw_spawn_marker(const UiState &ui);
    void draw_gravity_grid(const Universe &universe);
    void draw_trails(const Universe &universe, const UiState &ui);
    void draw_prediction(const Universe &universe, const SelectionState &selection,
                         const UiState &ui);
    void draw_effects(const Effects &effects);
    void update_trails(const Universe &universe, const UiState &ui);
    void collect_labels(const Universe &universe, const OrbitCamera &camera,
                        const SelectionState &selection, const UiState &ui);
    int find_light_source(const Universe &universe) const;
    void set_light(const Universe &universe);

    GlShader lit_shader_;
    GlShader lit_fallback_shader_;
    GlShader color_shader_;
    GlShader point_shader_;
    GlShader glow_shader_;
    GlShader ring_shader_;
    GlShader star_halo_shader_;
    std::string status_;
    bool use_fallback_lit_;

    GLuint sphere_vao_;
    GLuint sphere_vbo_;
    GLuint sphere_ebo_;
    int sphere_index_count_;

    GLuint line_vao_;
    GLuint line_vbo_;
    GLuint point_vao_;
    GLuint point_vbo_;
    GLuint ring_texture_;

    int light_index_;
    Vec3 light_position_;
    Vec3 camera_position_;
    double sim_time_;
    int viewport_height_;

    std::array<ScreenLabel, MAX_OBJECTS> labels_;
    int label_count_;

    std::array<std::array<Vec3, 96>, MAX_OBJECTS> trails_;
    std::array<int, MAX_OBJECTS> trail_head_;
    std::array<int, MAX_OBJECTS> trail_count_;
    double last_trail_time_;
    bool trails_was_visible_;

    std::array<Vec3, 320> prediction_points_;
    int prediction_count_;
    int prediction_selection_;
    double prediction_time_;
};

} // namespace spacesim

#endif
