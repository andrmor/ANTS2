#include "ageobasedelegate.h"
#include "aonelinetextedit.h"
#include "ageoconsts.h"

#include <QDebug>
#include <QCompleter>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>

AGeoBaseDelegate::AGeoBaseDelegate(QWidget *ParentWidget) :
    ParentWidget(ParentWidget) {}

bool AGeoBaseDelegate::isLeEmpty(const QVector<AOneLineTextEdit *> & v) const
{
    for (AOneLineTextEdit * a : v)
        if (a->text().isEmpty()) return true;

    return false;
}

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

void AGeoBaseDelegate::configureHighligherAndCompleter(AOneLineTextEdit * edit, int iUntilIndex)
{
    const AGeoConsts & GC = AGeoConsts::getConstInstance();

    int numConsts = GC.countConstants();
    if (iUntilIndex == -1 || iUntilIndex > numConsts) iUntilIndex = numConsts;

    ABasicHighlighter * highlighter = new ABasicHighlighter(edit->document());

    QTextCharFormat GeoConstantFormat;
    GeoConstantFormat.setForeground(Qt::darkMagenta);
    //GeoConstantFormat.setFontWeight(QFont::Bold);

    QStringList sl;
    AHighlightingRule rule;
    for (int i = 0; i < iUntilIndex; i++)
    {
        const QString & name = GC.getName(i);
        if (name.isEmpty()) continue;

        rule.pattern = QRegExp("\\b" + name + "\\b");
        rule.format = GeoConstantFormat;
        highlighter->HighlightingRules.append(rule);
        sl << name;
    }

    QTextCharFormat FormulaFormat;
    FormulaFormat.setForeground(Qt::blue);
    //GeoConstantFormat.setFontWeight(QFont::Bold);

    const QVector<QString> & words = AGeoConsts::getConstInstance().getTFormulaReservedWords();
    for (const QString & word : words)
    {
        rule.pattern = QRegExp("\\b" + word + "\\b");
        rule.format = FormulaFormat;
        highlighter->HighlightingRules.append(rule);
    }

    edit->Completer = new QCompleter(sl, edit);
    edit->Completer->setCaseSensitivity(Qt::CaseInsensitive); //Qt::CaseSensitive
    edit->Completer->setCompletionMode(QCompleter::PopupCompletion); //QCompleter::UnfilteredPopupCompletion
    edit->Completer->setFilterMode(Qt::MatchContains); //Qt::MatchStartsWith
    edit->Completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel); //QCompleter::CaseSensitivelySortedModel
    edit->Completer->setWrapAround(false);

    edit->Completer->setWidget(edit);
    QObject::connect(edit->Completer, SIGNAL(activated(QString)), edit, SLOT(insertCompletion(QString)));
}

bool AGeoBaseDelegate::processEditBox(AOneLineTextEdit *lineEdit, double &val, QString &str, QWidget *parent)
{
    str = lineEdit->text();
    if (str.isEmpty())
    {
        QMessageBox::warning(parent, "", "Empty line!");
        return false;
    }

    const AGeoConsts & GC = AGeoConsts::getConstInstance();
    QString errorStr;
    bool ok = GC.updateParameter(errorStr, str, val, false, false, false);
    if (ok) return true;
    QMessageBox::warning(parent, "", errorStr);
    return false;
}
