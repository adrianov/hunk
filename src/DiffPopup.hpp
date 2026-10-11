// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"

#include <QFrame>

#include <functional>

class QLabel;
class QScrollArea;
class DefCode;

// Floating definition: file and line, then the comment, signature, and body.
// The code uses the diff colors, and long lines wrap the same way.
class DefPopup : public QFrame {
public:
    explicit DefPopup(QWidget *parent);
    std::function<void()> onLeave;
    void present(const LangDef &def, const QFont &font, const QPoint &global);

protected:
    void leaveEvent(QEvent *event) override;

private:
    void fitCode(const QFont &font, const QString &code, const QString &path);
    void place(const QPoint &global);

    QLabel *m_path = nullptr;
    QScrollArea *m_scroll = nullptr;
    DefCode *m_code = nullptr;
};
