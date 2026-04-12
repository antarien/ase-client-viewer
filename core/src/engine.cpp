#include <viewer/engine.hpp>

namespace ase::viewer::engine {

static uint64_t s_frame = 0;

void init(int width, int height) {
    (void)width;
    (void)height;
    s_frame = 0;
}

void tick(float dt) {
    (void)dt;
    s_frame++;
}

void resize(int width, int height) {
    (void)width;
    (void)height;
}

void destroy() {
    s_frame = 0;
}

void key(int action, int keycode, int scancode, int mods) {
    (void)action;
    (void)keycode;
    (void)scancode;
    (void)mods;
}

void mouse_move(float x, float y, float dx, float dy) {
    (void)x;
    (void)y;
    (void)dx;
    (void)dy;
}

void mouse_button(int button, int action, float x, float y) {
    (void)button;
    (void)action;
    (void)x;
    (void)y;
}

void scroll(float dx, float dy) {
    (void)dx;
    (void)dy;
}

uint64_t frame_count() {
    return s_frame;
}

}  // namespace ase::viewer::engine
