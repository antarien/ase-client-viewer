/**
 * @file        engine.cpp
 * @brief       Viewer engine implementation - lifecycle, input bridge, frame count
 * @description Implements the platform-agnostic surface of engine.hpp. The
 *              lifecycle functions keep the frame counter, and the input bridge
 *              accepts every event a shell can deliver.
 *
 *              THE INPUT FUNCTIONS TAKE THEIR ARGUMENTS AND DO NOTHING WITH
 *              THEM YET, and the `(void)` casts say so out loud instead of
 *              leaving a warning behind. The surface exists first so a shell can
 *              be written against it; whoever gives an event its effect changes
 *              one body here and no caller.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/ui/screen
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-12
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
 */

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
