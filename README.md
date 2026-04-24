# ase-client-viewer

[![Layer](https://img.shields.io/badge/Layer-5%20Client-purple.svg)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![GTK4](https://img.shields.io/badge/UI-GTK4%2Fgtkmm-green.svg)]()

> Native Markdown documentation viewer with TECH & DESIGN modes

Part of [ASE - Antares Simulation Engine](../../..)

## Overview

The **ase-client-viewer** is a native GTK4 desktop application for viewing ASE documentation. It provides two rendering modes:

- **TECH mode:** Markdown rendering with headings, paragraphs, code blocks, tables, lists, callouts, diff blocks
- **DSGN mode:** CMS DSL directives — parser NOT YET IMPLEMENTED (directives render as plain text)

## Working Features

- Native Wayland support (Hyprland)
- GTK4 + libadwaita (Settings, Dark Mode)
- Cairo rendering for markdown content (headings, paragraphs, code, tables, lists, blockquotes, callouts, diff)
- Pango text layout
- File navigation tree with search
- Live reload via file watcher
- TECH/DSGN mode toggle (sidebar switches between tech/ and cms/ directories)
- Read/unread tracking
- Keyboard shortcuts (Ctrl+F, Esc, F5)
- Settings window (Adw::PreferencesWindow)
- AUR PKGBUILD (not verified)
- CLI: `ase mdv -B -R`

## Not Yet Implemented

- **Directive Parser** — `pass_directives()` is a stub, DSGN mode non-functional
- **Inline Extensions** — `[[wiki]]`, `{{glossary}}`, `{ref:}`, `:tip[]{}` not parsed
- **Syntax Highlighting** — tree-sitter not integrated (stub file exists)
- **Math Rendering** — MicroTeX not integrated
- **Diagram Rendering** — Graphviz/muparser not integrated
- **NerdFont Icons** in rendered content
- **Image Rendering** — Gdk::Pixbuf not implemented
- **Visual Parity** with web viewer

## Usage

```bash
ase-viewer /path/to/docs/       # Directory mode (full sidebar)
ase-viewer document.md           # Single file mode
ase-viewer                       # Default: opens docs/ase-docs/tech/
```

## Build

```bash
./build.sh                       # Full build
./build.sh clean                 # Clean (preserves _deps)
./build.sh fullclean             # Full clean
./build.sh debug                 # Debug build
./build.sh run                   # Build + run
```

## Dependencies

- **gtkmm-4.0** (system)
- **libadwaita-1** (system)
- **ase-markdown** (Layer 1 parser, FetchContent via ase-alloc + cmark-gfm)

## Dev Status & Explorer Integration (Pre-Release)

The viewer is **not yet installed system-wide** — `packaging/ase-viewer.desktop` + `PKGBUILD` exist but are not published to AUR. The binary lives at `clients/ase-client-viewer/build/bin/ase-viewer` and is not on `PATH`.

To make the dev build discoverable by `ase-client-explorer` (Open-With dialog, file-association mapping via Gio `GAppInfo`), register a user-scope `.desktop` entry with an **absolute `Exec=` path**:

```bash
cat > ~/.local/share/applications/ase-viewer.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=ASE TECH & DESIGN Viewer (dev)
GenericName=Markdown Viewer
Comment=Native documentation viewer with TECH and DESIGN modes (dev build)
Exec=/mnt/code/SRC/GITHUB/ase/clients/ase-client-viewer/build/bin/ase-viewer %F
Terminal=false
MimeType=text/markdown;text/x-markdown;
Categories=Documentation;Development;TextEditor;
Keywords=markdown;documentation;ase;viewer;
StartupWMClass=com.antarien.ase.viewer
EOF
update-desktop-database ~/.local/share/applications/
```

After registration, the viewer appears in `gio mime text/markdown` under "Recommended applications" and in Explorer's Open-With dialog. Explorer persists the chosen mapping in `~/.config/ase/explorer/file-associations.json`.

**Rollback:**

```bash
rm ~/.local/share/applications/ase-viewer.desktop
update-desktop-database ~/.local/share/applications/
```

Replace this entry with the official system-wide install (`/usr/share/applications/ase-viewer.desktop` via PKGBUILD) once the viewer reaches release status.
