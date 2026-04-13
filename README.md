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
