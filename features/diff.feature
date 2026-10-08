# Copyright © 2026 Peter Adrianov
# SPDX-License-Identifier: MIT

Feature: Diff view

  Scenario: Run hunk in a repository
    Given I run hunk in a git repository
    Then the window shows that repository's merge request
    And the window title shows the full path to that repository
    And the toolbar shows the path to that repository
    And toolbar labels use the normal text color
    And the Merge request menu uses the same arrow as Base and Branch
    And the dock or taskbar shows the Hunk icon
    And on macOS the dock icon has the system rounded shape

  Scenario: Opening another repository watches that one
    Given a repository is on screen
    When I open a different repository
    Then hunk watches the repository I opened

  Scenario: Base and branch selectors
    Given I open a repository
    Then the base selector shows the branch this work was cut from, with ^ or ~N when that cut is an older commit
    And the branch selector shows the current branch
    And the branch list puts the newest change first
    And typing in either box filters that list without leaving the field
    And a branch name stays readable on the toolbar, shortened only when it does not fit there
    And the diff is a merge request between them
    And uncommitted changes on the current branch are included

  Scenario: The file list shows added and removed lines
    Given a diff is on screen
    Then each file's added count is green and its removed count is red
    And under the file list the total added lines are green and the total removed lines are red
    And that total stays visible while the file list scrolls

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

  Scenario: A rewritten line stays one block
    Given a changed line where only spaces stay the same
    Then that line is shown as removed and added
    And individual words are not highlighted

  Scenario: Copy text from a diff pane
    Given a diff is on screen
    When I select text in the left or the right pane
    And I open the pane menu
    Then Copy is there, with the shortcut Ctrl+C
    When I copy
    Then the clipboard contains that text

  Scenario: A long line wraps inside the pane
    Given a changed line longer than the pane
    Then the rest of the line continues on the next lines in that pane
    And the other pane stays beside it

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

  Scenario: About Hunk
    Given hunk is open
    When I choose About from the application menu on macOS, or from Help on Linux
    Then a dialog shows the Hunk version and the copyright

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
