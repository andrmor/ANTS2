#ifndef ATEXTEDIT_H
#define ATEXTEDIT_H

#include <QObject>
#include <QPlainTextEdit>

class QCompleter;
class QWidget;

class ATextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    ATextEdit(QWidget *parent = 0);
    ~ATextEdit() {}

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

#endif // ATEXTEDIT_H
