// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "ReviewStore.hpp"

#include "ReviewSync.hpp"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

namespace {

bool sameAnchor(const ReviewNote &note, const QString &path, bool oldSide, int line, int span)
{
    const int held = note.end > note.line ? note.end : 0;
    return note.path == path && note.oldSide == oldSide && note.line == line && held == span;
}

} // namespace

ReviewStore::ReviewStore(QObject *parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(300);
    connect(&m_timer, &QTimer::timeout, this, [this]() { write(); });
}

void ReviewStore::setRepo(const QString &root)
{
    if (root == m_root)
        return;
    m_timer.stop();
    write();
    m_root = root;
    read();
    emit structureChanged();
}

int ReviewStore::ensure(const QString &path, bool oldSide, int line, const QString &snippet, int end)
{
    const int span = end > line ? end : 0;
    for (int index = 0; index < m_notes.size(); ++index) {
        ReviewNote &note = m_notes[index];
        if (sameAnchor(note, path, oldSide, line, span)) {
            if (note.snippet.isEmpty())
                note.snippet = snippet;
            note.inDiff = true;
            return index;
        }
    }
    ReviewNote note;
    note.path = path;
    note.oldSide = oldSide;
    note.line = line;
    note.end = span;
    note.snippet = snippet;
    note.inDiff = true;
    m_notes.push_back(note);
    write();
    emit structureChanged();
    return m_notes.size() - 1;
}

void ReviewStore::setBody(int index, const QString &body)
{
    if (index < 0 || index >= m_notes.size() || m_notes.at(index).body == body)
        return;
    m_notes[index].body = body;
    emit bodyEdited(index);
    m_timer.start();
}

void ReviewStore::removeAt(int index)
{
    if (index < 0 || index >= m_notes.size())
        return;
    m_notes.removeAt(index);
    write();
    emit structureChanged();
}

void ReviewStore::sync(const DiffDoc &doc, bool dropChanged)
{
    if (!applyNotes(&m_notes, doc, m_root, dropChanged))
        return;
    write();
    emit structureChanged();
}

QString ReviewStore::storageKey() const
{
    return QString::fromLatin1(QCryptographicHash::hash(m_root.toUtf8(), QCryptographicHash::Sha1).toHex());
}

namespace {

void readPlace(ReviewNote *note, const QJsonObject &object)
{
    note->path = object.value(QStringLiteral("path")).toString();
    note->oldSide = object.value(QStringLiteral("old")).toBool();
    note->line = object.value(QStringLiteral("line")).toInt();
    note->end = object.value(QStringLiteral("end")).toInt();
}

ReviewNote noteFromJson(const QJsonObject &object)
{
    ReviewNote note;
    readPlace(&note, object);
    note.snippet = object.value(QStringLiteral("snippet")).toString();
    note.body = object.value(QStringLiteral("body")).toString();
    return note;
}

QJsonObject noteToJson(const ReviewNote &note)
{
    QJsonObject object;
    object.insert(QStringLiteral("path"), note.path);
    object.insert(QStringLiteral("old"), note.oldSide);
    object.insert(QStringLiteral("line"), note.line);
    if (note.end > note.line)
        object.insert(QStringLiteral("end"), note.end);
    object.insert(QStringLiteral("snippet"), note.snippet);
    object.insert(QStringLiteral("body"), note.body);
    return object;
}

bool keepNote(const ReviewNote &note)
{
    return !note.path.isEmpty() && note.line > 0;
}

} // namespace

void ReviewStore::read()
{
    m_notes.clear();
    if (m_root.isEmpty())
        return;
    QSettings settings;
    settings.beginGroup(QStringLiteral("reviews"));
    const QByteArray raw = settings.value(storageKey()).toByteArray();
    settings.endGroup();
    for (const QJsonValue &value : QJsonDocument::fromJson(raw).array()) {
        const ReviewNote note = noteFromJson(value.toObject());
        if (keepNote(note))
            m_notes.push_back(note);
    }
}

void ReviewStore::write()
{
    if (m_root.isEmpty())
        return;
    QJsonArray array;
    for (const ReviewNote &note : m_notes)
        array.append(noteToJson(note));
    QSettings settings;
    settings.beginGroup(QStringLiteral("reviews"));
    settings.setValue(storageKey(), QJsonDocument(array).toJson(QJsonDocument::Compact));
    settings.endGroup();
}
