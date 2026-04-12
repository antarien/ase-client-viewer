#pragma once

/**
 * Viewer Engine Interface
 *
 * Platform-agnostic entry points for the viewer application.
 * Platform shells (GTK4 desktop) call these functions to drive the viewer.
 * Mirrors the aet-client-native engine.hpp pattern.
 */

#include <cstdint>

namespace ase::viewer::engine {

    // Application lifecycle
    void init(int width, int height);
    void tick(float dt);
    void resize(int width, int height);
    void destroy();

    // Input bridge (platform → engine)
    void key(int action, int keycode, int scancode, int mods);
    void mouse_move(float x, float y, float dx, float dy);
    void mouse_button(int button, int action, float x, float y);
    void scroll(float dx, float dy);

    // State queries
    uint64_t frame_count();

}  // namespace ase::viewer::engine
