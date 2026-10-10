# Hunk

Local merge-request diff for macOS and Linux. Qt 6, side-by-side, with review comments you can paste to an LLM agent.

Copyright © 2026 Peter Adrianov. Licensed under the [MIT License](LICENSE).

## Features

- Open a git repository and read the branch like a pull request. File → Open Recent lists repositories you opened, newest first. The toolbar shows the repository path in the normal text color. The dock and the app switcher on macOS show the Hunk icon sharp, in the system rounded shape, and the taskbar on Linux shows it
- File list grouped by folder, and the diff lists files in that same order. Filter files matches names. Search in files, under that, matches file content, highlights it in the diff, and leaves those lines unfolded. Two panes with a draggable border (double-click evens it), wrapped lines, syntax colors (keywords, methods, variables, strings, comments, and markdown headings, bold, italic, strike, code, and links), word highlights, sticky file header. A markdown file's title carries its added and removed counts and switches between that diff and a GitHub-style document (headings, tables, task lists, and highlighted code); a change shows the old and new documents side by side. Right-click a file in the list to copy its relative path. A file you already reviewed is highlighted when its diff changes again. Lines from that review stay a lighter green and red; lines changed since then are stronger
- A changed block stays open when it has 500 unchanged lines or fewer, matching the diff's own context. Longer blocks collapse to a few lines beside each change, and lines outside the block stay folded. Click the bar to show them, or the side arrows to show 20 lines and leave the rest folded
- New files use one pane
- Compare **merge request** (three-dot from the base, including uncommitted changes on the current branch; base defaults to the branch this work was cut from), **uncommitted** (`git diff HEAD`), or **staged**. That choice is kept the next time Hunk opens. The status bar names the target when the merge request would conflict with it
- Switching back reloads the diff. Saves of tracked files from other programs, and a base that moves, show up while Hunk stays open. Ignored files are left alone. An unchanged diff stays put, including the scroll position
- Under the file list, each file's added and removed counts are green and red, and the total stays visible. A check on the right of a file marks it reviewed and hides it until that file changes. The file title in the diff has the same check on the left of the name. The totals bar has a button to unhide those files. The status bar says how many commits local `main` or `master` is ahead of or behind its origin
- Click a line number to comment. Double-click a comment in the review list to show that line in the diff. Comments stay in app settings, not in the repo
- Select text in either pane and copy it from the pane menu or with Ctrl+C
- **Copy reviews** writes markdown with `` `path:line` `` and the line text
- Diff flags match a review diff: `-w -W --no-prefix --diff-algorithm=histogram`
- **About** (the application menu on macOS, **Help** on Linux) shows the app icon, the version, and the copyright. **View → Theme** follows the system, or switches to dark or light. Controls use Qlementine, so they match on macOS and Linux. Toolbar buttons use that filled button style. Open, Refresh, and Copy reviews each show a matching icon. Tooltips use the window background and the normal text color. Diff code uses the theme monospace size; the rest of the window uses the theme text size

Untracked files are not part of `git diff`.

## Build

Needs CMake, a C++20 compiler, Qt 6.9 or newer (Widgets, Svg, Concurrent, WebEngine), and `git`. Qlementine and cmark-gfm are fetched when configuring.

macOS (Homebrew Qt):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
ctest --test-dir build --output-on-failure
```

Debian / Ubuntu:

```sh
sudo apt install build-essential cmake git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9/gcc_64
cmake --build build
```

Fedora:

```sh
sudo dnf install gcc-c++ cmake git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9/gcc_64
cmake --build build
```

## Run

```sh
./build/hunk
./build/hunk /path/to/repo
```

A path opens that repository as a merge request. With no path, Hunk opens the last repository, or the current directory.

The base box defaults to where the branch left its parent: the parent name when that commit is the branch tip (`origin/main`), or `origin/main^` / `origin/main~4` when it is older. It then lists upstream, `origin/main`, `main`, `origin/master`, and `master` when they exist. The branch box lists local branches with the newest change first, and the current branch selected. That list scrolls with a scrollbar on the menu background. The field accepts a typed ref, not only a picked branch. Typing in either box keeps that text and lists matching branches under the field. Type another ref and press Return.

Drop a repository folder on the window to open it.

## Review copy

**Copy reviews** under the review list (⇧⌘C on macOS, Ctrl+Shift+C on Linux) puts this on the clipboard:

```markdown
# Review: feature vs origin/main

File references are `path:line` on the new side.

## `src/app.cpp:42`
> return value;

The nil check is missing when the user is blank.
```

A comment on the old side is marked `(old)`, and the copy then explains that marker. Comment with ⌘↩ / Ctrl+Enter on the selected line, or click the line number. One comment per line. Select several lines and click a line number beside that selection to comment the whole interval, copied as `path:first-last`. Each review line has its own delete control. Clear reviews removes every comment. Auto cleanup, on by default, removes a review when that line's text changes or the line leaves the file. A line that only moves keeps its review, and the comment follows the new line when that text still identifies it. A span follows the same way when the whole interval still matches, even if its first line appears more than once. A span that leaves the diff but is unchanged in the file stays. The same text, or the same span, on another line does not take the review.

## Layout

- `src/DiffParse.cpp`, `DiffPath.cpp`, `WordDiff.cpp`, `DiffZip.cpp` — unified diff, paired lines, word highlights
- `src/GitRepo.cpp`, `GitCmd.cpp` — git subprocess
- `src/DiffCanvas.cpp` and the other `Diff*.cpp` paint files — side-by-side view
- `src/ReviewStore.cpp` — comments in app settings
- `src/ReviewExport.cpp` — markdown for an agent
- `src/MainWindow.cpp` and the other `Main*.cpp` files — toolbar, file list, review dock
- `tests/parse_test.cpp` — parser and export checks

## License

Copyright © 2026 Peter Adrianov

[MIT License](LICENSE).
