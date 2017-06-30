#include "amaterialparticlecolection.h"
#include "checkupwindowclass.h"
#include "ui_checkupwindowclass.h"
#include "mainwindow.h"
#include "detectorclass.h"
#include "pms.h"
#include "pmtypeclass.h"
#include "materialinspectorwindow.h"
#include "generalsimsettings.h"
#include "particlesourcesclass.h"
#include "globalsettingsclass.h"

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDebug>
#include <QPainter>
#include <QMenu>
#include <QScrollBar>
#include <QFileDialog>
#include <QCloseEvent>

#include "TGeoManager.h"

CheckUpWindowClass::CheckUpWindowClass(MainWindow *parent, DetectorClass *detector) :
    QMainWindow(parent),
    ui(new Ui::CheckUpWindowClass)
{
    MW = parent;
    Detector = detector;
    ui->setupUi(this);

    Qt::WindowFlags windowFlags = (Qt::Window | Qt::CustomizeWindowHint);
    windowFlags |= Qt::WindowCloseButtonHint;
    windowFlags |= Qt::WindowStaysOnTopHint;
    this->setWindowFlags( windowFlags );

    setupTable(ui->opticsTable, QStringList() << "Refractive" << "Absorption" << "Primary scint." << "Secondary scint.", false);
    connect(ui->opticsTable->verticalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(onopticsTable_rowSelected(int)));

    setupTable(ui->pmtsTable, QStringList() << "Detect. Effic. Scalar" << "Detect. Effic. Wave." << "Angular Response" << "Area Response", true);
    connect(ui->pmtsTable->verticalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(onpmtsTable_rowSelected(int)));

    iconRed = createColorCircleIcon(ui->tabWidget->iconSize(), Qt::red);
    iconYellow = createColorCircleIcon(ui->tabWidget->iconSize(), Qt::yellow);
    iconGreen = createColorCircleIcon(ui->tabWidget->iconSize(), Qt::green);
    for(int i = 0; i < ui->tabWidget->tabBar()->count(); i++)
        ui->tabWidget->setTabIcon(i, iconGreen);
    //ui->tabWidget->setCurrentIndex(0);
}

void CheckUpWindowClass::setupTable(QTableWidget *table, const QStringList &columns, bool columnsHaveIcons)
{
    table->setColumnCount(columns.length());
    table->setHorizontalHeaderLabels(columns);
    if(columnsHaveIcons)
    {
        table->setIconSize(ui->tabWidget->iconSize());
        for(int i = 0; i < table->columnCount(); i++)
            table->horizontalHeaderItem(i)->setIcon(iconGreen);
    }
    table->resizeColumnsToContents();

    int columnmax = 0;
    for(int i = 0; i < table->columnCount(); i++)
        if(table->columnWidth(i) > columnmax)
            columnmax = table->columnWidth(i);
    if(columnsHaveIcons)
        columnmax += table->iconSize().width()+3;

    for(int i = 0; i < table->columnCount(); i++)
        table->setColumnWidth(i, columnmax);
}

CheckUpWindowClass::~CheckUpWindowClass()
{
    delete ui;
}

QIcon& CheckUpWindowClass::getIcon(TriState state)
{
    switch(state)
    {
        case TriStateOk: return iconGreen;
        case TriStateWarning: return iconYellow;
        case TriStateError: return iconRed;
        default: return iconRed;
    }
}

TriState CheckUpWindowClass::setTabState(int tab, TriState state)
{
    ui->tabWidget->setTabIcon(tab, getIcon(state));
    return state;
}

TriState CheckUpWindowClass::checkTable(QTableWidget *table, bool setColumnIcons)
{
    TriState state = TriStateOk;
    QVector<TriState> states(table->columnCount(), TriStateOk);

    for(int i = 0; i < table->rowCount(); i++)
        if(!table->isRowHidden(i))
            for(int j = 0; j < table->columnCount(); j++)
                states[j] &= dynamic_cast<CheckUpTableItem *>(table->item(i, j))->checkUp();

    for(int j = 0; j < table->columnCount(); j++)
    {
        state &= states[j];
        if(setColumnIcons)
            table->horizontalHeaderItem(j)->setIcon(getIcon(state));
    }

    table->verticalHeader()->adjustSize();

    //If this is not done, table will be incomplete. Yet Another Qt Mystery
    table->resize(table->minimumSize());
    adjustTable(table);
    table->repaint();
    return state;
}

TriState CheckUpWindowClass::CheckGeoOverlaps()
{
    if(!MW->Detector->GeoManager)
    {
        ui->overlaplist->setVisible(false);
        ui->labelOverlaps->setVisible(false);
        ui->labOver->setVisible(false);
        return setTabState(0, TriStateError);
    }

    int overlapCount = MW->Detector->checkGeoOverlaps();
    TObjArray* overlaps = MW->Detector->GeoManager->GetListOfOverlaps();

    ui->overlaplist->setVisible(overlapCount>0);
    ui->labOver->setVisible(overlapCount>0);
    ui->pbSaveOverlaps->setVisible(overlapCount>0);
    if (overlapCount==1)
        ui->labOver->setText("There is an overlap:");
    else
        ui->labOver->setText("There are "+QString::number(overlapCount)+" overlaps:");
    ui->labelOverlaps->setVisible(overlapCount==0);
    ui->overlaplist->clear();

    for (Int_t i=0; i<overlapCount; i++)
        ui->overlaplist->addItem(QString::fromLocal8Bit(((TNamed*)overlaps->At(i))->GetTitle()));

    return setTabState(0, overlapCount ? TriStateError : TriStateOk);
}

TriState CheckUpWindowClass::CheckOptics()
{
    ui->opticsTable->setVisible(MW->isWavelengthResolved());
    ui->labelOptics->setVisible(!MW->isWavelengthResolved());
    if(!MW->isWavelengthResolved())
        return setTabState(1, TriStateOk);

    for(int i = 0; i < MW->MpCollection->countMaterials(); i++)
    {
        int oldindex;
        QString mat = (*MW->MpCollection)[i]->name;
        for(oldindex = i; oldindex < ui->opticsTable->rowCount(); oldindex++)
            if(ui->opticsTable->verticalHeaderItem(oldindex)->text() == mat)
                break;
        if(oldindex == ui->opticsTable->rowCount() || oldindex != i)
        {
            if(oldindex < ui->opticsTable->rowCount() && oldindex != i)
                ui->opticsTable->removeRow(oldindex);
            ui->opticsTable->insertRow(i);
            ui->opticsTable->setVerticalHeaderItem(i, new QTableWidgetItem(mat));
            ui->opticsTable->setItem(i, 0, new RefractionCheckUpItem(MW));
            ui->opticsTable->setItem(i, 1, new AbsorptionCheckUpItem(MW));
            ui->opticsTable->setItem(i, 2, new PrimaryScintCheckUpItem(MW));
            ui->opticsTable->setItem(i, 3, new SecondaryScintCheckUpItem(MW));
        }
    }
    for(int i = ui->opticsTable->rowCount()-1; i >= MW->MpCollection->countMaterials(); i--)
        ui->opticsTable->removeRow(i);

    if (!Detector->fSecScintPresent) ui->opticsTable->hideColumn(3);
    else ui->opticsTable->showColumn(3);

    return setTabState(1, checkTable(ui->opticsTable, false));
}

TriState CheckUpWindowClass::CheckPMs()
{    
       // Andr:  changed flag reads -> use general sim config
    QJsonObject json;
    MW->SimGeneralConfigToJson(json);
    GeneralSimSettings SimSet;
    SimSet.readFromJson(json);

    //if(!MW->PMs || !MW->WavelengthResolved) ui->pmtsTable->hideColumn(1);
    if( !SimSet.fWaveResolved ) ui->pmtsTable->hideColumn(1);
    else ui->pmtsTable->showColumn(1);
    //if(!MW->PMs || !MW->PMs->isAngularResolved()) ui->pmtsTable->hideColumn(2);
    if( !SimSet.fAngResolved ) ui->pmtsTable->hideColumn(2);
    else ui->pmtsTable->showColumn(2);
    //if(!MW->PMs || !MW->PMs->isAreaResolved()) ui->pmtsTable->hideColumn(3);
    if( !SimSet.fAreaResolved ) ui->pmtsTable->hideColumn(3);
    else ui->pmtsTable->showColumn(3);

    //for(int i = 0; i < ui->pmtsTable->rowCount(); i++)
    //    ui->pmtsTable->setRowHidden(i, MW->PMs->isPassivePM(i));
    for(int i = ui->pmtsTable->rowCount(); i < MW->PMs->count(); i++)
    {
        ui->pmtsTable->insertRow(i);
        ui->pmtsTable->setVerticalHeaderItem(i, new QTableWidgetItem(QString::number(i)));
        ui->pmtsTable->setItem(i, 0, new DESCheckUpItem(MW));
        ui->pmtsTable->setItem(i, 1, new DEWCheckUpItem(MW));
        ui->pmtsTable->setItem(i, 2, new AngularResponseCheckUpItem(MW));
        ui->pmtsTable->setItem(i, 3, new AreaResponseCheckUpItem(MW));
        //ui->pmtsTable->setRowHidden(i, MW->PMs->isPassivePM(i));
    }
    for(int i = ui->pmtsTable->rowCount()-1; i >= MW->PMs->count(); i--)
        ui->pmtsTable->removeRow(i);

    return setTabState(2, checkTable(ui->pmtsTable, true));
}

TriState CheckUpWindowClass::CheckInteractions()
{
    int error;

    ui->listInteraction->clear();

    //Check all materials and particles interactions
    for(int iMat = 0; iMat < MW->MpCollection->countMaterials(); iMat++)
        for(int iPart = 0; iPart < Detector->MpCollection->countParticles(); iPart++)
        {
            AMaterial *mat = (*(MW->MpCollection))[iMat];
            if( (error = MW->MpCollection->CheckMaterial(iMat, iPart)) )
            {
                QString partname = Detector->MpCollection->getParticleName(iPart);
                QString errormsg = MW->MpCollection->getErrorString(error);
                ui->listInteraction->addItem(partname + " particle on " + mat->name + " -> " + errormsg);
            }
        }

    //Save separator position
    int listIndex = ui->listInteraction->count();

    //Check all particle sources: if links are valid, if interaction data for a given energy is available
    for(int i = 0; i < MW->ParticleSources->size(); i++)
    {
        ParticleSourceStructure* source = MW->ParticleSources->getSource(i);
        int sourceParticleCount = source->GunParticles.size();

        //Loop over all GunParticles
        for(int j = 0; j < sourceParticleCount; j++)
        {
            GunParticleStruct *part = source->GunParticles[j];
            const QString partname = Detector->MpCollection->getParticleName(part->ParticleId);

            //Check energy range
            if( (error = MW->MpCollection->CheckParticleEnergyInRange(part->ParticleId, part->energy)) >= 0 )
            {
                AMaterial *mat = (*(MW->MpCollection))[error];
                const QString &msg = QString("  [%1] -> %2 particle lacks interaction description on %3 for %4 energy").arg(i).arg(partname, mat->name).arg(part->energy);
                ui->listInteraction->addItem(msg);
            }

            //Check valid linkage
            if(!part->Individual && (part->LinkedTo < 0 || part->LinkedTo >= sourceParticleCount))
                ui->listInteraction->addItem(QString("  [%1] -> %2 particle is linked to inexistent particle index %3").arg(i).arg(partname).arg(part->LinkedTo));
        }
    }

    //Manage separator for easier visualization
    if(ui->listInteraction->count() != listIndex)
    {
        ui->listInteraction->insertItem(listIndex, "Particles Sources:");
        if(listIndex > 0)
            ui->listInteraction->insertItem(listIndex, "");
    }

    //Change ui according to checkup results
    ui->listInteraction->setVisible(ui->listInteraction->count() > 0);
    ui->labelInteraction->setVisible(!ui->listInteraction->isVisible());
    if(ui->listInteraction->count() > 1)
        return setTabState(3, TriStateError);
    else
    {
        ui->listInteraction->clear();
        return setTabState(3, TriStateOk);
    }
}

void CheckUpWindowClass::on_overlaplist_clicked(const QModelIndex &index)
{
    QString clicked = ui->overlaplist->item(index.row())->text();
    QStringList overlap = clicked.split(" overlapping ");
    MW->Detector->GeoManager->PushPath();
    //qDebug() << MW->GeoManager->FindVolumeFast(overlap.at(0).toStdString().c_str())->GetName(); //even this fails
    //MW->GeoManager->FindVolumeFast(overlap.at(0).toStdString().c_str())->Draw();
    //MW->GeoManager->FindVolumeFast(overlap.at(1).toStdString().c_str())->Draw();
    MW->Detector->GeoManager->PopPath();
}

void CheckUpWindowClass::on_tabWidget_currentChanged(int index) { refreshTab(index); }

TriState CheckUpWindowClass::Refresh() { return refreshTab(ui->tabWidget->currentIndex()); }

TriState CheckUpWindowClass::refreshTab(int index)
{
    switch(index)
    {
        case 0: return CheckGeoOverlaps();
        case 1: return CheckOptics();
        case 2: return CheckPMs();
        case 3: return CheckInteractions();
    }
    return TriStateError;
}

void CheckUpWindowClass::adjustTable(QTableWidget *table)
{
    adjustTable(table, ui->tabWidget->width()-8, ui->tabWidget->height()-2*ui->tabWidget->tabBar()->height());
    //why 2*? Because! (It doesn't work well otherwise) Long live Qt!
}

void CheckUpWindowClass::adjustTable(QTableWidget *table, int maxw, int maxh)
{
    int tw = table->verticalHeader()->width();
    for(int i = 0; i < table->columnCount(); i++) {
        tw += table->columnWidth(i);
    }
    int th = table->horizontalHeader()->height();
    for(int i = 0; i < table->rowCount(); i++) {
        th += table->rowHeight(i);
    }
    if(tw > maxw && th > maxh)
    {
        tw = maxw;
        th = maxh;
    }
    else if(th > maxh)
    {
        th = maxh;
        tw += table->verticalScrollBar()->width()+4;
        if(tw > maxw)
            tw = maxw;
    }
    else if(tw > maxw)
    {
        tw = maxw;
        th += table->horizontalScrollBar()->height()+4;
        if(th > maxh)
            th = maxh;
    }
    table->resize(tw, th);
}

void CheckUpWindowClass::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    CheckAll();
}

void CheckUpWindowClass::closeEvent(QCloseEvent *event)
{
    //qDebug()<<"Close event received";
    this->hide();
    event->ignore();
    //qDebug()<<"done!";
}

void CheckUpWindowClass::onopticsTable_rowSelected(int row)
{
    MW->MIwindow->showNormal();
    MW->MIwindow->raise();
    MW->MIwindow->activateWindow();
    MW->MIwindow->SetMaterial(row);
}

void CheckUpWindowClass::onpmtsTable_rowSelected(int row)
{
    row += 1;
    //Select PM
}

void CheckUpWindowClass::on_opticsTable_customContextMenuRequested(const QPoint &p)
{
    dynamic_cast<CheckUpTableItem *>(ui->opticsTable->itemAt(p))->toggleEnabled();
    TriState state = TriStateOk;
    for(int i = 0; i < ui->opticsTable->rowCount(); i++)
        for(int j = 0; j < ui->opticsTable->columnCount(); j++)
            state &= dynamic_cast<CheckUpTableItem *>(ui->opticsTable->item(i, j))->getState();
    setTabState(1, state);
}

void CheckUpWindowClass::on_pmtsTable_customContextMenuRequested(const QPoint &p)
{
    dynamic_cast<CheckUpTableItem *>(ui->pmtsTable->itemAt(p))->toggleEnabled();
    TriState state = TriStateOk;
    for(int j = 0; j < ui->pmtsTable->columnCount(); j++)
    {
        TriState statecol = TriStateOk;
        for(int i = 0; i < ui->pmtsTable->rowCount(); i++)
            statecol &= dynamic_cast<CheckUpTableItem *>(ui->pmtsTable->item(i, j))->getState();
        ui->pmtsTable->horizontalHeaderItem(j)->setIcon(getIcon(statecol));
        state &= statecol;
    }
    setTabState(2, state);
}

QIcon CheckUpWindowClass::createColorCircleIcon(QSize size, Qt::GlobalColor color)
{
    QPixmap pm(size.width(), size.height());
    pm.fill(Qt::transparent);
    QPainter b(&pm);
    b.setBrush(QBrush(color));
    b.drawEllipse(0, 0, size.width()-1, size.width()-1);
    return QIcon(pm);
}

/*********************************************************************************
 *                            CheckUp Helper Classes                             *
 ********************************************************************************/

TriState operator &(const TriState a, const TriState b) { return a > b ? a : b; }
TriState operator &=(TriState &a, const TriState b) { if(a < b) a = b; return a; }
TriState operator |(const TriState a, const TriState b) { return a < b ? a : b; }
TriState operator |=(TriState &a, const TriState b) { if(a > b) a = b; return a; }

const QColor CheckUpItem::ColorDisabled = Qt::white;
const QColor CheckUpItem::Color[] = { Qt::green, Qt::yellow, Qt::red };
const QString CheckUpItem::wavelenToolTips[] = { " is Ok", " had to be extrapolated", " is not defined" };

TriState CheckUpItem::rangeCheck(const QVector<double> &vec, double from, double to)
{
    if(vec.isEmpty())
        return TriStateError;
    if(from < vec.at(0) || to > vec.last())
        return TriStateWarning;
    else
        return TriStateOk;
}

TriState CheckUpItem::checkUp() { return this->enabled ? doCheckUp() : TriStateOk; }
bool CheckUpItem::toggleEnabled() { return (this->enabled = !this->enabled); }
TriState CheckUpItem::getState() const { return enabled ? state : TriStateOk; }

CheckUpTableItem::CheckUpTableItem()
{
    setFlags(Qt::NoItemFlags);
    setForeground(Qt::black);
    setFont(QFont("Serif", 12, QFont::Bold));
    setTextAlignment(Qt::AlignCenter);
    setBackground(Color[TriStateOk]);
}


TriState CheckUpTableItem::checkUp() { return (tableWidget()->isColumnHidden(column()) || tableWidget()->isRowHidden(row())) ? TriStateOk : CheckUpItem::checkUp(); }
TriState CheckUpTableItem::getState() const { return (tableWidget()->isColumnHidden(column()) || tableWidget()->isRowHidden(row())) ? TriStateOk : CheckUpItem::getState(); }

bool CheckUpTableItem::toggleEnabled()
{
    this->enabled = !this->enabled;
    if(this->enabled)
        doCheckUp();
    else
    {
        setText("");
        setToolTip("Checkup is disabled");
        setBackground(ColorDisabled);
    }
    return this->enabled;
}

TriState CheckUpTableItem::setState(TriState state, QString text)
{
    this->state = state;
    if(enabled) {
        setText(text);
        setToolTip(getToolTip());
        setBackground(Color[state]);
    }
    return state;
}

const QString RefractionCheckUpItem::getToolTip() const { return (state == TriStateOk && usingDefault) ? "Using default value of 1.0" : "Refractive index" + CheckUpItem::wavelenToolTips[state]; }
TriState RefractionCheckUpItem::doCheckUp()
{
    AMaterial *mat = (*MW->MpCollection)[row()];
    state = CheckUpItem::rangeCheck(mat->nWave_lambda, MW->WaveFrom, MW->WaveTo);
    usingDefault = (state == TriStateError) && (mat->n == 1.0);
    return usingDefault ? setState(TriStateOk, "- 1 -") : setState(state);
}

const QString AbsorptionCheckUpItem::getToolTip() const { return (state == TriStateOk && usingDefault) ? "Using default value of 0.0" : "Absorption" + CheckUpItem::wavelenToolTips[state]; }
TriState AbsorptionCheckUpItem::doCheckUp()
{
    AMaterial *mat = (*MW->MpCollection)[row()];
    state = CheckUpItem::rangeCheck(mat->absWave_lambda, MW->WaveFrom, MW->WaveTo);
    usingDefault = (state == TriStateError) && (mat->abs == 0.0);
    return usingDefault ? setState(TriStateOk, "- 0 -") : setState(state);
}

const QString PrimaryScintCheckUpItem::getToolTip() const  { return "Primary Scintillation" + CheckUpItem::wavelenToolTips[state]; }
TriState PrimaryScintCheckUpItem::doCheckUp()
{
    return setState(CheckUpItem::rangeCheck( (*MW->MpCollection)[row()]->PrimarySpectrum_lambda, MW->WaveFrom, MW->WaveTo));
}

const QString SecondaryScintCheckUpItem::getToolTip() const  { return "Secondary Scintillation" + CheckUpItem::wavelenToolTips[state]; }
TriState SecondaryScintCheckUpItem::doCheckUp() {
    AMaterial *mat = (*MW->MpCollection)[row()];
    if(MW->Detector->fSecScintPresent && mat->name == MW->Detector->GeoManager->GetVolume("SecScint")->GetMaterial()->GetName())
        return setState(CheckUpItem::rangeCheck(mat->SecondarySpectrum_lambda, MW->WaveFrom, MW->WaveTo));
    setBackground(Qt::white);
    setText("");
    setToolTip("This is not the Secondary Scintillator's material");
    return (state = TriStateOk);
}

const QString DESCheckUpItem::getToolTip() const  { return state == TriStateWarning ? "Other PMs have overriden scalars, but this one does not" : ""; }
TriState DESCheckUpItem::doCheckUp()
{
    const double effPDE = MW->PMs->getPDEeffective(row());
    if(effPDE != -1)
        return setState(TriStateOk, QString::number(effPDE, 'g', 4));

    const PMtypeClass *pmtype = MW->PMs->getType(MW->PMs->at(row()).type);
    int i = 0;
    for(; i < MW->PMs->count(); i++)
        if(MW->PMs->getPDEeffective(i) != -1 && i != row())
            break;
    return setState((i == MW->PMs->count()) ? TriStateOk : TriStateWarning, QString::number(pmtype->effectivePDE, 'g', 4));
}

const QString DEWCheckUpItem::getToolTip() const  { return "Overriden DE" + CheckUpItem::wavelenToolTips[overriden] + " and Inherited DE" + CheckUpItem::wavelenToolTips[inherited]; }
TriState DEWCheckUpItem::doCheckUp()
{
    const PMtypeClass *pmtype = MW->PMs->getType(MW->PMs->at(row()).type);
    overriden = CheckUpItem::rangeCheck(*MW->PMs->getPDE_lambda(row()), MW->WaveFrom, MW->WaveTo);
    inherited = CheckUpItem::rangeCheck(pmtype->PDE_lambda, MW->WaveFrom, MW->WaveTo);
    setState(inherited | overriden, "Over.     Inher.");

    QLinearGradient gradient(0, 0, tableWidget()->columnWidth(column()), tableWidget()->rowHeight(row()));
    gradient.setColorAt(0, overriden == TriStateError ? Qt::white : CheckUpItem::Color[overriden]);
    gradient.setColorAt(1, CheckUpItem::Color[inherited]);
    setBackground(QBrush(gradient));
    return state;
}

const QString AngularResponseCheckUpItem::getToolTip() const { return state == TriStateError ? "Neither PM Type nor PM overriden angular sensitivity are defined" : ""; }
TriState AngularResponseCheckUpItem::doCheckUp()
{
    const QVector<double> *angular = MW->PMs->getAngularSensitivityCosRefracted(row());
    const QVector<double> *typeang = &MW->PMs->getType(MW->PMs->at(row()).type)->AngularSensitivityCosRefracted;
    return setState((angular->isEmpty() && typeang->isEmpty()) ? TriStateError : TriStateOk, angular->isEmpty() ? "Inherited" : "Overriden");
}

const QString AreaResponseCheckUpItem::getToolTip() const { return state == TriStateError ? "Neither PM Type nor PM overriden area sensitivity are defined" : ""; }
TriState AreaResponseCheckUpItem::doCheckUp()
{
    const QVector< QVector<double> > *area = MW->PMs->getAreaSensitivity(row());
    const QVector< QVector<double> > *typearea = &MW->PMs->getType(MW->PMs->at(row()).type)->AreaSensitivity;
    return setState((area->isEmpty() && typearea->isEmpty()) ? TriStateError : TriStateOk, area->isEmpty() ? "Inherited" : "Overriden");
}

void CheckUpWindowClass::on_pbSaveOverlaps_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save overlaps as text", MW->GlobSet->LastOpenDir+"/Overlaps.txt", "Text files (*.txt)");
    if (fileName.isEmpty()) return;
    MW->GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();

    QFile outputFile(fileName);
    outputFile.open(QIODevice::WriteOnly);
    if(!outputFile.isOpen())
      {
        qWarning()<<"Unable to open file"<<fileName<<"for writing!";
        return;
      }
    QTextStream outStream(&outputFile);

    for (int i=0; i<ui->overlaplist->count(); i++)
        outStream << ui->overlaplist->item(i)->text() << "\r\n";

    outputFile.close();
}
