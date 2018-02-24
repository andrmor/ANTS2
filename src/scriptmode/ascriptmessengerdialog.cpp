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
    init(false);
}

AScriptMessengerDialog::~AScriptMessengerDialog()
{
    qDebug() << "destructor for AScriptMessengerDialog";
    delete D; D = 0;
}

bool AScriptMessengerDialog::IsShown() const
{
    return bShowStatus;
}

void AScriptMessengerDialog::RestoreWidget()
{
    D->show();
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
    e->appendHtml(text);
}

void AScriptMessengerDialog::ShowTemporary(int ms)
{
    Show();

    QTime t;
    t.restart();
    do qApp->processEvents();
    while (t.elapsed() < ms);

    Hide();
}

void AScriptMessengerDialog::SetTransparent(bool flag)
{
    QString text = e->document()->toHtml();
    delete D; D = 0;

    init(flag);
    e->appendHtml(text);
}

void AScriptMessengerDialog::SetDialogTitle(const QString &title)
{
    Title = title;
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

void AScriptMessengerDialog::init(bool bTransparent)
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

    if (bTransparent)
    {
        D->setWindowFlags(Qt::FramelessWindowHint);
        D->setAttribute(Qt::WA_TranslucentBackground);

        e->setStyleSheet("background: rgba(0,0,255,0%)");
        e->setFrameStyle(QFrame::NoFrame);
    }
}
