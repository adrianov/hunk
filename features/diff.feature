# Copyright © 2026 Peter Adrianov
# SPDX-License-Identifier: MIT

Feature: Diff view

  Scenario: Run hunk in a repository
    Given I run hunk in a git repository
    Then the window shows the last diff mode, or that repository's merge request
    And the window title shows the full path to that repository
    And the toolbar shows the path to that repository
    And toolbar labels use the normal text color
    And the Merge request menu uses the same arrow as Base and Branch
    And the dock or taskbar shows the Hunk icon
    And on macOS the dock and the app switcher show that icon sharp, in the system rounded shape

  Scenario: Opening with no path
    Given I run hunk with no path
    Then the last repository opens, or the current directory when none was opened

  Scenario: Open a repository from the File menu
    Given hunk is open
    When I choose Open and pick a repository folder
    Then that repository is on screen

  Scenario: Drop a repository on the window
    Given hunk is open
    When I drop a repository folder on the window
    Then that repository is on screen

  Scenario: Open Recent reopens a repository
    Given I have opened more than one repository
    When I open the File menu
    Then Open Recent lists those repositories, newest first
    When I choose one
    Then that repository is on screen

  Scenario: Opening another repository watches that one
    Given a repository is on screen
    When I open a different repository
    Then hunk watches the repository I opened

  Scenario: Base and branch selectors
    Given I open a repository
    Then the base selector shows the branch this work was cut from, with ^ or ~N when that cut is an older commit
    And the branch selector shows the current branch
    And the branch list puts the newest change first
    And the branch list scrolls with a scrollbar on the menu background
    And the field accepts a typed ref, not only a picked branch
    And typing in either box keeps that text in the field and lists matching branches under it
    And I can type a ref that is not listed and press Return to use it
    And Escape puts the previous ref back
    And a branch name stays readable on the toolbar, shortened only when it does not fit there
    And the context menu on either box, or on a branch in its list, copies that branch name
    And the diff is a merge request between them
    And uncommitted changes on the current branch are included

  Scenario: Uncommitted and staged diffs
    Given a repository is on screen
    When I choose Uncommitted
    Then the diff is staged and unstaged changes against HEAD
    And Uncommitted is still selected the next time hunk opens
    When I choose Staged
    Then the diff is staged changes only

  Scenario: The file list shows added and removed lines
    Given a diff is on screen
    Then files are grouped by folder
    And the diff lists those files in that same order
    And each file's added count is green and its removed count is red
    And under the file list the total added lines are green and the total removed lines are red
    And that total stays visible while the file list scrolls

  Scenario: Copy a file path from the list
    Given a diff of "src/app.rb"
    When I open the context menu on that file in the file list
    Then I can copy its relative path
    And the clipboard contains "src/app.rb"
    When I open the context menu on the "src" folder in the file list
    Then I can copy its relative path
    And the clipboard contains "src"

  Scenario: Filter the file list
    Given a diff is on screen
    When I type in Filter files
    Then the list shows only files whose name matches

  Scenario: Search the text of listed files
    Given a diff is on screen
    When I type in Search in files
    Then the list shows only files whose content matches
    And the diff highlights that text
    And matched lines are shown, not folded
    And Filter files still limits that list by name

  Scenario: A new file uses one pane
    Given a diff of a file that was added
    Then that file is shown in one pane

  Scenario: Mark a file reviewed hides it until it changes
    Given a diff is on screen
    Then each file has a check mark on the right, labeled Mark as reviewed
    And the diff file title has that check on the left of the name
    When I mark a file reviewed
    Then that file leaves the list and the diff
    And the next file in the list is selected
    And when that file was the last one, the file before it is selected
    And the totals under the list no longer include it
    When that file changes
    Then it shows in the list and the diff again
    When I open a review comment on a hidden file
    Then that file is shown again

  Scenario: Unhide reviewed files
    Given some files are marked reviewed
    Then the totals bar has a button to unhide those reviewed files, on the right
    When I unhide them
    Then the full diff is on screen again

  Scenario: The main branch differs from its origin
    Given local main or master is ahead of or behind its origin
    Then the status bar says how many commits ahead or behind it is

  Scenario: A merge request that conflicts with its target
    Given a merge request that cannot merge into its target
    Then the status bar names that target and the conflicting files

  Scenario: Switching back leaves an unchanged diff still
    Given the merge request is on screen
    When I switch back to hunk and the files are unchanged
    Then the diff stays as it was
    And the scroll position stays

  Scenario: A reviewed file stays marked after it changes
    Given I have reviewed a file in the list
    When that file changes
    Then that file is highlighted in the list
    And lines I already reviewed are a lighter green and red
    And lines that changed since then are a stronger green and red
    When I view that file again
    Then the highlight is gone

  Scenario: An update does not mark every reviewed file
    Given the file list remembers reviews from an older version
    When I open hunk after those marks are updated
    Then those files are not highlighted

  Scenario: A save in another program updates the diff
    Given the merge request is on screen
    When another program saves a tracked file
    Then the diff updates
    And the scroll position stays

  Scenario: A save of an ignored file leaves the diff still
    Given the merge request is on screen
    When another program saves a file git ignores
    Then the diff stays as it was

  Scenario: An incomplete watch retries then rests
    Given the merge request is on screen
    And hunk cannot watch every directory
    When a tracked file changes
    Then the diff updates within a few checks
    And hunk stops checking every second

  Scenario: A moved base updates the diff
    Given the merge request is on screen
    When the selected base moves to another commit
    Then the diff updates

  Scenario: A short changed block stays open
    Given a diff whose unchanged lines in the block are 500 or fewer
    Then those unchanged lines stay visible
    And unchanged lines outside that block are hidden

  Scenario: A hunk collapses when the rest of the file is missing
    Given a diff that only includes the lines git printed
    And an unchanged stretch beside a change is long
    Then that stretch collapses to a few lines beside the change

  Scenario: A long changed block collapses
    Given a diff whose unchanged lines in the block are more than 500
    Then those unchanged lines are hidden
    And a few lines stay visible beside the change
    When I click that bar
    Then the unchanged lines are shown

  Scenario: Lines above the hunk stay folded
    Given a changed file with many unchanged lines before the change
    Then the change's block stays visible
    And a bar counts the unchanged lines before that block
    When I show 20 lines from the side of that bar
    Then a bar remains for the lines still hidden

  Scenario: An indent-only line keeps each side's spaces
    Given a line whose only change is its indent
    Then the left pane shows the old indent
    And the right pane shows the new indent

  Scenario: A rewritten line stays one block
    Given a changed line where only spaces stay the same
    Then that line is shown as removed and added
    And individual words are not highlighted

  Scenario: Changed words are highlighted
    Given a changed line where a word differs
    Then that word is highlighted
    And the rest of the line is not

  Scenario: The file header stays put while scrolling
    Given a diff of a long file
    When I scroll inside that file
    Then its name stays at the top of the diff

  Scenario: Copy text from a diff pane
    Given a diff is on screen
    When I double-click a word in the left or the right pane
    Then that word is selected
    When I double-click that word again without pausing
    Then the whole line is selected
    When I select text in the left or the right pane
    And the selection crosses a gap where that pane has no line
    And I open the pane menu
    Then Copy is there, with the shortcut Ctrl+C
    When I press Ctrl+C
    Then the clipboard contains that text
    And the gap is not an empty line

  Scenario: A long line wraps inside the pane
    Given a changed line longer than the pane
    Then the rest of the line continues on the next lines in that pane
    And the other pane stays beside it

  Scenario: Drag the border between the panes
    Given a diff with a left and a right pane
    When I drag the border between those panes
    Then the border stays where I leave it
    And lines wrap inside each pane
    When I double-click that border
    Then the panes are equal again

  Scenario: Ruby names on an added line
    Given a diff of "lib/app.rb"
    When a line adds "def greet(name)"
    Then "def" is colored as a keyword
    And "greet" is colored as a method
    And "name" is colored as a variable

  Scenario: A Ruby call colors the method and its names
    Given a diff of "lib/app.rb"
    When a line adds "amount = ask_rate.zero?"
    Then "zero?" is colored as a method
    And "amount" and "ask_rate" are colored as variables

  Scenario: Hover a Ruby method
    Given a diff of a Ruby file
    When I hover a call to a method
    Then a popup shows the file and line where that method is defined
    And a definition in this file is labeled "Same file" and the line
    And the popup shows the comment above it, its signature, and its whole body, scrolled when it is long
    And that code is colored like the diff, and long lines wrap
    And shared leading spaces are removed
    When I hover that method's definition
    Then no definition popup appears

  Scenario: Jump to a Ruby definition
    Given a diff of a Ruby file
    When I hold Command on macOS, or Control on Linux, over a name that opens
    Then the pointer is a hand
    When I hold it over a name that does not open
    Then the pointer stays a text cursor
    When I Command-click a call to a method on macOS, or Control-click it on Linux
    Then the diff shows the file where that method is defined, at that line
    And that file stays listed even when it has no added or removed lines
    When I Command-click a method name on its def on macOS, or Control-click it on Linux
    Then the diff shows a call to that method
    And that call is in the application when one exists, not a spec
    And a short class name such as CatalogSource.model counts
    And for initialize that call is .new on the class

  Scenario: Hover a call on a class
    Given a diff of a Ruby file that defines its own call method
    When I hover a call such as "Products::AdditionalFieldsFromPostgres.call"
    Then a popup shows that class's call method
    And it does not show the call method in the file I am reading
    When I hover a call on a local such as "io.write"
    Then no definition popup appears

  Scenario: Hover a class DSL declaration
    Given a diff of a Ruby file
    When I hover a call such as "Post.comments", "http.active_requests", or "Telemetry::Metrics::VirtualProducts.missing_items"
    Then a popup shows that declaration, such as has_many, up_down, or counter
    When I hover add or measure on a chain such as "Telemetry::Metrics::VirtualProducts.errors.add" or "Telemetry::Metrics::VirtualProducts.operation_duration.measure"
    Then a popup shows that method on the class the declaration builds

  Scenario: Hover an ambiguous Ruby name
    Given a diff of a Ruby file
    When I hover a name that is defined in more than one file
    Then no definition popup appears

  Scenario: Hover a Ruby class
    Given a diff of a Ruby file
    When I hover a class or module name, including one nested in a module such as "Telemetry::KarafkaMonitor"
    Then a popup shows the file and line where it is defined
    And the popup shows the comment above it, the class or module line, and the whole body, scrolled when it is long
    And a nested class also shows the module around it
    When I hover User and another file also reopens class User
    Then the popup shows app/models/user.rb
    When I Command-click that name on macOS, or Control-click it on Linux
    Then the diff shows that file at that line, even when it has no added or removed lines
    When I Command-click the class name on its class or module line on macOS, or Control-click it on Linux
    Then the diff shows a file that uses that class, at that line

  Scenario: Hover a Ruby constant
    Given a diff of a Ruby file
    When I hover a constant such as "DEFAULT_PAYMENT" or ":DEFAULT_PAYMENT"
    Then a popup shows its assignment, such as "DEFAULT_PAYMENT = 'card'"
    And an array value shows every element, through the closing bracket
    When I Command-click that constant on macOS, or Control-click it on Linux
    Then the diff shows that file at that line

  Scenario: Hover an Active Record column
    Given a diff of a Ruby file
    When I hover a call such as "online_order.final_delivery_price" or "address.city"
    And I hover a column such as ":product_gold_erp_id" or "manzana_session_id"
    Then a popup shows that column from schema.rb, or from another schema file
    And the popup shows the column type and options such as precision
    And when several tables have that column, the popup shows the table named on the line, or the model file's table

  Scenario: Hover super
    Given a diff of a Ruby file
    When I hover super inside a method
    Then a popup shows that method on the parent class
    And when several classes prepend the module, the popup lists each of those methods

  Scenario: Hover a Ruby hash key
    Given a diff of a Ruby file
    When I hover a hash key, a keyword argument, or a local variable such as "normalized = path.dup" or "|(key, value), normalized|"
    Then no definition popup appears
    And hovering that name after the block opens the method

  Scenario: Markdown on an added line
    Given a diff of "README.md"
    When a line adds "# Hello"
    Then "#" is colored as a keyword
    When a line adds "see `code`"
    Then "`code`" is colored as a string
    When a line adds "[Hunk](https://example.com)"
    Then "Hunk" is colored as a method
    And "https://example.com" is colored as a string
    When a line adds "see **bold**, *italics*, and ~~gone~~"
    Then "bold" is bold
    And "italics" is italic
    And "gone" is struck through
    And the "**", "*", and "~~" marks are colored as keywords

  Scenario: A markdown file can be read rendered
    Given a diff of "README.md"
    Then Source and Rendered sit on that file's title, beside its added and removed counts
    And that title is the only heading for the file
    When I choose Rendered
    Then that file is shown as a GitHub document
    And headings, tables, and code blocks are styled
    And both sides are shown when the file changed on each side
    When I choose Source
    Then the diff of that file is on screen again

  Scenario: A rendered file that leaves the diff returns to the source
    Given I am reading "README.md" rendered
    When that file is no longer in the diff
    Then the diff is on screen again

  Scenario: About Hunk
    Given hunk is open
    When I choose About from the application menu on macOS, or from Help on Linux
    Then a dialog shows the Hunk icon, the version, and the copyright

  Scenario: Theme follows the system
    Given hunk is open
    Then the colors follow the system theme
    And the View menu can switch to dark or light
    And buttons, fields, and menus have rounded corners
    And the toolbar buttons have a filled background
    And Open, Refresh, and Copy reviews each show a matching icon
    And tooltips use the window background and the normal text color

  Scenario: Text sizes follow the theme
    Given a diff is on screen
    Then the code uses the theme monospace size
    And file names, labels, and the rest of the window use the theme text size

  Scenario: A rename keeps each side's language
    Given "app.rb" is renamed to "app.cpp"
    Then the old side colors Ruby
    And the new side colors C++

  Scenario: A renamed file stays one file
    Given a file was renamed and then changed
    When I view uncommitted changes or this branch's merge request
    Then the file list shows the old name and the new name
    And the diff is that one rename
    When I choose Staged
    Then the file list shows the old name and the new name
    And the diff is the staged copy of that rename
