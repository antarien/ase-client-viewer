#pragma once

/**
 * @file        engine.hpp
 * @brief       Viewer engine interface - the platform-agnostic entry points
 * @description Declares the functions a platform shell calls to drive the
 *              viewer: the application lifecycle (init, tick, resize, destroy),
 *              the input bridge from the platform into the engine (key, mouse
 *              move, mouse button, scroll) and the frame counter as a state
 *              query.
 *
 *              THE SHELL KNOWS THE ENGINE, NEVER THE REVERSE: the GTK4 desktop
 *              shell calls in, and nothing here names GTK. The file mirrors the
 *              engine.hpp of aet-client-native so a second shell can be written
 *              against the same surface.
 *
 * @module      ase-client-viewer
 * @layer       5 (Servers & Clients)
 * @category    render/ui/screen
 * @usecase     usecase:100008 - render the knowledge stock
 * @created     2026-04-12
 * @modified    2026-10-13
 * @version     00.00.35.00035 [core]
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
