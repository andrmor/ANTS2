#include "ageobasedelegate.h"

#include <QHBoxLayout>
#include <QPushButton>

AGeoBaseDelegate::AGeoBaseDelegate(QWidget *ParentWidget) :
    ParentWidget(ParentWidget) {}

QHBoxLayout *AGeoBaseDelegate::createBottomButtons()
{
    QHBoxLayout * abl = new QHBoxLayout();

    pbShow = new QPushButton("Show");
    QObject::connect(pbShow, &QPushButton::clicked, this, &AGeoBaseDelegate::RequestShow);
    abl->addWidget(pbShow);
    pbChangeAtt = new QPushButton("Color/line");
    QObject::connect(pbChangeAtt, &QPushButton::clicked, this, &AGeoBaseDelegate::RequestChangeVisAttributes);
    abl->addWidget(pbChangeAtt);
    pbScriptLine = new QPushButton("Script to clipboard");
    pbScriptLine->setToolTip("Click right mouse button to generate script recursively");
    QObject::connect(pbScriptLine, &QPushButton::clicked, this, &AGeoBaseDelegate::RequestScriptToClipboard);
    pbScriptLine->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(pbScriptLine, &QPushButton::customContextMenuRequested, this, &AGeoBaseDelegate::RequestScriptRecursiveToClipboard);
    abl->addWidget(pbScriptLine);

    return abl;
}

#include "aonelinetextedit.h"
#include "ageoconsts.h"
#include <QCompleter>
void AGeoBaseDelegate::configureHighligherAndCompleter(AOneLineTextEdit * edit)
{
    ABasicHighlighter * highlighter = new ABasicHighlighter(edit->document());

    QTextCharFormat GeoConstantFormat;
    GeoConstantFormat.setForeground(Qt::darkMagenta);
    //GeoConstantFormat.setFontWeight(QFont::Bold);

    AHighlightingRule rule;
    for (const QString & name : AGeoConsts::getConstInstance().getNames())
    {
        rule.pattern = QRegExp("\\b" + name + "\\b");
        rule.format = GeoConstantFormat;
        highlighter->HighlightingRules.append(rule);
    }

    QTextCharFormat FormulaFormat;
    FormulaFormat.setForeground(Qt::blue);
    //GeoConstantFormat.setFontWeight(QFont::Bold);

    QVector<QString> words = AGeoConsts::getTFormulaReservedWords();
    for (const QString & word : words)
    {
        rule.pattern = QRegExp("\\b" + word + "\\b");
        rule.format = FormulaFormat;
        highlighter->HighlightingRules.append(rule);
    }

    QStringList sl;
    for (const QString & name : AGeoConsts::getConstInstance().getNames()) sl << name;

    /*
    QCompleter* completer = new QCompleter(sl, edit);
    completer->setCaseSensitivity(Qt::CaseInsensitive); //Qt::CaseSensitive
    completer->setCompletionMode(QCompleter::PopupCompletion);
    //completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    completer->setFilterMode(Qt::MatchContains);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setWrapAround(false);
    */
    edit->Completer = new QCompleter(sl, edit);
    //CompletitionModel = new QStringListModel(functions, this);
    //Completer->setModel(CompletitionModel);
    //edit->Completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    edit->Completer->setCompletionMode(QCompleter::PopupCompletion);
    edit->Completer->setFilterMode(Qt::MatchContains);
    //completer->setFilterMode(Qt::MatchStartsWith);
    edit->Completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    edit->Completer->setCaseSensitivity(Qt::CaseSensitive);
    edit->Completer->setWrapAround(false);


    edit->Completer->setWidget(edit);
    QObject::connect(edit->Completer, SIGNAL(activated(QString)), edit, SLOT(insertCompletion(QString)));
}
