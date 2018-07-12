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
#include <QMessageBox>
#include <QDebug>

ARemoteWindow::ARemoteWindow(MainWindow *MW) :
    MW(MW), ui(new Ui::ARemoteWindow)
{
    ui->setupUi(this);
    ui->lwServers->setViewMode(QListView::ListMode);
    //ui->lwServers->setDragDropMode(QAbstractItemView::InternalMove);
    //ui->lwServers->setDragEnabled(true);
    ui->lwServers->setSpacing(1);
    ui->twLog->clear();

    AddNewServer();

    GR = new AGridRunner(MW->EventsDataHub, MW->PMs);
    GR->SetTimeout(ui->leiTimeout->text().toInt());
    QObject::connect(GR, &AGridRunner::requestTextLog, this, &ARemoteWindow::onTextLogReceived/*, Qt::QueuedConnection*/);
    QObject::connect(GR, &AGridRunner::requestStatusLog, this, &ARemoteWindow::onStatusLogReceived/*, Qt::QueuedConnection*/);
    QObject::connect(GR, &AGridRunner::requestDelegateGuiUpdate, this, &ARemoteWindow::onGuiUpdate/*, Qt::QueuedConnection*/);

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
    GR->SetTimeout(ui->leiTimeout->text().toInt());

    QJsonArray ar = json["Servers"].toArray();
    for (int i=0; i<ar.size(); i++)
    {
        ARemoteServerRecord* record = new ARemoteServerRecord();

        QJsonObject js = ar.at(i).toObject();
        record->ReadFromJson(js);

        AddNewServer(record);
    }
}

void ARemoteWindow::onBusy(bool flag)
{
    flag = !flag;
    ui->pbAdd->setEnabled(flag);
    ui->pbRemove->setEnabled(flag);
    //ui->lwServers->setEnabled(flag);
    ui->pbStatus->setEnabled(flag);
    ui->pbSimulate->setEnabled(flag);
    ui->pbReconstruct->setEnabled(flag);
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

void ARemoteWindow::onStatusLogReceived(const QString message)
{
    ui->leStatus->setText(message);
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

    onBusy(true);
    GR->CheckStatus(Records);
    onBusy(false);
}

#include "reconstructionwindow.h"
#include "outputwindow.h"
void ARemoteWindow::on_pbSimulate_clicked()
{
    WriteConfig();

    onBusy(true);
    GR->Simulate(Records, &MW->Config->JSON);
    onBusy(false);

    MW->Owindow->RefreshData();
    MW->Rwindow->OnEventsDataAdded();
}

void ARemoteWindow::on_pbReconstruct_clicked()
{
    WriteConfig();

    onBusy(true);
    GR->Reconstruct(Records, &MW->Config->JSON);
    onBusy(false);
}

void ARemoteWindow::on_pbRateServers_clicked()
{
    WriteConfig();

    const QString DefDet = MW->GlobSet->ExamplesDir + "/StartupDetector.json";
    QJsonObject js;
    bool bOK = LoadJsonFromFile(js, DefDet);
    if (!bOK) return;

    //setting flood sim (1000 ev by default)
    QJsonObject sc = js["SimulationConfig"].toObject();
        QJsonObject psc = sc["PointSourcesConfig"].toObject();
            QJsonObject co = psc["ControlOptions"].toObject();
                co["Single_Scan_Flood"] = 2;
            psc["ControlOptions"] = co;
        sc["PointSourcesConfig"] = psc;
    js["SimulationConfig"] = sc;

    onBusy(true);
    GR->RateServers(Records, &js);
    onBusy(false);

    for (AServerDelegate* d : Delegates)
        d->updateGui();
}

void ARemoteWindow::on_leiTimeout_editingFinished()
{
    GR->SetTimeout(ui->leiTimeout->text().toInt());
}

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
