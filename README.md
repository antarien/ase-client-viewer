# ase-client-viewer

[![Layer](https://img.shields.io/badge/Layer-5%20Client-purple.svg)]()
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![GTK4](https://img.shields.io/badge/UI-GTK4%2Fgtkmm-green.svg)]()
[![AUR](https://img.shields.io/badge/Package-AUR-blue.svg)]()

> Native Markdown documentation viewer with TECH & DESIGN modes

Part of [ASE - Antares Simulation Engine](../../..)

## Overview

The **ase-client-viewer** is a native GTK4 desktop application for viewing ASE documentation. It provides two rendering modes:

- **TECH mode:** Standard Markdown + callouts, math, syntax highlighting, Mermaid diagrams, ASE-Math plots
- **DSGN mode:** Full CMS DSL with 37 directives (columns, cards, tabs, accordion, timeline, etc.)

## Features

- Native Wayland support (Hyprland first-class)
- GTK4 + libadwaita (Settings, Dark Mode)
- Cairo rendering for markdown content
- Pango text layout with NerdFont icons
- tree-sitter syntax highlighting
- MicroTeX math rendering
- Graphviz diagram layout
- File navigation tree with search
- Live reload via file watcher
- AUR package for Arch Linux
- CLI: `ase mdv -B -R`

## Usage

```bash
ase-viewer /path/to/docs/       # Directory mode (full sidebar)
ase-viewer document.md           # Single file mode
ase-viewer                       # Welcome screen with directory chooser
```

## Build

```bash
./build.sh                       # Full build
cmake -B build -G Ninja && ninja -C build
```

## Dependencies

- **gtkmm-4.0** (system)
- **libadwaita-1** (system)
- **graphviz** (system)
- **ase-markdown** (Layer 1 parser)
- **EnTT** v3.13.0 (FetchContent)
- **spdlog** v1.13.0 (FetchContent)
- **tree-sitter** v0.22.6 (FetchContent)
- **MicroTeX** (FetchContent)
- **muparser** v2.3.4 (FetchContent)
