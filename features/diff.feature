Feature: Diff view

  Scenario: Run hunk in a repository
    Given I run hunk in a git repository
    Then the window shows that repository's merge request

  Scenario: Base and branch selectors
    Given I open a repository
    Then the base selector shows the branching point
    And the branch selector shows the current branch
    And the diff is a merge request between them
    And uncommitted changes on the current branch are included

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

  Scenario: A rename keeps each side's language
    Given "app.rb" is renamed to "app.cpp"
    Then the old side colors Ruby
    And the new side colors C++
