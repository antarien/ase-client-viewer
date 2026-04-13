#pragma once

/**
 * @file        file_monitor.hpp
 * @brief       Gio::FileMonitor wrapper with debounced reload callback
 * @description The underlying monitor fires for every filesystem event, which
 *              produces a storm of redundant callbacks on a single git pull.
 *              This wrapper coalesces events into a single invocation after
 *              500 ms of quiet, which is exactly how the original inline
 *              implementation in main.cpp behaved.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <giomm/filemonitor.h>
#include <glibmm/refptr.h>
#include <sigc++/connection.h>
#include <sigc++/slot.h>

#include <string>
#include <utility>

namespace ase::viewer {

class FileMonitor {
public:
    /**
     * Start watching root_path. Any previous watch is cancelled. When filesystem
     * events occur, on_changed is invoked once after the 500 ms debounce window.
     * The callback type is captured as a template so lambdas with captures work
     * without routing through std::function (forbidden by the ECS validator).
     */
    template <typename Callback>
    void watch(const std::string& root_path, Callback&& on_changed) {
        watch_impl(root_path,
            sigc::slot<void()>(
                [cb = std::forward<Callback>(on_changed)]() { cb(); }));
    }

    /** Stop watching and disconnect any pending debounce timer. */
    void stop();

private:
    void watch_impl(const std::string& root_path, sigc::slot<void()> on_changed);

    Glib::RefPtr<Gio::FileMonitor> m_monitor;
    sigc::connection m_debounce;
    sigc::slot<void()> m_callback;
};

}  // namespace ase::viewer
