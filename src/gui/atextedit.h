#ifndef ATEXTEDIT_H
#define ATEXTEDIT_H

#include <QPlainTextEdit>
#include <QObject>
#include <QWidget>
#include <QSet>

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
    void RefreshExtraHighlight();
    void setTextCursorSilently(const QTextCursor& tc);

    void setDeprecatedOrRemovedMethods(const QHash<QString, QString>* DepRem) {DeprecatedOrRemovedMethods = DepRem;}

    QStringList functionList;
    QString FindString;
    static const int TabInSpaces = 7;
    const QHash<QString, QString>* DeprecatedOrRemovedMethods = 0;

public slots:
    void paste();
    void align();

protected:
    bool event(QEvent *event);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void insertCompletion(const QString &completion);    
    void onCursorPositionChanged();
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    int  previousLineNumber = 0;
    bool bMonitorLineChange = true;

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

    void checkBracketsOnLeft(QList<QTextEdit::ExtraSelection> &extraSelections, const QColor &color);
    void checkBracketsOnRight(QList<QTextEdit::ExtraSelection> &extraSelections, const QColor &color);

    bool TryShowFunctionTooltip(QTextCursor *cursor);

    int getIndent(const QString &line) const;
    void setIndent(QString &line, int indent);
    void convertTabToSpaces(QString &line);
    int getSectionCounterChange(const QString &line) const;

signals:
    void requestHelp(QString);
    void editingFinished();
    void fontSizeChanged(int size);
    void lineNumberChanged(int lineNumber);

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
