#include "aonelinetextedit.h"

#include <QtGui>
#include <QCompleter>
#include <QAbstractItemView>
#include <QScrollBar>

AOneLineTextEdit::AOneLineTextEdit(QWidget * parent) : QPlainTextEdit(parent)
{
    setTabChangesFocus(true);
    setWordWrapMode(QTextOption::NoWrap);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(sizeHint().height());
}

void AOneLineTextEdit::setText(const QString & text)
{
    clear();
    appendPlainText(text);
}

QString AOneLineTextEdit::text() const
{
    return document()->toPlainText();
}

void AOneLineTextEdit::insertCompletion(const QString &completion)
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    tc.removeSelectedText();
    tc.insertText(completion);
}

void AOneLineTextEdit::keyPressEvent(QKeyEvent * e)
{
    //qDebug() << "Key pressed:" << e->text();
    if ( !Completer || (Completer && !Completer->popup()->isVisible()) )
    {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
        {
            e->accept();
            emit editingFinished();
            return;
        }
    }

    if (!Completer)
    {
        QPlainTextEdit::keyPressEvent(e);
        return;
    }

    if (Completer->popup()->isVisible())
    {
        // The following keys are forwarded by the completer to the widget
        switch (e->key())
        {
        case Qt::Key_Right:
        {
            //qDebug() << "Tab pressed when completer is active";  // Tab is not intercepted, using right arrow instead
            //QString startsWith = c->completionPrefix();
            int i = 0;
            QAbstractItemModel * m = Completer->completionModel();
            QStringList sl;
            while (m->hasIndex(i, 0)) sl << m->data(m->index(i++, 0)).toString();
            if (sl.size() < 2)
            {
                e->ignore(); // let the completer do default behavior
                return;
            }
            QString root = sl.first();
            for (int isl=1; isl<sl.size(); isl++)
            {
                const QString & item = sl.at(isl);
                if (root.length() > item.length())
                    root.truncate(item.length());
                for (int i = 0; i < root.length(); ++i)
                {
                    if (root[i] != item[i])
                    {
                        root.truncate(i);
                        break;
                    }
                }
            }
            //qDebug() << root;
            if (root.isEmpty())
            {
                //do nothing
            }
            else
            {
                insertCompletion(root);
                Completer->popup()->setCurrentIndex(Completer->completionModel()->index(0, 0));
            }
            return;
        }
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Backtab:
            e->ignore(); // let the completer do default behavior
            return;
        default:
            break;
        }
    }

    QPlainTextEdit::keyPressEvent(e);

    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);//textUnderCursor();
    QString completionPrefix = tc.selectedText();

    if (/*e->text().isEmpty() || */ completionPrefix.length() < 3)
    {
        //qDebug() << "Hiding!";
        Completer->popup()->hide();
        return;
    }

    if (completionPrefix != Completer->completionPrefix())
    {
        Completer->setCompletionPrefix(completionPrefix);
        Completer->popup()->setCurrentIndex(Completer->completionModel()->index(0, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(Completer->popup()->sizeHintForColumn(0)
                + Completer->popup()->verticalScrollBar()->sizeHint().width());
    Completer->complete(cr); // popup it up!

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

void ABasicHighlighter::highlightBlock(const QString & text)
{
    for (const AHighlightingRule & rule : HighlightingRules)
    {
        const QRegExp & expression = rule.pattern;
        int index = rule.pattern.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}
