#include "aremotewindow.h"
#include "ui_aremotewindow.h"
#include "agridrunner.h"
#include "aremoteserverrecord.h"
#include "aserverdelegate.h"
#include "mainwindow.h"

#include <QPlainTextEdit>
#include <QDebug>

ARemoteWindow::ARemoteWindow(MainWindow *MW) :
    MW(MW), ui(new Ui::ARemoteWindow)
{
    ui->setupUi(this);
    ui->lwServers->setViewMode(QListView::ListMode);
    ui->lwServers->setSpacing(1);
    ui->twLog->clear();

    AddNewServer();

    GR = new AGridRunner();
    QObject::connect(GR, &AGridRunner::requestTextLog, this, &ARemoteWindow::onTextLogReceived/*, Qt::QueuedConnection*/);
    QObject::connect(GR, &AGridRunner::requestGuiUpdate, this, &ARemoteWindow::onGuiUpdate/*, Qt::QueuedConnection*/);
}

ARemoteWindow::~ARemoteWindow()
{
    delete GR;
    delete ui;

    for (ARemoteServerRecord* r : Records) delete r;
    Records.clear();
    for (AServerDelegate* d : Delegates) delete d;
    Delegates.clear();
}

void ARemoteWindow::AddNewServer()
{
    ARemoteServerRecord* record = new ARemoteServerRecord();
    Records << record;
    AServerDelegate* delegate = new AServerDelegate(record);
    Delegates << delegate;

    QListWidgetItem* item = new QListWidgetItem();
    ui->lwServers->addItem(item);    
    ui->lwServers->setItemWidget(item, delegate);
    item->setSizeHint(delegate->sizeHint());
    QObject::connect(delegate, &AServerDelegate::nameWasChanged, this, &ARemoteWindow::onNameWasChanged);
    ui->lwServers->updateGeometry();

    QPlainTextEdit* pte = new QPlainTextEdit();
    ui->twLog->addTab(pte, record->Name);
}

void ARemoteWindow::on_pbAdd_clicked()
{
    AddNewServer();
}

void ARemoteWindow::onTextLogReceived(int index, const QString message)
{
    QWidget* w = ui->twLog->widget(index);
    QPlainTextEdit* pte = dynamic_cast<QPlainTextEdit*>(w);
    if (w) pte->appendPlainText(message);
    else qDebug() << "Cannot find pte for index"<<index<<"to display"<<message;

}

void ARemoteWindow::onGuiUpdate()
{
    for (AServerDelegate* d : Delegates)
        d->updateGui();
}

void ARemoteWindow::onNameWasChanged()
{
    int index = ui->lwServers->currentRow();
    const QString newName = Records.at(index)->Name;
    ui->twLog->setTabText(index, newName);
}

void ARemoteWindow::on_pbStatus_clicked()
{
    GR->CheckStatus(Records);
}

#include "aconfiguration.h"
void ARemoteWindow::on_pbSimulate_clicked()
{
    GR->Simulate(Records, &MW->Config->JSON);
}
