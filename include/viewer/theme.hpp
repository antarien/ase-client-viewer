#pragma once

/**
 * @file        theme.hpp
 * @brief       Dark-theme CSS generator for the viewer
 * @description Single responsibility: return the CSS string installed once at
 *              application startup. The CSS is built from the ase::colors
 *              SSOT (sha-web-styles/src/colors.ts via the generated
 *              colors.hpp header) so the desktop palette stays in sync
 *              with the web client without manual hex updates.
 *
 *              No widget code lives here - the caller feeds generate_css()
 *              to ase::gtk::CssProvider::load_from_data().
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <string>

namespace ase::viewer::theme {

/** Build the viewer's dark-theme CSS by interpolating ase::colors constants. */
std::string generate_css();

}  // namespace ase::viewer::theme
