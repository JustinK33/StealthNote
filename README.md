# QuickDraft

QuickDraft is a lightweight desktop popup for fast notes and clipboard history.

## Build (macOS, Qt 6)

```bash
brew install qt
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
open build/QuickDraft.app
```

## Global hotkey

Default: fn + Space. Click the keybind pill to change it.

Global hotkeys use macOS Accessibility permissions. If the shortcut does not work, enable QuickDraft in System Settings > Privacy and Security > Accessibility.

## Menu bar + search

QuickDraft adds a menu bar icon with quick actions (show/hide, new note, auto-hide toggle, auto-pin #pin tags).
Use the search field to filter notes or type #tag to filter by tag. Notes with #pin are auto-pinned when enabled.

## Data

Notes and clipboard history are stored at:
~/Library/Application Support/QuickDraft/quickdraft.json
