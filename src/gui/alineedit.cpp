#include "alineedit.h"

#include <QCompleter>
#include <QTextCursor>
#include <QAbstractItemView>
#include <QScrollBar>

void ALineEdit::keyPressEvent(QKeyEvent *e)
{
    QLineEdit::keyPressEvent(e);

    if (!c) return;

    c->setCompletionPrefix(cursorWord(this->text()));
    if (c->completionPrefix().length() < 1)
    {
        c->popup()->hide();
        return;
    }
    QRect cr = cursorRect();
    cr.setWidth(c->popup()->sizeHintForColumn(0) + c->popup()->verticalScrollBar()->sizeHint().width());
    c->complete(cr);
}

QString ALineEdit::cursorWord(const QString &sentence) const
{
    return sentence.mid(sentence.left(cursorPosition()).lastIndexOf(" ") + 1,
                        cursorPosition() -
                        sentence.left(cursorPosition()).lastIndexOf(" ") - 1);
}

void ALineEdit::insertCompletion(QString arg)
{
    setText(text().replace(text().left(cursorPosition()).lastIndexOf(" ") + 1,
                           cursorPosition() -
                           text().left(cursorPosition()).lastIndexOf(" ") - 1,
                           arg));
}

void ALineEdit::setCompleter(QCompleter* completer)
{
    if (c) QObject::disconnect(c, 0, this, 0);
    c = completer;
    if (!c) return;

    c->setWidget(this);
    //c->setCompletionMode(QCompleter::PopupCompletion);
    connect(c, SIGNAL(activated(QString)),
            this, SLOT(insertCompletion(QString)));
}


