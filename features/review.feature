# Copyright © 2026 Peter Adrianov
# SPDX-License-Identifier: MIT

Feature: Review comments

  Scenario: Comment on a line number
    Given a diff of "src/app.rb"
    When I click the line number on line 1
    Then I can write a review comment for that line

  Scenario: One comment per line
    Given a review comments on a diff line
    When I comment on that line again
    Then the review list still has one comment for it

  Scenario: Copy a line comment for an agent
    Given a diff of "src/app.rb"
    When I comment on line 1
    And I copy reviews
    Then the clipboard contains "`src/app.rb:1`"
    And the clipboard contains that line's text

  Scenario: A selected span comments the line interval
    Given a diff of "src/app.rb"
    When I select the text of lines 2 through 4
    And I click a line number beside that selection
    Then the review comment refers to lines 2 through 4
    When I copy reviews
    Then the clipboard contains "`src/app.rb:2-4`"

  Scenario: Comments stay with the app
    Given I have a review comment
    When I reopen the repository
    Then that comment is still there
    And it was not written into the repository

  Scenario: Refresh keeps the comment being edited
    Given I am typing in a review comment
    When the diff refreshes
    Then the comment box still has focus
    And the text I typed is still there
    And that stays true when the comment's line moves
    And a different comment with the same text is left alone

  Scenario: The main toolbar has no Comment button
    Given hunk is open
    Then the main toolbar does not show Comment
    And the Review menu still has Comment

  Scenario: Copy reviews explains old lines only when one is commented
    Given every review comment is on the new side
    When I copy reviews
    Then the clipboard does not explain what `(old)` means

  Scenario: Double-click a comment shows that line
    Given a review comments on a diff line
    When I double-click that comment
    Then the diff shows that line

  Scenario: Each review line can be deleted
    Given the review list has a comment
    Then that line has its own delete control on the left, inset from the row edge
    And hovering that control highlights it
    And the line text is drawn once in the list color
    And that text stays readable when the line is selected
    And the line shows as much of the comment as fits

  Scenario: Auto cleanup starts on
    Given the review list is open
    Then auto cleanup of changed lines is on
    And it sits near Copy reviews

  Scenario: A changed line drops its review
    Given auto cleanup is on
    And a review comments on a diff line
    When that line's text changes
    Then the review is removed

  Scenario: A line that leaves the file drops its review
    Given auto cleanup is on
    And a review comments on a diff line
    When that line leaves the file
    Then the review is removed

  Scenario: A shifted line keeps its review
    Given auto cleanup is on
    And a review comments on a diff line
    When that line stays in the file at a new line
    Then the review stays on the new line
    And a comment on a span of lines does the same

  Scenario: The same text on another line does not take a review
    Given auto cleanup is on
    And a review comments on a diff line
    When that line's text changes
    And the same text remains on another line
    Then the review is removed

  Scenario: Auto cleanup can stay off
    Given auto cleanup is off
    And a review comments on a diff line
    When that line's text changes
    Then the review stays

  Scenario: A line that leaves the diff can stay
    Given auto cleanup is off
    And a review comments on a diff line
    When that line leaves the diff
    Then the review stays
