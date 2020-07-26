#include "aremotewindow.h"
#include "ui_aremotewindow.h"
#include "agridrunner.h"
#include "aremoteserverrecord.h"
#include "aserverdelegate.h"
#include "mainwindow.h"
#include "aconfiguration.h"
#include "aglobalsettings.h"
#include "amessage.h"
#include "anetworkmodule.h"
#include "ajsontools.h"

#include <QPlainTextEdit>
#include <QMessageBox>
#include <QDebug>

ARemoteWindow::ARemoteWindow(MainWindow *MW) :
    AGuiWindow("remote", MW),
    MW(MW), ui(new Ui::ARemoteWindow),
    GR(*MW->NetModule->GridRunner),
    Records(GR.ServerRecords)
{
    ui->setupUi(this);

    Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
    windowFlags |= Qt::WindowCloseButtonHint;
    this->setWindowFlags( windowFlags );

    ui->lwServers->setViewMode(QListView::ListMode);
    ui->lwServers->setSpacing(1);
    ui->twLog->clear();

    if (Records.isEmpty()) Records << new ARemoteServerRecord();
    for (ARemoteServerRecord * record : Records)
        AddNewServerDelegate(record);

    GR.SetTimeout(ui->leiTimeout->text().toInt());
    QObject::connect(&GR, &AGridRunner::requestTextLog, this, &ARemoteWindow::onTextLogReceived/*, Qt::QueuedConnection*/);
    QObject::connect(&GR, &AGridRunner::requestStatusLog, this, &ARemoteWindow::onStatusLogReceived/*, Qt::QueuedConnection*/);
    QObject::connect(&GR, &AGridRunner::requestDelegateGuiUpdate, this, &ARemoteWindow::onGuiUpdate/*, Qt::QueuedConnection*/);

    connect(&GR, &AGridRunner::notifySimulationFinished, MW, &MainWindow::onGridSimulationFinished);

    QIntValidator* intVal = new QIntValidator(this);
    intVal->setRange(1, 100000000);
    ui->leiTimeout->setValidator(intVal);

    ui->cbShowLog->setChecked(false);
}

ARemoteWindow::~ARemoteWindow()
{
    Clear();
    delete ui;
}

void ARemoteWindow::onBusy(bool flag)
{
    flag = !flag;
    ui->pbAdd->setEnabled(flag);
    ui->pbRemove->setEnabled(flag);
    ui->pbStatus->setEnabled(flag);
    ui->pbSimulate->setEnabled(flag);
    ui->pbReconstruct->setEnabled(flag);
    ui->pbRateServers->setEnabled(flag);
}

void ARemoteWindow::AddNewServerDelegate(ARemoteServerRecord * record)
{
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
    ARemoteServerRecord * rec = new ARemoteServerRecord();
    Records << rec;
    AddNewServerDelegate(rec);
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
    GR.writeConfig();

    onBusy(true);
    QString err = GR.CheckStatus();
    onBusy(false);

    if (!err.isEmpty()) message(err, this);
    else
    {
        ui->pbRateServers->setEnabled(true);
        ui->pbSimulate->setEnabled(true);
        ui->pbReconstruct->setEnabled(true);
    }
}

#include "reconstructionwindow.h"
#include "outputwindow.h"
void ARemoteWindow::on_pbSimulate_clicked()
{
    GR.writeConfig();

    onBusy(true);
    MW->writeSimSettingsToJson(MW->Config->JSON);
    QString err = GR.Simulate(&MW->Config->JSON);
    onBusy(false);

    if (!err.isEmpty())
        message(err, this);

    /*
    MW->Owindow->RefreshData();
    MW->Rwindow->OnEventsDataAdded();
    MW->Rwindow->ShowPositions(1, true);
    */
}

void ARemoteWindow::on_pbReconstruct_clicked()
{
    GR.writeConfig();

    onBusy(true);
    GR.Reconstruct(&MW->Config->JSON);
    onBusy(false);
}

void ARemoteWindow::on_pbRateServers_clicked()
{
    GR.writeConfig();

    const QString DefDet = MW->GlobSet.ExamplesDir + "/StartupDetector.json";
    QJsonObject js;
    bool bOK = LoadJsonFromFile(js, DefDet);
    if (!bOK)
    {
        message("Cannot open config file StartupDetector.json", this);
        return;
    }

    //setting flood sim (1000 ev by default)
    QJsonObject sc = js["SimulationConfig"].toObject();
        QJsonObject psc = sc["PointSourcesConfig"].toObject();
            QJsonObject co = psc["ControlOptions"].toObject();
                co["Single_Scan_Flood"] = 2;
            psc["ControlOptions"] = co;
        sc["PointSourcesConfig"] = psc;
    js["SimulationConfig"] = sc;

    onBusy(true);
    QString err = GR.RateServers(&js);
    onBusy(false);

    if (!err.isEmpty())
        message(err, this);

    for (AServerDelegate* d : Delegates)
        d->updateGui();
}

void ARemoteWindow::on_leiTimeout_editingFinished()
{
    GR.SetTimeout(ui->leiTimeout->text().toInt());
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

void ARemoteWindow::on_pbAbort_clicked()
{
    GR.Abort();
    onStatusLogReceived("Aborted!");
}
