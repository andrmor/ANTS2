#include "ascriptmessengerdialog.h"

#include <QWidget>
#include <QTime>
#include <QApplication>
#include <QDialog>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QDebug>

// -------------  WIDGET -----------------
AScriptMessengerDialog::AScriptMessengerDialog(QWidget *parent) :
    Parent(parent)
{
    D = new QDialog(Parent);
    //if (bMasterCopy) QObject::connect(D, &QDialog::finished, this, &AInterfaceToMessageWindow::Hide);

    QVBoxLayout* l = new QVBoxLayout;
    e = new QPlainTextEdit();
    //e->setReadOnly(true);
    l->addWidget(e);
    D->setLayout(l);

    D->setGeometry(X, Y, WW, HH);
    D->setWindowTitle(Title);

    connect(D, &QDialog::rejected, this, &AScriptMessengerDialog::onRejected);
}

AScriptMessengerDialog::~AScriptMessengerDialog()
{
    //qDebug() << "destructor for AScriptMessengerDialog";
    delete D; D = 0;
}

bool AScriptMessengerDialog::IsShown() const
{
    return bShowStatus;
}

void AScriptMessengerDialog::RestoreWidget()
{
    D->setVisible(true);
    D->showNormal();
    D->raise();
}

void AScriptMessengerDialog::HideWidget()
{
    D->hide();
}

void AScriptMessengerDialog::Show()
{
    D->show();
    D->raise();
    bShowStatus = true;
}

void AScriptMessengerDialog::Hide()
{
    D->hide();
    bShowStatus = false;
}

void AScriptMessengerDialog::Clear()
{
    e->clear();
}

void AScriptMessengerDialog::Append(const QString &text)
{
    if (!D->isVisible()) D->show();
    e->appendHtml(text);
}

void AScriptMessengerDialog::ShowTemporary(int ms)
{
    Show();

    if (ms == -1) return;

    QTime t;
    t.restart();
    do qApp->processEvents();
    while (t.elapsed() < ms);

    Hide();
}

void AScriptMessengerDialog::SetDialogTitle(const QString &title)
{
    Title = title;
    D->setWindowTitle(title);
}

void AScriptMessengerDialog::Move(double x, double y)
{
    X = x; Y = y;
    D->move(X, Y);
}

void AScriptMessengerDialog::Resize(double w, double h)
{
    WW = w; HH = h;
    D->resize(WW, HH);
}

void AScriptMessengerDialog::SetFontSize(int size)
{
    QFont f = e->font();
    f.setPointSize(size);
    e->setFont(f);
}

void AScriptMessengerDialog::onRejected()
{
    bShowStatus = false;
    Hide();
}
