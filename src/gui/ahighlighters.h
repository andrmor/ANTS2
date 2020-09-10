#ifndef AHIGHLIGHTERS_H
#define AHIGHLIGHTERS_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class AHighlighterScriptWindow : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    AHighlighterScriptWindow(QTextDocument *parent = 0);
    virtual ~AHighlighterScriptWindow() {}

    void setHighlighterRules(const QStringList &functions, const QStringList &deprecatedOrRemoved, const QStringList &constants);

protected:
    void highlightBlock(const QString &text);
    bool bMultilineCommentAllowed = true;

//private:
    struct HighlightingRule
      {
        QRegularExpression pattern;
        QTextCharFormat format;
      };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat customKeywordFormat;
    QTextCharFormat unitFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat charFormat;
//    QTextCharFormat functionFormat;
    QTextCharFormat deprecatedOrRemovedFormat;
};

class AHighlighterLrfScript : public AHighlighterScriptWindow
{
    Q_OBJECT
public:
    AHighlighterLrfScript(QTextDocument *parent = 0);

private:
    void setFixedVariables();
};

class APythonHighlighter : public AHighlighterScriptWindow
{
    Q_OBJECT
public:
    APythonHighlighter(QTextDocument *parent = 0);
};

#endif // AHIGHLIGHTERS_H
