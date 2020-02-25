#include "atextpavedialog.h"
#include "ui_atextpavedialog.h"
#include "aroottextconfigurator.h"
#include "arootlineconfigurator.h"

#include <QDebug>

#include "TPaveText.h"
#include "TList.h"

ATextPaveDialog::ATextPaveDialog(TPaveText &Pave, QWidget *parent) :
    QDialog(parent),
    Pave(Pave),
    ui(new Ui::ATextPaveDialog)
{
    ui->setupUi(this);

    ui->pbDummy->setDefault(true);
    ui->pbDummy->setVisible(false);

    TList * list = Pave.GetListOfLines();
    int numLines = list->GetEntries();
    for (int i=0; i<numLines; i++)
        ui->pte->appendPlainText(((TText*)list->At(i))->GetTitle());

    connect(ui->pte, &QPlainTextEdit::blockCountChanged, this, &ATextPaveDialog::updatePave);
}

ATextPaveDialog::~ATextPaveDialog()
{
    delete ui;
}

void ATextPaveDialog::on_pbDummy_clicked()
{
    updatePave();
}

void ATextPaveDialog::on_pbConfirm_clicked()
{
    updatePave();
    accept();
}

void ATextPaveDialog::on_pbTextAttributes_clicked()
{
    int   color = Pave.GetTextColor();
    int   align = Pave.GetTextAlign();
    int   font  = Pave.GetTextFont();
    float size  = Pave.GetTextSize();

    ARootTextConfigurator D(color, align, font, size, this);
    int res = D.exec();
    if (res == QDialog::Accepted)
    {
        Pave.SetTextColor(color);
        Pave.SetTextAlign(align);
        Pave.SetTextFont(font);
        Pave.SetTextSize(size);
        updatePave();
    }
}

void ATextPaveDialog::on_pbConfigureFrame_clicked()
{
    int color = Pave.GetLineColor();
    int width = Pave.GetLineWidth();
    int style = Pave.GetLineStyle();
    ARootLineConfigurator RC(&color, &width, &style, this);
    int res = RC.exec();
    if (res == QDialog::Accepted)
    {
        Pave.SetLineColor(color);
        Pave.SetLineWidth(width);
        Pave.SetLineStyle(style);
        updatePave();
    }
}

void ATextPaveDialog::updatePave()
{
    Pave.Clear();

    QString txt = ui->pte->document()->toPlainText();
    QStringList sl = txt.split("\n");
    for (QString s : sl)
        Pave.AddText(s.toLatin1());

    emit requestRedraw();
}
