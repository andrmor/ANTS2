#include "completingtexteditclass.h"

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

CompletingTextEditClass::CompletingTextEditClass(QWidget *parent) : QTextEdit(parent), c(0)
{
  this->setToolTipDuration(100000);
  QObject::connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));
  //QObject::connect(this, SIGNAL(textChanged()), this, SLOT(onCursorPositionChanged()));
}

void CompletingTextEditClass::setCompleter(QCompleter *completer)
{
    if (c) QObject::disconnect(c, 0, this, 0);
    c = completer;
    if (!c) return;

    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(c, SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
}

void CompletingTextEditClass::keyPressEvent(QKeyEvent *e)
{
    QTextCursor tc = this->textCursor();
    if (e->key() == Qt::Key_F1)
      {
        QString text = SelectObjFunctUnderCursor();
        emit requestHelp(text);
        e->ignore();
        return;
      }

    if (e->key() == Qt::Key_Return  && !(c && c->popup()->isVisible()))
      { //enter is pressed but completer popup is not visible        
        tc.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        //qDebug() << "---"<<tc.selectedText()<<"---";
        if (tc.selectedText().simplified() == "")
          {
            //no text is to the right
            tc = this->textCursor();
            tc.select(QTextCursor::LineUnderCursor);
            QString line = tc.selectedText();
            int startingSpaces = 0;
            for (int i=0; line.size(); i++)
              {
                if (line.at(i) != QChar::Space) break;
                else startingSpaces++;
              }
            QString spacer;
            spacer.fill(QChar::Space, startingSpaces);
            QString insert = "\n";
            //qDebug() <<line << startingSpaces << "starter>"+spacer+"<";
            tc.movePosition(QTextCursor::EndOfLine);
            tc.select(QTextCursor::WordUnderCursor);
            //qDebug() << tc.selectedText();
            QString lastWord = tc.selectedText();
            bool fUp = false;
            if ( line.simplified().endsWith("{") )
              {
                 //if the next curly bracket is "{" (or do not found), add "}"
                 QTextCursor tmpc = this->textCursor();
                 bool fDoIt = true;
                 do
                   {
                     tmpc.clearSelection();
                     tmpc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                     QString st = tmpc.selectedText();
                     if (st == "}")
                       {
                         fDoIt = false;
                         break;
                       }
                     if (st == "{") break; //still true
                   }
                 while (!tmpc.atEnd());
                 if (fDoIt)
                   {
                     insert += spacer+"    \n"+
                               spacer+"}";
                     fUp = true;
                   }
                 else insert += spacer+"  ";
              }
            else if (line.simplified().startsWith("for") || lastWord == "while" || lastWord == "do")
              {
                 insert += spacer +"{\n"+
                           spacer+"   \n"+
                           spacer+"}";
                 fUp = true;
              }
            else insert += spacer;

            tc.movePosition(QTextCursor::EndOfLine);           
            tc.insertText(insert);
            if (fUp)
              {
                tc.movePosition(QTextCursor::Up);
                tc.movePosition(QTextCursor::EndOfLine);
              }
            this->setTextCursor(tc);
            e->ignore();
            return;
          }
      }

    //font size
    if ( (e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Plus )
    {
        QFont f = font();
        f.setPointSize(f.pointSize()+1);
        setFont(f);
        e->ignore();
        return;
    }
    if ( (e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Minus )
    {
        QFont f = font();
        int size = f.pointSize()-1;
        if (size<1) size = 1;
        f.setPointSize(size);
        setFont(f);
        e->ignore();
        return;
    }

    //Copy line;
    if ( (e->modifiers() & Qt::ControlModifier) && (e->modifiers() & Qt::AltModifier) )
      {
        if (e->key() == Qt::Key_Down)
          {
            tc.select(QTextCursor::LineUnderCursor);
            QString line = tc.selectedText();
            tc.movePosition(QTextCursor::EndOfLine);
            this->setTextCursor(tc);
            this->insertPlainText("\n");
            this->insertPlainText(line);
            e->ignore();
            return;
          }
      }

    //Delete line
    if ( e->modifiers() & Qt::ShiftModifier )
        if (e->key() == Qt::Key_Delete)
        {
            tc.select(QTextCursor::LineUnderCursor);
            tc.removeSelectedText();
            tc.deleteChar();
            e->ignore();
            return;
        }

    if (c && c->popup()->isVisible())
    {        
        // The following keys are forwarded by the completer to the widget
        switch (e->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:        
            e->ignore();            
            return; // let the completer do default behavior
        default:
            break;
        }
    }

    bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (!c || !isShortcut) // do not process the shortcut when we have a completer
        QTextEdit::keyPressEvent(e);

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

void CompletingTextEditClass::focusInEvent(QFocusEvent *e)
{
    if (c) c->setWidget(this);
    QTextEdit::focusInEvent(e);
}

void CompletingTextEditClass::wheelEvent(QWheelEvent *e)
{
  if (e->modifiers().testFlag(Qt::ControlModifier))
    {
      QFont f = font();
      if (e->delta()>0)
      {
        f.setPointSize(f.pointSize()+1);
        setFont(f);
      }
      else
      {
        int size = f.pointSize()-1;
        if (size<1) size = 1;
        f.setPointSize(size);
        setFont(f);
      }
    }
  else
    {
      //scroll
      int vas = this->verticalScrollBar()->value();
      this->verticalScrollBar()->setValue(vas - e->delta()*0.5);
    }
}

void CompletingTextEditClass::mouseDoubleClickEvent(QMouseEvent* /*e*/)
{
  QTextCursor tc = textCursor();
  tc.select(QTextCursor::WordUnderCursor);
  setTextCursor(tc);

  QList<QTextEdit::ExtraSelection> extraSelections;
  setExtraSelections(extraSelections);

//  QTextCursor tc = textCursor();
//  tc.select(QTextCursor::WordUnderCursor);
//      //tc.select(QTextCursor::LineUnderCursor);
//  QString selection = tc.selectedText();

//  QList<QTextEdit::ExtraSelection> extraSelections;

//  QColor color = QColor(Qt::green).lighter(160);

//  QTextCursor cursor = document()->find(selection, 0, QTextDocument::FindCaseSensitively);

//  while(cursor.hasSelection())
//    {
//      QTextEdit::ExtraSelection extra;
//      extra.format.setBackground(color);
//      extra.cursor = cursor;
//      extraSelections.append(extra);
//      cursor = document()->find(selection, cursor, QTextDocument::FindCaseSensitively);
//    }
  //  setExtraSelections(extraSelections);
}

void CompletingTextEditClass::focusOutEvent(QFocusEvent *e)
{
  emit editingFinished();
  QTextEdit::focusOutEvent(e);
}

void CompletingTextEditClass::insertCompletion(const QString &completion)
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

    QString OnRight;
    while (tc.position() != document()->characterCount()-1)
      {
        tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
        QString selected = tc.selectedText();
        //qDebug() << "->"<< selected << selected.right(1).contains(QRegExp("[A-Za-z0-9.]"));
        OnRight = selected.right(1);
        if ( !OnRight.contains(QRegExp("[A-Za-z0-9._]")) )
          {
            tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
            break;
          }
      }

    if (!tc.selectedText().isEmpty()) tc.removeSelectedText();
    tc.insertText(completion);

    //if OnRight is "(", remove the "()" on the left if exist
    //qDebug() << "OnRight" << OnRight;
    if (OnRight == "(")
      {
        tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 2);
        if (tc.selectedText() == "()") tc.removeSelectedText();
      }
}

bool CompletingTextEditClass::findInList(QString text, QString& tmp)
{
  for (int i=0; i<functionList.size(); i++)
    {
      tmp = functionList[i];
      if (tmp.remove(text).startsWith("("))
        {
          tmp = functionList[i];
          return true;
        }
    }
  return false;
}

void CompletingTextEditClass::onCursorPositionChanged()
{    
  QString thisOne = SelectObjFunctUnderCursor();
  QString tmp;
  bool fFound = findInList(thisOne, tmp); // cursor is on one of defined functions
  if (!fFound)
    {
      QTextCursor tc = textCursor();
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
  if (fFound) QToolTip::showText( mapToGlobal(cursorRect().topRight()), tmp );
  else QToolTip::hideText();

  QList<QTextEdit::ExtraSelection> extraSelections;
  QTextCursor tc = textCursor();

  //highlight the line
  tc.select(QTextCursor::LineUnderCursor);
  QColor color = QColor(Qt::gray).lighter(140);
  QTextEdit::ExtraSelection extra;
  extra.format.setBackground(color);
  extra.cursor = tc;
  extraSelections.append(extra);
  setExtraSelections(extraSelections);

  // highlight same text if not in Exclude list
  tc = textCursor();
  tc.select(QTextCursor::WordUnderCursor);
  QString selection = tc.selectedText();
  color = QColor(Qt::green).lighter(170);
  QRegExp exl("[0-9 (){}\\[\\]=+\\-*/\\|~^.,:;\"'<>\\#\\$\\&\\?]");
  QString test = selection.simplified();
  test.remove(exl);
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
      setExtraSelections(extraSelections);
    }

  //checking for '}' on the left
  tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
  if (tc.selectedText() == "}" || tc.selectedText() == ")" || tc.selectedText() == "]")
    {
      QString same, inverse;
      if (tc.selectedText() == "}")
        {
          same = "}"; inverse = "{";
        }
      else if (tc.selectedText() == ")")
        {
          same = ")"; inverse = "(";
        }
      else if (tc.selectedText() == "]")
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
      setExtraSelections(extraSelections);
      return;
    }

  tc = textCursor();
  //checking for '{' on the right
  tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
  if (tc.selectedText() == "{" || tc.selectedText() == "(" || tc.selectedText() == "[")
    {
      QString same, inverse;
      if (tc.selectedText() == "{")
        {
          same = "{"; inverse = "}";
        }
      else if (tc.selectedText() == "(")
        {
          same = "("; inverse = ")";
        }
      else if (tc.selectedText() == "[")
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
      setExtraSelections(extraSelections);
      return;
    }
}

QString CompletingTextEditClass::textUnderCursor() const
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

QString CompletingTextEditClass::SelectObjFunctUnderCursor(QTextCursor *cursor) const
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
