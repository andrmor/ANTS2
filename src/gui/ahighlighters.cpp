#include "ahighlighters.h"

AHighlighterScriptWindow::AHighlighterScriptWindow(QTextDocument *parent)
   : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

         keywordFormat.setForeground(Qt::blue);
         keywordFormat.setFontWeight(QFont::Bold);
         QStringList keywordPatterns;
         keywordPatterns << "\\bbreak\\b" << "\\bcatch\\b" << "\\bcontinue\\b" << "\\b.length\\b" << "\\barguments\\b"
                         << "\\bdo\\b" << "\\bwhile\\b" << "\\bfor\\b"
                         << "\\bin\\b" << "\\bfunction\\b" << "\\bif\\b"
                         << "\\belse\\b" << "\\breturn\\b" << "\\bswitch\\b"
                         << "\\bthrow\\b" << "\\btry\\b" << "\\bvar\\b" << "\\bpush\\b" << "\\btypeof\\b"
                         << "\\Math.\\b" << "\\Array.\\b" << "\\String.\\b";
         foreach (const QString &pattern, keywordPatterns) {
             rule.pattern = QRegExp(pattern);
             rule.format = keywordFormat;
             highlightingRules.append(rule);
         }

       /*
         classFormat.setFontWeight(QFont::Bold);
         classFormat.setForeground(Qt::darkMagenta);
         rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
         rule.format = classFormat;
         highlightingRules.append(rule);
*/

         singleLineCommentFormat.setForeground(Qt::darkGreen);
         rule.pattern = QRegExp("//[^\n]*");
         rule.format = singleLineCommentFormat;
         highlightingRules.append(rule);

         multiLineCommentFormat.setForeground(Qt::darkGreen);

         quotationFormat.setForeground(Qt::darkGreen);
         QRegExp rx("\".*\"");
         rx.setMinimal(true); //fixes the problem with "xdsfdsfds" +variable+ "dsfdsfdsf"
         rule.pattern = rx;
         rule.format = quotationFormat;
         highlightingRules.append(rule);

         charFormat.setForeground(Qt::darkGreen);
         rule.pattern = QRegExp("'.'");
         rule.format = charFormat;
         highlightingRules.append(rule);
/*
         functionFormat.setFontItalic(true);
         functionFormat.setForeground(Qt::blue);
         rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
         rule.format = functionFormat;
         highlightingRules.append(rule);
*/

         commentStartExpression = QRegExp("/\\*");
         commentEndExpression = QRegExp("\\*/");
}



void AHighlighterScriptWindow::setCustomCommands(QStringList functions, QStringList constants)
{
    HighlightingRule rule;

    customKeywordFormat.setForeground(Qt::darkCyan);
    customKeywordFormat.setFontWeight(QFont::Bold);
//    customKeywordFormat.setFontItalic(true);

    QVector<HighlightingRule> hr;
    foreach (const QString &pattern, functions)
      {
        rule.pattern = QRegExp("\\b"+pattern+"(?=\\()");
        rule.format = customKeywordFormat;
        hr.append(rule);
      }
    foreach (const QString &pattern, constants)
      {
        rule.pattern = QRegExp("\\b"+pattern+"\\b(?![\\(\\{\\[])");
        rule.format = customKeywordFormat;
        hr.append(rule);
      }

    highlightingRules = hr + highlightingRules; //so e.g. comments and quatation rule have higher priority
}

void AHighlighterScriptWindow::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules)
      {
             QRegExp expression(rule.pattern);
             int index = expression.indexIn(text);
             while (index >= 0) {
                 int length = expression.matchedLength();
                 setFormat(index, length, rule.format);
                 index = expression.indexIn(text, index + length);
             }
      }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
             startIndex = commentStartExpression.indexIn(text);

    while (startIndex >= 0)
      {
             int endIndex = commentEndExpression.indexIn(text, startIndex);
             int commentLength;
             if (endIndex == -1) {
                 setCurrentBlockState(1);
                 commentLength = text.length() - startIndex;
             } else {
                 commentLength = endIndex - startIndex
                                 + commentEndExpression.matchedLength();
             }
             setFormat(startIndex, commentLength, multiLineCommentFormat);
             startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
       }
}


AHighlighterLrfScript::AHighlighterLrfScript(QTextDocument *parent)
  : AHighlighterScriptWindow(parent)
{
  setFixedVariables();
}

void AHighlighterLrfScript::setFixedVariables()
{
  QStringList variables;
  variables << "r" << "R";

  HighlightingRule rule;

  //customKeywordFormat.setForeground(Qt::darkCyan);
  customKeywordFormat.setForeground(Qt::cyan);
  customKeywordFormat.setFontWeight(QFont::Bold);
//    customKeywordFormat.setFontItalic(true);

  QVector<HighlightingRule> hr;
//  foreach (const QString &pattern, functions)
//    {
//      rule.pattern = QRegExp("\\b"+pattern+"(?=\\()");
//      rule.format = customKeywordFormat;
//      hr.append(rule);
//    }
  foreach (const QString &pattern, variables)
    {
      rule.pattern = QRegExp("\\b"+pattern+"\\b(?![\\(\\{\\[])");
      rule.format = customKeywordFormat;
      hr.append(rule);
    }

  highlightingRules = hr + highlightingRules; //so e.g. comments and quatation rule have higher priority
}
