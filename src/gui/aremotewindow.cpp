#include "aremotewindow.h"
#include "ui_aremotewindow.h"
#include "agridrunner.h"
#include "aremoteserverrecord.h"
#include "aserverdelegate.h"
#include "mainwindow.h"
#include "aconfiguration.h"
#include "globalsettingsclass.h"

#include "ajsontools.h"

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

    QIntValidator* intVal = new QIntValidator(this);
    intVal->setRange(1, 100000000);
    ui->leiTimeout->setValidator(intVal);
}

ARemoteWindow::~ARemoteWindow()
{
    Clear();

    delete GR;
    delete ui;
}

void ARemoteWindow::WriteConfig()
{
    QJsonObject json;

    json["Timeout"] = ui->leiTimeout->text().toInt();
    QJsonArray ar;
        for (ARemoteServerRecord* r : Records) ar << r->WriteToJson();
    json["Servers"] = ar;

    MW->GlobSet->RemoteServers = json;
}

void ARemoteWindow::ReadConfig()
{
    QJsonObject& json = MW->GlobSet->RemoteServers;
    if (json.isEmpty()) return;

    Clear();
    JsonToLineEditInt(json, "Timeout", ui->leiTimeout);

    QJsonArray ar = json["Servers"].toArray();
    for (int i=0; i<ar.size(); i++)
    {
        ARemoteServerRecord* record = new ARemoteServerRecord();

        QJsonObject js = ar.at(i).toObject();
        record->ReadFromJson(js);

        AddNewServer(record);
    }
}

void ARemoteWindow::AddNewServer(ARemoteServerRecord* record)
{
    if (!record) record = new ARemoteServerRecord();
    Records << record;
    AServerDelegate* delegate = new AServerDelegate(record);
    Delegates << delegate;

    QListWidgetItem* item = new QListWidgetItem();
    ui->lwServers->addItem(item);    
    ui->lwServers->setItemWidget(item, delegate);
    item->setSizeHint(delegate->sizeHint());
    QObject::connect(delegate, &AServerDelegate::nameWasChanged, this, &ARemoteWindow::onNameWasChanged);
    QObject::connect(delegate, &AServerDelegate::updateSizeHint, this, &ARemoteWindow::onUpdateSizeHint);
    ui->lwServers->updateGeometry();

    QPlainTextEdit* pte = new QPlainTextEdit();
    ui->twLog->addTab(pte, record->Name);
}

void ARemoteWindow::on_pbAdd_clicked()
{
    AddNewServer();
}

void ARemoteWindow::onUpdateSizeHint(AServerDelegate* d)
{
    for (int i=0; i<Delegates.size(); i++)
    {
        if (d == Delegates.at(i))
        {
            d->updateGeometry();
            if (i < ui->lwServers->count())
                ui->lwServers->item(i)->setSizeHint(d->sizeHint());
            d->updateGeometry();
            return;
        }
    }
}

void ARemoteWindow::Clear()
{
    for (AServerDelegate* d : Delegates) delete d;
    Delegates.clear();

    for (ARemoteServerRecord* r : Records) delete r;
    Records.clear();

    ui->lwServers->clear();
    ui->twLog->clear();
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
    WriteConfig();

    GR->CheckStatus(Records);
}

void ARemoteWindow::on_pbSimulate_clicked()
{
    WriteConfig();

    GR->Simulate(Records, &MW->Config->JSON);
}

void ARemoteWindow::on_leiTimeout_editingFinished()
{
    GR->SetTimeout(ui->leiTimeout->text().toInt());
}

#include <QMessageBox>
void ARemoteWindow::on_pbRemove_clicked()
{
    int index = ui->lwServers->currentRow();
    if (index < 0) return;

    ARemoteServerRecord* r = Records.at(index);

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Remove server", QString("Remove server ") + r->Name + "?",
                                    QMessageBox::Yes|QMessageBox::Cancel);
    if (reply == QMessageBox::Cancel) return;

    delete r;
    Records.remove(index);

    delete Delegates.at(index);
    Delegates.remove(index);
    delete ui->lwServers->takeItem(index);

    ui->twLog->removeTab(index);
}
