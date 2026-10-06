#include "ReviewStore.hpp"

#include "DiffParse.hpp"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

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

int ReviewStore::ensure(const QString &path, bool oldSide, int line, const QString &snippet)
{
    for (int index = 0; index < m_notes.size(); ++index) {
        ReviewNote &note = m_notes[index];
        if (note.path == path && note.oldSide == oldSide && note.line == line) {
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

void ReviewStore::sync(const DiffDoc &doc)
{
    bool changed = false;
    for (ReviewNote &note : m_notes) {
        const LineHit hit = findLine(doc, note.path, note.oldSide, note.line);
        const bool inDiff = hit.file >= 0;
        QString snippet = note.snippet;
        if (inDiff) {
            const DiffRow &row = doc.files.at(hit.file).rows.at(hit.row);
            snippet = note.oldSide ? row.leftText : row.rightText;
        }
        if (inDiff != note.inDiff || snippet != note.snippet) {
            note.inDiff = inDiff;
            note.snippet = snippet;
            changed = true;
        }
    }
    if (!changed)
        return;
    write();
    emit structureChanged();
}

QString ReviewStore::storageKey() const
{
    return QString::fromLatin1(QCryptographicHash::hash(m_root.toUtf8(), QCryptographicHash::Sha1).toHex());
}

namespace {

ReviewNote noteFromJson(const QJsonObject &object)
{
    ReviewNote note;
    note.path = object.value(QStringLiteral("path")).toString();
    note.oldSide = object.value(QStringLiteral("old")).toBool();
    note.line = object.value(QStringLiteral("line")).toInt();
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
