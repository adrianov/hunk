#include "DiffCanvas.hpp"

#include <QFontDatabase>
#include <QFontMetrics>
#include <QScrollBar>

DiffCanvas::DiffCanvas(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    ensureFont();
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        viewport()->update();
        emitVisibleFile();
    });
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() { viewport()->update(); });
}

void DiffCanvas::ensureFont()
{
    m_mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_mono.setPointSize(12);
    const QFontMetrics metrics(m_mono);
    m_rowH = metrics.height() + 6;
    m_gutterW = qMax(56, metrics.horizontalAdvance(QStringLiteral("00000")) + 18);
}

void DiffCanvas::setMessage(const QString &text)
{
    m_message = text;
    m_doc = {};
    m_bands.clear();
    m_docH = 0;
    m_selFile = m_selRow = m_hoverFile = m_hoverRow = m_lastFile = -1;
    updateScroll();
    viewport()->update();
}

void DiffCanvas::setDoc(const DiffDoc &doc, const QString &leftLabel, const QString &rightLabel)
{
    m_message.clear();
    m_doc = doc;
    m_leftLabel = leftLabel;
    m_rightLabel = rightLabel;
    m_selFile = m_selRow = m_hoverFile = m_hoverRow = m_lastFile = -1;
    rebuild();
}

void DiffCanvas::setNoteKeys(const QSet<QString> &keys)
{
    m_notes = keys;
    viewport()->update();
}

void DiffCanvas::trackLine(const QString &text)
{
    const QFontMetrics metrics(m_mono);
    m_maxAdvance = qMax(m_maxAdvance, metrics.horizontalAdvance(text));
}

void DiffCanvas::addFileBands(int fileIndex, int *y)
{
    const FileDiff &file = m_doc.files.at(fileIndex);
    m_bands.push_back(Band{Band::Header, fileIndex, -1, *y, m_headerH});
    *y += m_headerH;
    m_bands.push_back(Band{Band::Labels, fileIndex, -1, *y, m_labelH});
    *y += m_labelH;
    if (file.binary || file.rows.isEmpty()) {
        m_bands.push_back(Band{Band::Note, fileIndex, -1, *y, m_rowH});
        *y += m_rowH;
    } else {
        for (int rowIndex = 0; rowIndex < file.rows.size(); ++rowIndex) {
            trackLine(file.rows.at(rowIndex).leftText);
            trackLine(file.rows.at(rowIndex).rightText);
            m_bands.push_back(Band{Band::Row, fileIndex, rowIndex, *y, m_rowH});
            *y += m_rowH;
        }
    }
    *y += 16;
}

void DiffCanvas::rebuild()
{
    ensureFont();
    m_bands.clear();
    m_maxAdvance = 0;
    int y = 8;
    for (int fileIndex = 0; fileIndex < m_doc.files.size(); ++fileIndex)
        addFileBands(fileIndex, &y);
    m_docH = y + 12;
    updateScroll();
    viewport()->update();
    emitVisibleFile();
}

int DiffCanvas::rowBaseline(const Band &band) const
{
    const QFontMetrics metrics(m_mono);
    return band.y + (band.h - metrics.height()) / 2 + metrics.ascent();
}
