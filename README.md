# Hunk

Local merge-request diff for macOS and Linux. Qt 6, side-by-side, with review comments you can paste to an LLM agent.

## Features

- Open a git repository and read the branch like a pull request
- File list grouped by folder, two panes, wrapped lines, syntax colors (keywords, methods, variables, strings, comments), word highlights, sticky file header
- Long unchanged stretches collapse. Click the bar to show them, or the side arrows to show 20 lines
- New files use one pane
- Compare **merge request** (three-dot from the base, including uncommitted changes on the current branch; base defaults to the branching point), **uncommitted** (`git diff HEAD`), or **staged**
- Switching back reloads the diff. Saves from other programs, and a base that moves, show up while Hunk stays open. An unchanged diff stays put, including the scroll position
- Click a line number to comment. Comments stay in app settings, not in the repo
- **Copy reviews** writes markdown with `` `path:line` `` and the line text
- Diff flags match a review diff: `-w -W --no-prefix --diff-algorithm=histogram`

Untracked files are not part of `git diff`.

## Build

Needs CMake, a C++20 compiler, Qt 6 (Widgets, Concurrent), and `git`.

macOS (Homebrew Qt):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
ctest --test-dir build --output-on-failure
```

Debian / Ubuntu:

```sh
sudo apt install build-essential cmake qt6-base-dev git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Fedora:

```sh
sudo dnf install gcc-c++ cmake qt6-qtbase-devel git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```sh
./build/hunk
./build/hunk /path/to/repo
```

A path opens that repository as a merge request. With no path, Hunk opens the last repository, or the current directory.

The base box defaults to the branching point (where the branch left the default branch), then lists upstream, `origin/main`, `main`, `origin/master`, and `master` when they exist. The branch box lists local branches, with the current branch selected. Type another ref and press Return.

Drop a repository folder on the window to open it.

## Review copy

**Copy reviews** (⇧⌘C on macOS, Ctrl+Shift+C on Linux) puts this on the clipboard:

```markdown
# Review: feature vs origin/main

File references are `path:line` on the new side. `(old)` means the line number before the change.

## `src/app.cpp:42`
> return value;

The nil check is missing when the user is blank.
```

Comment with ⌘↩ / Ctrl+Enter on the selected line, or click the line number. One comment per line. Delete removes it.

## Layout

- `src/DiffParse.cpp`, `DiffPath.cpp`, `WordDiff.cpp`, `DiffZip.cpp` — unified diff, paired lines, word highlights
- `src/GitRepo.cpp`, `GitCmd.cpp` — git subprocess
- `src/DiffCanvas.cpp` and the other `Diff*.cpp` paint files — side-by-side view
- `src/ReviewStore.cpp` — comments in app settings
- `src/ReviewExport.cpp` — markdown for an agent
- `src/MainWindow.cpp` and the other `Main*.cpp` files — toolbar, file list, review dock
- `tests/parse_test.cpp` — parser and export checks
