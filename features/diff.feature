Feature: Diff view

  Scenario: Run hunk in a repository
    Given I run hunk in a git repository
    Then the window shows that repository's merge request

  Scenario: Opening another repository watches that one
    Given a repository is on screen
    When I open a different repository
    Then hunk watches the repository I opened

  Scenario: Base and branch selectors
    Given I open a repository
    Then the base selector shows the branching point and its parent branch
    And the branch selector shows the current branch
    And the branch list puts the newest change first
    And typing in either box filters that list without leaving the field
    And a branch name stays readable on the toolbar, shortened only when it does not fit there
    And the diff is a merge request between them
    And uncommitted changes on the current branch are included

  Scenario: Switching back leaves an unchanged diff still
    Given the merge request is on screen
    When I switch back to hunk and the files are unchanged
    Then the diff stays as it was
    And the scroll position stays

  Scenario: A reviewed file stays marked after it changes
    Given I have reviewed a file in the list
    When that file changes
    Then that file is highlighted in the list
    When I view that file again
    Then the highlight is gone

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

  Scenario: Unchanged lines collapse between changes
    Given a diff with a long stretch of unchanged lines
    Then those lines are hidden
    And a bar shows how many unchanged lines are hidden
    When I click that bar
    Then the unchanged lines are shown

  Scenario: A rewritten line stays one block
    Given a changed line where only spaces stay the same
    Then that line is shown as removed and added
    And individual words are not highlighted

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

  Scenario: Theme follows the system
    Given hunk is open
    Then the colors follow the system theme
    And the View menu can switch to dark or light
    And buttons, fields, and menus have rounded corners
    And the toolbar buttons have a filled background

  Scenario: Text sizes follow the theme
    Given a diff is on screen
    Then the code uses the theme monospace size
    And file names, labels, and the rest of the window use the theme text size

  Scenario: A rename keeps each side's language
    Given "app.rb" is renamed to "app.cpp"
    Then the old side colors Ruby
    And the new side colors C++
