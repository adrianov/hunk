Feature: Review comments

  Scenario: Each review line can be deleted
    Given the review list has a comment
    Then that line has its own delete control
    And hovering that control highlights it

  Scenario: Auto cleanup starts on
    Given the review list is open
    Then auto cleanup of changed lines is on
    And it sits near Copy reviews

  Scenario: A changed line drops its review
    Given auto cleanup is on
    And a review comments on a diff line
    When that line's text changes
    Then the review is removed

  Scenario: Auto cleanup can stay off
    Given auto cleanup is off
    And a review comments on a diff line
    When that line's text changes
    Then the review stays
