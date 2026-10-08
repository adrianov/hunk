// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"
#include "ReviewNote.hpp"

#include <QObject>
#include <QTimer>

// Review comments for one repository. Saved in app settings, not in the repo.
class ReviewStore : public QObject {
    Q_OBJECT
public:
    explicit ReviewStore(QObject *parent = nullptr);

    void setRepo(const QString &root);
    const QList<ReviewNote> &notes() const { return m_notes; }
    int ensure(const QString &path, bool oldSide, int line, const QString &snippet);
    void setBody(int index, const QString &body);
    void removeAt(int index);
    void sync(const DiffDoc &doc, bool dropChanged);

signals:
    void structureChanged();
    void bodyEdited(int index);

private:
    void read();
    void write();
    QString storageKey() const;

    QString m_root;
    QList<ReviewNote> m_notes;
    QTimer m_timer;
};
