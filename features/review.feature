# Copyright © 2026 Peter Adrianov
# SPDX-License-Identifier: MIT

Feature: Review comments

  Scenario: Refresh keeps the comment being edited
    Given I am typing in a review comment
    When the diff refreshes
    Then the comment box still has focus
    And the text I typed is still there
    And that stays true when the comment's line moves
    And a different comment with the same text is left alone

  Scenario: Copy reviews explains old lines only when one is commented
    Given every review comment is on the new side
    When I copy reviews
    Then the clipboard does not explain what `(old)` means

  Scenario: Each review line can be deleted
    Given the review list has a comment
    Then that line has its own delete control
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
