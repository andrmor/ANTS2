#include "atextedit.h"

#include <QCompleter>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QDebug>
#include <QApplication>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QScrollBar>
#include <QToolTip>
#include <QWidget>
#include <QTextEdit>
#include <QPainter>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QClipboard>

ATextEdit::ATextEdit(QWidget *parent) : QPlainTextEdit(parent), c(0)
{
    LeftField = new ALeftField(*this);
    connect(this, &ATextEdit::blockCountChanged, this, &ATextEdit::updateLineNumberAreaWidth);
    connect(this, &ATextEdit::updateRequest, this, &ATextEdit::updateLineNumberArea);
    updateLineNumberAreaWidth();

    connect(this, &ATextEdit::cursorPositionChanged, this, &ATextEdit::onCursorPositionChanged);

    setMouseTracking(true);
}

void ATextEdit::setCompleter(QCompleter *completer)
{
    if (c) QObject::disconnect(c, 0, this, 0);
    c = completer;
    if (!c) return;

    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(c, SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
}

void ATextEdit::keyPressEvent(QKeyEvent *e)
{
    QTextCursor tc = this->textCursor();

    //simple cases handle with switch
    switch (e->key())
    {
        case Qt::Key_V :
        {
            if (e->modifiers() & Qt::ControlModifier)
            {
                paste();
                return;
            }
        }
        break;

        case Qt::Key_Tab :
        {
            if (e->modifiers() == 0 && !(c && c->popup()->isVisible()) )
            {
                int posInBlock = tc.positionInBlock();
                int timesInsert = TabInSpaces - posInBlock % TabInSpaces;
                if (timesInsert == 0) timesInsert += TabInSpaces;
                QString s(" ");
                this->insertPlainText(s.repeated(timesInsert));
                return;
            }
        }
        break;

        case Qt::Key_Backspace :
        {
            if (e->modifiers() & Qt::ShiftModifier)
            {
                QTextCursor tc = this->textCursor();
                int posInBlock = tc.positionInBlock();
                int timesDelete = posInBlock % TabInSpaces;
                if (timesDelete == 0) timesDelete = TabInSpaces;
                if (timesDelete <= posInBlock)
                {
                    tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, timesDelete);
                    QString s = tc.selectedText().simplified();
                    if (s.isEmpty()) tc.removeSelectedText();
                }
                return;
            }
        }
        break;

        case Qt::Key_Escape :
            QToolTip::hideText();
            //no return!
        break;

        case Qt::Key_F1 :
        {
            QString text = SelectObjFunctUnderCursor();
            emit requestHelp(text);
            return;
        }
        break;

        case Qt::Key_Delete :
        {
            if ( e->modifiers() & Qt::ShiftModifier )  //Delete line
            {
                tc.select(QTextCursor::LineUnderCursor);
                tc.removeSelectedText();
                tc.deleteChar();
                onCursorPositionChanged();
                return;
            }
            QPlainTextEdit::keyPressEvent(e);
            onCursorPositionChanged();
            return;
        }
        break;

        case Qt::Key_Down :
        {
            if ( (e->modifiers() & Qt::ControlModifier) && (e->modifiers() & Qt::AltModifier) ||
                 (e->modifiers() & Qt::ControlModifier) && (e->modifiers() & Qt::ShiftModifier) )  //Copy line;
            {
                tc.select(QTextCursor::LineUnderCursor);
                QString line = tc.selectedText();
                tc.movePosition(QTextCursor::EndOfLine);
                this->setTextCursor(tc);
                this->insertPlainText("\n");
                this->insertPlainText(line);
                onCursorPositionChanged();
                return;
            }
        }
        break;

        case Qt::Key_Plus :
        {
            if ( e->modifiers() & Qt::ControlModifier ) //font size +
            {
                int size = font().pointSize();
                setFontSizeAndEmitSignal(++size);
                return;
            }
        }
        break;

        case Qt::Key_Minus :
        {
            if ( e->modifiers() & Qt::ControlModifier )  //font size -
            {
                int size = font().pointSize();
                setFontSizeAndEmitSignal(--size); //check is there: cannot go < 1
                return;
            }
        }
        break;

        default:
        break;
    }

    /*  BUGS!
    if (e->key() == Qt::Key_Delete && (e->modifiers()==0))
    {
        QTextCursor tc = this->textCursor();
        int pos = tc.position();
        tc.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        if (tc.selectedText().simplified().isEmpty())
        {
            //tc.insertText("");
            tc.removeSelectedText();
            tc.movePosition(QTextCursor::Down);
            tc.movePosition(QTextCursor::StartOfLine);
            tc.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
            QString s = tc.selectedText().trimmed();
            tc.removeSelectedText();

            tc.setPosition(pos);
            tc.insertText(s);
            tc.setPosition(pos);
            setTextCursor(tc);
            return;
        }
        else
        {
            QPlainTextEdit::keyPressEvent(e);
            return;
        }
    }
    */

    if (e->key() == Qt::Key_Return  && !(c && c->popup()->isVisible()))
      { //enter is pressed but completer popup is not visible
        QTextCursor tc = this->textCursor();
        tc.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        QString onRight = tc.selectedText();

        //leading spaces?
        tc.select(QTextCursor::LineUnderCursor);
        QString line = tc.selectedText();
        int startingSpaces = 0;
        for (int i=0; i<line.size(); i++)
          {
            if (line.at(i) != QChar::Space) break;
            else startingSpaces++;
          }
        QString spacer;
        spacer.fill(QChar::Space, startingSpaces);  //leading spaces


        if (onRight.simplified() == "")
          {
            QString insert = "\n";
            tc = this->textCursor();
            tc.movePosition(QTextCursor::EndOfLine);
            tc.select(QTextCursor::WordUnderCursor);

            bool fUp = false;

            //auto-insert "}"
            if (SelectTextToLeft(this->textCursor(), 1) == "{")
            {
                //do it only if this last "{" is inside closed brackets section (or no brackets)
                if (InsertClosingBracket())
                {
                    insert += spacer + QString(" ").repeated(TabInSpaces) + "\n" + spacer + "}";
                    fUp = true;
                }
            }

            //auto-insert "{" and "}"//
//            QString lastWord = tc.selectedText();
//            if ( line.simplified().endsWith("{") )
//              {
//                 //if the next curly bracket is "{" (or do not found), add "}"
//                 QTextCursor tmpc = this->textCursor();
//                 bool fDoIt = true;
//                 do
//                   {
//                     tmpc.clearSelection();
//                     tmpc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
//                     QString st = tmpc.selectedText();
//                     if (st == "}")
//                       {
//                         fDoIt = false;
//                         break;
//                       }
//                     if (st == "{") break; //still true
//                   }
//                 while (!tmpc.atEnd());
//                 if (fDoIt)
//                   {
//                     insert += spacer+"    \n"+
//                               spacer+"}";
//                     fUp = true;
//                   }
//                 else insert += spacer+"  ";
//              }

//            else if (line.simplified().startsWith("for") || lastWord == "while" || lastWord == "do")
//              {
//                 insert += spacer +"{\n"+
//                           spacer+"   \n"+
//                           spacer+"}";
//                 fUp = true;
//              }
//            else
            insert += spacer;

            tc.movePosition(QTextCursor::EndOfLine);
            tc.insertText(insert);
            if (fUp)
              {
                tc.movePosition(QTextCursor::Up);
                tc.movePosition(QTextCursor::EndOfLine);
              }
            this->setTextCursor(tc);
            return;
          }
        else
        {
            tc = this->textCursor();
            tc.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            tc.insertText("\n" + spacer + onRight);
            tc.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
            tc.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, startingSpaces);
            this->setTextCursor(tc);
            return;
        }

      }

    // ----- completer -----
    if (c && c->popup()->isVisible())
    {
        // The following keys are forwarded by the completer to the widget
        switch (e->key())
        {
        case Qt::Key_Tab:
          {
            //qDebug() << "Tab pressed when completer is active";
            //QString startsWith = c->completionPrefix();
            int i = 0;
            QAbstractItemModel * m = c->completionModel();
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
                c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
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

    bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (!c || !isShortcut) // do not process the shortcut when we have a completer
        QPlainTextEdit::keyPressEvent(e);

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!c || (ctrlOrShift && e->text().isEmpty()))
        return;

    //static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
    static QString eow("~!@#$%^&*()+{}|:\"<>?,/;'[]\\-="); // end of word  //no dot
    bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    //qDebug() <<completionPrefix<< e->text().right(1)<< "hasModifier:"<<hasModifier << (e->modifiers() != Qt::NoModifier) << !ctrlOrShift;
    if (e->text().right(1) == "." || e->text().right(1) == "_") hasModifier = false; //my fix for dot

    if (!isShortcut && (hasModifier || e->text().isEmpty()|| completionPrefix.length() < 3 || eow.contains(e->text().right(1))))
    {
        //qDebug() << "Hiding!";
        c->popup()->hide();
        return;
    }

    if (completionPrefix != c->completionPrefix())
    {
        c->setCompletionPrefix(completionPrefix);
        c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(c->popup()->sizeHintForColumn(0)
                + c->popup()->verticalScrollBar()->sizeHint().width());
    c->complete(cr); // popup it up!
}

void ATextEdit::focusInEvent(QFocusEvent *e)
{
    //if (c) c->setWidget(this);
    QPlainTextEdit::focusInEvent(e);
}

void ATextEdit::wheelEvent(QWheelEvent *e)
{
  if (e->modifiers().testFlag(Qt::ControlModifier))
    {
      int size = font().pointSize();
      if (e->delta() > 0) setFontSizeAndEmitSignal(++size);
      else setFontSizeAndEmitSignal(--size); //check is there: cannot go < 1
    }
  else
    {
      //scroll
      int delta = e->delta();
      if (delta != 0) //paranoic :)
      {
          int was = this->verticalScrollBar()->value();
          delta /= abs(delta);
          this->verticalScrollBar()->setValue(was - 2*delta);
      }
    }
}

void ATextEdit::setFontSizeAndEmitSignal(int size)
{
    QFont f = font();
    if (size<1) size = 1;
    f.setPointSize(size);
    setFont(f);
    emit fontSizeChanged(size);
}

void ATextEdit::paintLeftField(QPaintEvent *event)
{
    QPainter painter(LeftField);
    QColor color = QColor(Qt::gray).lighter(152);
    painter.fillRect(event->rect(), color);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>( blockBoundingGeometry(block).translated(contentOffset()).top() );
    int bottom = top + static_cast<int>( blockBoundingRect(block).height() );

    QFont font = painter.font();
    font.setPointSize(font.pointSize() * 0.8);
    painter.setFont(font);

    int currentLine = textCursor().block().firstLineNumber();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen( currentLine == blockNumber ? Qt::black : Qt::gray);
            painter.drawText(0, top, LeftField->width(), fontMetrics().height(), Qt::AlignCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>( blockBoundingRect(block).height() );
        ++blockNumber;
    }
}

int ATextEdit::getWidthLeftField() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int width = 20 + fontMetrics().width(QLatin1Char('0')) * digits;
    return width;
}

void ATextEdit::mouseDoubleClickEvent(QMouseEvent* /*e*/)
{
  QTextCursor tc = textCursor();
  tc.select(QTextCursor::WordUnderCursor);
  setTextCursor(tc);

  QList<QTextEdit::ExtraSelection> extraSelections;
  setExtraSelections(extraSelections);
}

void ATextEdit::focusOutEvent(QFocusEvent *e)
{
    emit editingFinished();
    QPlainTextEdit::focusOutEvent(e);
}

void ATextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    LeftField->setGeometry( QRect(cr.left(), cr.top(), getWidthLeftField(), cr.height()) );
}

void ATextEdit::insertCompletion(const QString &completion)
{
    if (c->widget() != this) return;

    QTextCursor tc = textCursor();
    //tc.select(QTextCursor::WordUnderCursor);
    while (tc.position() != 0)
      {
       tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
       QString selected = tc.selectedText();
       //qDebug() << "<-" <<selected << selected.left(1).contains(QRegExp("[A-Za-z0-9.]"));
       if ( !selected.left(1).contains(QRegExp("[A-Za-z0-9._]")) )
         {
           tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
           break;
         }
      }    
    //qDebug() << "reached left, selection:"<<tc.selectedText();
    tc.removeSelectedText();


    /*
    QString OnRight;
    while (tc.position() != document()->characterCount()-1)
      {
        tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        QString selected = tc.selectedText();
        qDebug() << "->"<< selected << selected.right(1).contains(QRegExp("[A-Za-z0-9.]"));
        OnRight = selected.right(1);
        if ( !OnRight.contains(QRegExp("[A-Za-z0-9._]")) )
          {
            tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            break;
          }
      }
    if (!tc.selectedText().isEmpty()) tc.removeSelectedText();
    */
    tc.insertText(completion);


    //if inserted empty brackets at the end of the functuion, and the next character is "(", remove "()"
    tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
    tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 3);
    if (tc.selectedText() == "()(")
    {
        tc.removeSelectedText();
        tc.insertText("(");
    }
}

bool ATextEdit::findInList(QString text, QString& tmp) const
{
    if (DeprecatedOrRemovedMethods && DeprecatedOrRemovedMethods->contains(text))
    {
        tmp = DeprecatedOrRemovedMethods->value(text);
        return true;
    }

  for (int i=0; i<functionList.size(); i++)
    {
      tmp = functionList.at(i);
      QString returnArgument = tmp.section(' ', 0, 0) + " ";
      tmp.remove(returnArgument);
      if (tmp.remove(text).startsWith("("))
        {
          tmp = functionList[i];
          return true;
        }
    }
  return false;
}

void ATextEdit::onCursorPositionChanged()
{
  //    qDebug() << "--> On cursor change";

  QList<QTextEdit::ExtraSelection> extraSelections;

  // extraSelections introduced later override previous if both match!

  //lowest priority: highlight line where the cursor is
  QTextEdit::ExtraSelection extra;
  QColor color = QColor(Qt::gray).lighter(150);
  extra.format.setBackground(color);
  extra.format.setProperty(QTextFormat::FullWidthSelection, true);
  extra.cursor = textCursor();
  extra.cursor.clearSelection();
  extraSelections.append(extra);

  //higher priority: show with trailing spaces
  QTextCursor tc = textCursor();
  tc.select(QTextCursor::LineUnderCursor);
  color = QColor(Qt::gray).lighter(140);
  extra.format.setBackground(color);
  extra.cursor = tc;
  extraSelections.append(extra);

  //checking for '}', ')' or ']' on the left
  QColor colorBrackets = QColor(Qt::green).lighter(170);
  checkBracketsOnLeft(extraSelections, colorBrackets);
  //checking for '{', '(' or '[' on the right
  checkBracketsOnRight(extraSelections, colorBrackets);

  if (!FindString.isEmpty())
  {
      QColor col = Qt::yellow;
      QTextCursor cursor = document()->find(FindString, 0, QTextDocument::FindCaseSensitively);
      while(cursor.hasSelection())
        {
          QTextEdit::ExtraSelection extra;
          extra.format.setBackground(col);
          extra.cursor = cursor;
          extraSelections.append(extra);
          cursor = document()->find(FindString, cursor, QTextDocument::FindCaseSensitively);
        }
  }
  else
  {
      // highlight same text if not in Exclude list
      QTextCursor tc = textCursor();
      tc.select(QTextCursor::WordUnderCursor);
      QString selection = tc.selectedText();
      //    qDebug() << "-->"<<selection;
      QColor color = QColor(Qt::green).lighter(170);
      QRegExp exl("[0-9 (){}\\[\\]=+\\-*/\\|~^.,:;\"'<>\\#\\$\\&\\?]");
      QString test = selection.simplified();
      test.remove(exl);
      //    qDebug() << "rem-->"<<test;
      if (test.isEmpty())
      {
          //could be after the var name and followed by bracket
          tc = textCursor();
          tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor);
          tc.select(QTextCursor::WordUnderCursor);
          selection = tc.selectedText();
          test = selection.simplified();
          test.remove(exl);
          //    qDebug() << "New test:>>>"<<test;
      }

      if (!test.isEmpty())
        {
          QRegExp pat("\\b"+selection+"\\b");
          QTextCursor cursor = document()->find(pat, 0, QTextDocument::FindCaseSensitively);
          while(cursor.hasSelection())
            {
              QTextEdit::ExtraSelection extra;
              extra.format.setBackground(color);
              extra.cursor = cursor;
              extraSelections.append(extra);
              cursor = document()->find(pat, cursor, QTextDocument::FindCaseSensitively);
            }

          //variable highlight test
          QRegExp patvar("\\bvar\\s+"+selection+"\\b");
          QTextCursor cursor1 = document()->find(patvar, tc, QTextDocument::FindCaseSensitively | QTextDocument::FindBackward);
          if (cursor1.hasSelection())// && cursor1 != tc)
            {
              QTextEdit::ExtraSelection extra;
              extra.format.setBackground(Qt::black);
              extra.format.setForeground(Qt::white);
                //extra.format.setFontUnderline(true);
              extra.format.setUnderlineColor(Qt::black);
              extra.cursor = cursor1;
              extraSelections.append(extra);

              QTextEdit::ExtraSelection extra1;
              extra1.format.setBackground(Qt::white);
              extra1.format.setFontUnderline(true);
              extra1.format.setUnderlineColor(Qt::blue);
              //extra1.format.setForeground(color);
              extra1.cursor = tc;
              extraSelections.append(extra1);
            }
        }
  }

  //all extra selections defined, applying now
  setExtraSelections(extraSelections);

  // tooltip for known functions
  QTextCursor tcc = textCursor();
  TryShowFunctionTooltip(&tcc);

  if (bMonitorLineChange)
  {
      int currentLine = textCursor().blockNumber();
      if ( currentLine != previousLineNumber) emit lineNumberChanged(currentLine);
  }
}

bool ATextEdit::TryShowFunctionTooltip(QTextCursor* cursor)
{
    QTextCursor tc = *cursor;
    QString functionCandidateText = SelectObjFunctUnderCursor(cursor);
    QString tmp;
    bool fFound = findInList(functionCandidateText, tmp); // cursor is on one of defined functions
    if (!fFound)
      {
        while (tc.position() != 0)
          {
            tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            QString selected = tc.selectedText();
            //qDebug() << selected << selected.left(1).contains(QRegExp("[A-Za-z0-9.]"));
            if ( selected.left(1) == ")" ) break;
            if ( selected.left(1) == "\n" ) break;
            if ( selected.left(1) == "(")
              {
                tc.setPosition(tc.position(), QTextCursor::MoveAnchor);
                //qDebug() << SelectObjFunctUnderCursor(&tc);
                fFound = findInList(SelectObjFunctUnderCursor(&tc), tmp); //is cursor is in the arguments of a defined function
                break;
              }
          }
      }
    int fh = fontMetrics().height();

    if (fFound)
    {
        QToolTip::showText( mapToGlobal( QPoint(cursorRect(*cursor).topRight().x(), cursorRect(*cursor).topRight().y() -3.0*fh )),
                                    tmp,
                                    this,
                                    QRect(),
                                    100000);
        return true;
    }
    else
    {
        QToolTip::hideText();
        return false;
    }
}

bool ATextEdit::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        QTextCursor cursor = cursorForPosition(helpEvent->pos());

        return TryShowFunctionTooltip(&cursor);
    }
    return QPlainTextEdit::event(event);
}

void ATextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    QTextCursor cursor = cursorForPosition(e->pos());
    TryShowFunctionTooltip(&cursor);
    QPlainTextEdit::mouseReleaseEvent(e);
}

void ATextEdit::checkBracketsOnLeft(QList<QTextEdit::ExtraSelection>& extraSelections, const QColor& color)
{
    //checking for '}', ')' or ']' on the left
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    const QString selText = tc.selectedText();
    if ( selText == "}" || selText == ")" || selText == "]")
    {
        QString same, inverse;
        if (selText == "}")
        {
            same = "}"; inverse = "{";
        }
        else if (selText == ")")
        {
            same = ")"; inverse = "(";
        }
        else if (selText == "]")
        {
            same = "]"; inverse = "[";
        }

        int depth = 0;
        QString selected;
        do
        {
            tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            selected = tc.selectedText();
            //qDebug() << selected << selected.left(1).contains(QRegExp("[A-Za-z0-9.]"));
            QString s = selected.left(1);
            if ( s == same )
            {
                depth++;
                continue;
            }
            if (s == inverse)
            {
                if (depth>0)
                {
                    depth--;
                    continue;
                }
                else break;
            }
        }
        while (tc.position() != 0);

        QTextEdit::ExtraSelection extra;
        extra.format.setBackground(color);
        extra.cursor = tc;
        extraSelections.append(extra);
    }
}

void ATextEdit::checkBracketsOnRight(QList<QTextEdit::ExtraSelection> &extraSelections, const QColor &color)
{
    //checking for '{', '(' or '[' on the right
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    const QString selText = tc.selectedText();
    if ( selText == "{" || selText == "(" || selText == "[" )
    {
        QString same, inverse;
        if (selText == "{")
        {
            same = "{"; inverse = "}";
        }
        else if (selText == "(")
        {
            same = "("; inverse = ")";
        }
        else if (selText == "[")
        {
            same = "["; inverse = "]";
        }
        int depth = 0;
        QString selected;
        do
        {
            tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
            selected = tc.selectedText();
            //qDebug() << selected << selected.left(1).contains(QRegExp("[A-Za-z0-9.]"));
            QString s = selected.right(1);
            if ( s == same )
            {
                depth++;
                continue;
            }
            if (s == inverse)
            {
                if (depth>0)
                {
                    depth--;
                    continue;
                }
                else break;
            }
        }
        while (tc.position() < document()->characterCount()-1);

        QTextEdit::ExtraSelection extra;
        extra.format.setBackground(color);
        extra.cursor = tc;
        extraSelections.append(extra);
    }
}

void ATextEdit::updateLineNumberAreaWidth()
{
    setViewportMargins(getWidthLeftField(), 0, 0, 0);
}

void ATextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) LeftField->scroll(0, dy);
    else LeftField->update(0, rect.y(), LeftField->width(), rect.height());

    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth();
}

void ATextEdit::SetFontSize(int size)
{
    if (size<1) size = 1;
    QFont f = font();
    f.setPointSize(size);
    setFont(f);
}

void ATextEdit::RefreshExtraHighlight()
{
    onCursorPositionChanged();
}

void ATextEdit::setTextCursorSilently(const QTextCursor &tc)
{
    bMonitorLineChange = false;
    setTextCursor(tc);
    bMonitorLineChange = true;
}

int ATextEdit::getIndent(const QString& line) const
{
    int indent = 0;
    if (!line.isEmpty())
    {
        for (indent = 0; indent<line.size(); indent++)
            if (line.at(indent) != ' ') break;

        if (indent == line.size()) indent = -1;
    }
    return indent;
}

void ATextEdit::setIndent(QString &line, int indent)
{
    line = line.trimmed();

    const QString spaces = QString(indent, ' ');
    line.insert(0, spaces);
}

void ATextEdit::convertTabToSpaces(QString& line)
{
    for (int i=line.size()-1; i>-1; i--)
        if (line.at(i) == '\t')
        {
            line.remove(i, 1);
            const QString spaces = QString(ATextEdit::TabInSpaces, ' ');
            line.insert(i, spaces);
        }
}

int ATextEdit::getSectionCounterChange(const QString& line) const
{
    int counter = 0;
    int bComment = false;
    for (int i=0; i<line.size(); i++)
    {
        const QChar & ch = line.at(i);
        // ***!!! add ignore commented inside /* */
        if      (ch == '{' ) counter++;
        else if (ch == '}' ) counter--;
        else if (ch == "/")
        {
            if (bComment) break;
            else bComment = true;
        }
        else bComment = false;
    }
    return counter;
}

void ATextEdit::paste()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString text = clipboard->text();
    QStringList lines = text.split('\n');
    if (text.isEmpty() || lines.isEmpty())
    {
        QPlainTextEdit::paste(); //just in case, but most likely it is empty
        return;
    }

    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    QString s = tc.selectedText();
    s.remove(' ');
    if (s.isEmpty())
    {
        //  qDebug() << "Adjusting ident";
        tc = textCursor();
        int newIdent = textCursor().positionInBlock();

        QString toInsert;
        for (int i=0; i<lines.size(); i++)
        {
            QString modLine = lines.at(i);
            if (i != lines.size()-1) modLine += "\n";
            if (i != 0) modLine = QString(newIdent, ' ') + modLine;
            toInsert += modLine;
        }
        tc.insertText( toInsert );
    }
    else
    {
        //  qDebug() << "Not empty, using regular paste";
        QPlainTextEdit::paste(); //just in case, but most likely it is empty
    }
}

void ATextEdit::align()
{
    QTextCursor tc = textCursor();
    int start = tc.anchor();
    int stop = tc.position();
    if (start > stop) std::swap(start, stop);

    tc.setPosition(stop, QTextCursor::MoveAnchor);
    tc.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
    tc.setPosition(start, QTextCursor::KeepAnchor);
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);

    QString text = tc.selection().toPlainText();
    if (text.isEmpty()) return;

    QStringList list = text.split('\n');
    if (list.size() <= 1) return;

    for (QString& s : list) convertTabToSpaces(s);

    int currentIndent = getIndent(list.first());
    const int size = list.size();
    for (int i = 1; i <= size; i++)
    {
        int deltaSections = getSectionCounterChange(list.at(i-1));

        if (deltaSections >= 0)
        {
            currentIndent += deltaSections * TabInSpaces;
            //no need to adjust the previous line
        }
        else
        {
            currentIndent += deltaSections * TabInSpaces;
            if (currentIndent < 0) currentIndent = 0;
            setIndent(list[i-1], currentIndent);
        }
        if (i != size)
            setIndent(list[i], currentIndent);
    }

    QString res = list.join('\n');
    tc.insertText(res);
}

QString ATextEdit::textUnderCursor() const
{    
    QTextCursor tc = textCursor();
/*
    tc.select(QTextCursor::WordUnderCursor);
    //tc.select(QTextCursor::LineUnderCursor);
    return tc.selectedText();
*/
    QString selected;
    do
      {
       tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
       selected = tc.selectedText();
       //qDebug() << selected << selected.left(1).contains(QRegExp("[A-Za-z0-9.]"));
       if ( !selected.left(1).contains(QRegExp("[A-Za-z0-9._]")) ) return selected.remove(0,1);
      }
    while (tc.position() != 0);

    return selected;
}

QString ATextEdit::SelectObjFunctUnderCursor(QTextCursor *cursor) const
{
  QString sel;
  QTextCursor tc = (cursor == 0) ? textCursor() : *cursor;
  while (tc.position() != 0)
    {
     tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
     QString selected = tc.selectedText();
     //qDebug() << "<-" <<selected << selected.left(1).contains(QRegExp("[A-Za-z0-9.]"));
     if ( !selected.left(1).contains(QRegExp("[A-Za-z0-9._]")) )
       {
         tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
         break;
       }
    }
  sel = tc.selectedText();

  while (tc.position() != document()->characterCount()-1)
    {
      tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
      QString selected = tc.selectedText();
      if (selected.isEmpty()) continue;
      //qDebug() << "->"<< selected << selected.right(1).contains(QRegExp("[A-Za-z0-9.]"));
      if ( !selected.right(1).contains(QRegExp("[A-Za-z0-9._]")) )
        {
          tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
          break;
        }
    }
  sel += tc.selectedText();
  return sel;
}

QString ATextEdit::SelectTextToLeft(QTextCursor cursor, int num) const
{
    while ( num>0 && cursor.position() != 0)
    {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
        num--;
    }
    return cursor.selectedText();
}

bool ATextEdit::InsertClosingBracket() const
{
    //to the left
    QTextCursor tc = this->textCursor();
    int depthL = 0;
    do
    {
        QString s = SelectTextToLeft(tc, 1);
        tc.movePosition(QTextCursor::Left);
        if (s == "{") depthL++;
        else if (s == "}") depthL--;
    }
    while (tc.position() != 0 );

    //to the right
    tc = this->textCursor();
    int depthR = 0;
    while (tc.position() != document()->characterCount()-1)
      {
        tc.movePosition(QTextCursor::Right);
        QString s = SelectTextToLeft(tc, 1);
        if (s == "{") depthR++;
        if (s == "}") depthR--;
      }

    if ( (depthL + depthR) == 0) return false;
    if ( (depthL + depthR) < 0 ) return false;
    return true;
}
