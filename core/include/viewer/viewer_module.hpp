#pragma once

/**
 * ViewerModule — ECS Module definition for the documentation viewer.
 * Registers all viewer systems into the ECS App.
 */

namespace ase::viewer {

struct ViewerModule {
    static constexpr const char* name() { return "ase-client-viewer"; }

    // void build(ecs::App& app) — registered when ECS integration is added
};

}  // namespace ase::viewer
