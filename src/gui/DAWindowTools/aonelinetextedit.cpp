#include "aonelinetextedit.h"

#include <QtGui>

AOneLineTextEdit::AOneLineTextEdit(QWidget * parent) : QPlainTextEdit(parent)
{
    setTabChangesFocus(true);
    setWordWrapMode(QTextOption::NoWrap);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(sizeHint().height());

    //setFrameShape(QFrame::Box);
    //setFrameShadow(QFrame::Raised);
}

void AOneLineTextEdit::setText(const QString & text)
{
    appendPlainText(text);
}

QString AOneLineTextEdit::text() const
{
    return document()->toPlainText();
}

void AOneLineTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        event->accept();
        emit editingFinished();
    }
    else
        QPlainTextEdit::keyPressEvent(event);
}

void AOneLineTextEdit::focusOutEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusOutEvent(event);
    emit editingFinished();
}

#include <QFont>
#include <QFontMetrics>
#include <QStyleOptionFrame>
#include <QStyle>
#include <QApplication>

QSize AOneLineTextEdit::sizeHint() const
{
    /*
    QFontMetrics fm(font());
    int h = qMax(fm.height(), 14) + 4;
    int w = fm.width(QLatin1Char('x')) * 17 + 4;
    QStyleOptionFrameV2 opt;
    opt.initFrom(this);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                      expandedTo(QApplication::globalStrut()), this));
                                      */
/*
    QFontMetrics fm(font());
    QStyleOptionFrame opt;
    QString text = document()->toPlainText();

    int h = qMax(fm.height(), 14) + 4;
    int w = fm.width(text) + 4;

    opt.initFrom(this);

    return style()->sizeFromContents(
                QStyle::CT_LineEdit,
                &opt,
                QSize(w, h).expandedTo(QApplication::globalStrut()),
                this
                );
                */

    ensurePolished();
    QFontMetrics fm(font());
    const int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
    //const QMargins tm = d->effectiveTextMargins();
    int h = qMax(fm.height(), qMax(14, iconSize - 2));// + 2 * QLineEditPrivate::verticalMargin + tm.top() + tm.bottom() + d->topmargin + d->bottommargin;
    int w = fm.width(QLatin1Char('x')) * 17 + 4;//fm.horizontalAdvance(QLatin1Char('x')) * 17 + 2 * QLineEditPrivate::horizontalMargin + tm.left() + tm.right() + d->leftmargin + d->rightmargin; // "some"
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                      expandedTo(QApplication::globalStrut()), this));
}

// ------------------

/*
void AShapeHighlighter::highlightBlock(const QString & text)
{
    for (const HighlightingRule &rule, highlightingRules)
        {
          QRegExp expression(rule.pattern);
          int index = expression.indexIn(text);
          while (index >= 0) {
              int length = expression.matchedLength();
              setFormat(index, length, rule.format);
              index = expression.indexIn(text, index + length);
            }
        }
}
*/
