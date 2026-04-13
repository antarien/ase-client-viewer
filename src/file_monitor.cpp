/**
 * @file        file_monitor.cpp
 * @brief       Debounced Gio::FileMonitor implementation
 * @description Every filesystem event resets a 500 ms timeout; when the
 *              timeout finally fires, the stored callback runs exactly once.
 *              Copying the slot instead of borrowing it keeps the watcher
 *              alive for the full window lifetime.
 *
 * @module      ase-client-viewer
 * @layer       5
 */

#include <viewer/file_monitor.hpp>

#include <giomm/file.h>
#include <glibmm/main.h>

namespace ase::viewer {

void FileMonitor::watch_impl(const std::string& root_path, sigc::slot<void()> on_changed) {
    stop();
    m_callback = std::move(on_changed);
    auto dir = Gio::File::create_for_path(root_path);
    m_monitor = dir->monitor_directory(Gio::FileMonitorFlags::WATCH_MOVES);
    m_monitor->signal_changed().connect(
        [this](const Glib::RefPtr<Gio::File>&,
               const Glib::RefPtr<Gio::File>&,
               Gio::FileMonitor::Event) {
            if (m_debounce.connected()) m_debounce.disconnect();
            m_debounce = Glib::signal_timeout().connect([this]() -> bool {
                if (m_callback) m_callback();
                return false;
            }, 500);
        });
}

void FileMonitor::stop() {
    if (m_debounce.connected()) m_debounce.disconnect();
    if (m_monitor) {
        m_monitor->cancel();
        m_monitor.reset();
    }
}

}  // namespace ase::viewer
