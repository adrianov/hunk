Feature: Diff view

  Scenario: Run hunk in a repository
    Given I run hunk in a git repository
    Then the window shows that repository's merge request

  Scenario: A long line wraps inside the pane
    Given a changed line longer than the pane
    Then the rest of the line continues on the next lines in that pane
    And the other pane stays beside it

  Scenario: Ruby keyword on an added line
    Given a diff of "lib/app.rb"
    When a line adds "def greet"
    Then "def" is colored as a keyword
    And "greet" keeps the added-line color

  Scenario: A rename keeps each side's language
    Given "app.rb" is renamed to "app.cpp"
    Then the old side colors Ruby
    And the new side colors C++
