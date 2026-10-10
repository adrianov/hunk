// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

class QWebEngineView;

// GitHub Flavored Markdown, drawn with GitHub's stylesheet.
void prepareMarkdown(QWebEngineView *view);
void showMarkdown(QWebEngineView *view, const QString &markdown, const QString &dir, bool dark);
