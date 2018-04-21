#ifndef ATEXTEDIT_H
#define ATEXTEDIT_H

#include <QPlainTextEdit>
#include <QObject>
#include <QWidget>

class QCompleter;
class ALeftField;

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
    int TabInSpaces;

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void insertCompletion(const QString &completion);    
    void onCursorPositionChanged();
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QString textUnderCursor() const;
    QString SelectObjFunctUnderCursor(QTextCursor* cursor = 0) const;
    QString SelectTextToLeft(QTextCursor cursor, int num) const;
    bool InsertClosingBracket() const;
    bool findInList(QString text, QString &tmp) const;
    void setFontSizeAndEmitSignal(int size);

    friend class ALeftField;
    void paintLeftField(QPaintEvent *event);
    int  getWidthLeftField() const;

    QCompleter* c;
    ALeftField* LeftField;

signals:
    void requestHelp(QString);
    void editingFinished();
    void fontSizeChanged(int size);    
//    void requestHistoryBefore();
//    void requestHistoryAfter();
//    void requestHistoryStart();
//    void requestHistoryEnd();
};

class ALeftField : public QWidget
{
public:
    ALeftField(ATextEdit& edit) : QWidget(&edit), edit(edit) {}

    QSize sizeHint() const override { return QSize(edit.getWidthLeftField(), 0); }
protected:
    void paintEvent(QPaintEvent *event) override { edit.paintLeftField(event); }

private:
    ATextEdit& edit;
};

#endif // ATEXTEDIT_H
