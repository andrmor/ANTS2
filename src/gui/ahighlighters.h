#ifndef AHIGHLIGHTERS_H
#define AHIGHLIGHTERS_H


#include <QSyntaxHighlighter>

class AHighlighterScriptWindow : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    AHighlighterScriptWindow(QTextDocument *parent = 0);
    virtual ~AHighlighterScriptWindow() {}

    void setCustomCommands(QStringList functions, QStringList constants = QStringList());

protected:
    void highlightBlock(const QString &text);
    bool bMultilineCommentAllowed = true;

//private:
    struct HighlightingRule
      {
        QRegExp pattern;
        QTextCharFormat format;
      };
    QVector<HighlightingRule> highlightingRules;

    QRegExp commentStartExpression;
    QRegExp commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat customKeywordFormat;
//    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat charFormat;
//    QTextCharFormat functionFormat;
};

class AHighlighterLrfScript : public AHighlighterScriptWindow
{
    Q_OBJECT
public:
    AHighlighterLrfScript(QTextDocument *parent = 0);

private:
    void setFixedVariables();
};

class AHighlighterPythonScriptWindow : public AHighlighterScriptWindow
{
    Q_OBJECT
public:
    AHighlighterPythonScriptWindow(QTextDocument *parent = 0);
};

#endif // AHIGHLIGHTERS_H
