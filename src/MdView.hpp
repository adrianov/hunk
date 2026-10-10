// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

#include <QWidget>

class DiffCanvas;
class QButtonGroup;
class QLabel;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QTextBrowser;

// Source or rendered view for the markdown file currently in the diff.
class MdView : public QWidget {
public:
    explicit MdView(DiffCanvas *canvas, QWidget *parent = nullptr);

    void setLabels(const QString &left, const QString &right);
    int shownFile() const { return m_file; }
    void leaveFile(int file);
    void track(int file, const DiffDoc &doc, const QString &root);
    void reload(const DiffDoc &doc, const QString &root);
    void showSource();
    void applyTheme();

private:
    void buildBar();
    void wireSwitch();
    void buildPage();
    QWidget *buildPane(QTextBrowser **browser, QLabel **caption);
    void choose(int id);
    void apply();
    void showDocs(const FileDiff &file);
    const FileDiff *currentFile() const;

    DiffCanvas *m_canvas = nullptr;
    const DiffDoc *m_doc = nullptr;
    QWidget *m_bar = nullptr;
    QWidget *m_page = nullptr;
    QStackedWidget *m_stack = nullptr;
    QSplitter *m_split = nullptr;
    QWidget *m_leftPane = nullptr;
    QWidget *m_rightPane = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_leftCaption = nullptr;
    QLabel *m_rightCaption = nullptr;
    QPushButton *m_source = nullptr;
    QPushButton *m_rendered = nullptr;
    QButtonGroup *m_modes = nullptr;
    QTextBrowser *m_left = nullptr;
    QTextBrowser *m_right = nullptr;
    QString m_root;
    QString m_dir;
    QString m_leftName;
    QString m_rightName;
    QString m_leftText;
    QString m_rightText;
    int m_file = -1;
    bool m_on = false;
};
