/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef COMPLETINGTEXTEDITCLASS_H
#define COMPLETINGTEXTEDITCLASS_H

#include <QObject>
#include <QTextEdit>

class QCompleter;
class QWidget;

class CompletingTextEditClass : public QTextEdit
{
    Q_OBJECT
public:
    CompletingTextEditClass(QWidget *parent = 0);
    ~CompletingTextEditClass() {}

    void setCompleter(QCompleter *completer);
    QCompleter *completer() const {return c; }

    void SetFontSize(int size);

    QStringList functionList;
    int TabGivesSpaces;

protected:
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *e) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *e) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void focusOutEvent(QFocusEvent *e) Q_DECL_OVERRIDE;

private slots:
    void insertCompletion(const QString &completion);    
    void onCursorPositionChanged();

private:
    QString textUnderCursor() const;
    QString SelectObjFunctUnderCursor(QTextCursor* cursor = 0) const;
    QString SelectTextToLeft(QTextCursor cursor, int num) const;
    bool InsertClosingBracket() const;
    bool findInList(QString text, QString &tmp) const;
    void setFontSizeAndEmitSignal(int size);

    QCompleter *c;    

signals:
    void requestHelp(QString);
    void editingFinished();
    void fontSizeChanged(int size);    
//    void requestHistoryBefore();
//    void requestHistoryAfter();
//    void requestHistoryStart();
//    void requestHistoryEnd();
};

#endif // COMPLETINGTEXTEDITCLASS_H
