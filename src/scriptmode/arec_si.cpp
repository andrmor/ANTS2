#include "arec_si.h"
#include "areconstructionmanager.h"
#include "aconfiguration.h"
#include "eventsdataclass.h"
#include "tmpobjhubclass.h"
#include "detectorclass.h"
#include "apmgroupsmanager.h"
#include "apositionenergyrecords.h"
#include "aglobalsettings.h"

#include <QJsonArray>

#include "TH1.h"
#include "TH2D.h"

ARec_SI::ARec_SI(AReconstructionManager *RManager, AConfiguration *Config, EventsDataClass *EventsDataHub, TmpObjHubClass* TmpHub)
 : RManager(RManager), Config(Config), EventsDataHub(EventsDataHub),
   PMgroups(RManager->PMgroups), TmpHub(TmpHub)
{
    Description = "Event reconstructor";
}

void ARec_SI::ForceStop()
{
  emit RequestStopReconstruction();
}

void ARec_SI::ReconstructEvents(int NumThreads, bool fShow)
{
  if (NumThreads == -1) NumThreads = AGlobalSettings::getInstance().RecNumTreads;
  RManager->reconstructAll(Config->JSON, NumThreads, fShow);
}

void ARec_SI::UpdateFilters(int NumThreads)
{
  if (NumThreads == -1) NumThreads = AGlobalSettings::getInstance().RecNumTreads;
  RManager->filterEvents(Config->JSON, NumThreads);
}

double ARec_SI::GetChi2valueToCutTop(double cutUpper_fraction, int sensorGroup)
{
    const int numBins = 1000;

    if (!EventsDataHub->isReconstructionReady(sensorGroup)) return 0;

    TH1D* h = new TH1D("h","", numBins, 0, 0);
    for (int ievent = 0; ievent < EventsDataHub->ReconstructionData[sensorGroup].size(); ievent++)
        h->Fill(EventsDataHub->ReconstructionData[sensorGroup][ievent]->chi2);

    double integral = h->Integral();
    double sum = 0;
    for (int ibin = numBins; ibin > -1; ibin--)
    {
        sum += h->GetBinContent(ibin);
        if(sum >= integral * cutUpper_fraction)
           return ibin * h->GetBinWidth(0);
    }
    return 0;
}

void ARec_SI::DoBlurUniform(double range, bool fUpdateFilters)
{
  EventsDataHub->BlurReconstructionData(0, range, Config->GetDetector()->RandGen);
  if (fUpdateFilters) UpdateFilters();
}

void ARec_SI::DoBlurGauss(double sigma, bool fUpdateFilters)
{
  EventsDataHub->BlurReconstructionData(1, sigma, Config->GetDetector()->RandGen);
  if (fUpdateFilters) UpdateFilters();
}

int ARec_SI::countSensorGroups()
{
    return PMgroups->countPMgroups();
}

void ARec_SI::clearSensorGroups()
{
    PMgroups->clearPMgroups();
}

void ARec_SI::addSensorGroup(QString name)
{
    PMgroups->definePMgroup(name);
}

void ARec_SI::setPMsOfGroup(int igroup, QVariant PMlist)
{
    if (igroup<0 || igroup>PMgroups->countPMgroups()-1)
    {
        abort("Script: attempt to add pms to a non-existent group");
        return;
    }

    PMgroups->removeAllPmRecords(igroup);
    QString type = PMlist.typeName();
    if (type == "QVariantList")
        {
          QVariantList vl = PMlist.toList();
          //qDebug() << vl;
          QJsonArray ar = QJsonArray::fromVariantList(vl);  //TODO shorten by using directly
          for (int i=0; i<ar.size(); i++)
          {
             int ipm = ar.at(i).toInt();
             PMgroups->addPMtoGroup(ipm, igroup);
          }
          PMgroups->updateGroupsInGlobalConfig();
    }
    else abort("Script: list of PMs should be an array of ints");
}

QVariant ARec_SI::getPMsOfGroup(int igroup)
{

    if (igroup<0 || igroup>PMgroups->countPMgroups()-1)
    {
        abort("Bad sensor group number!");
        QJsonValue jv = QJsonArray();
        QVariant res = jv.toVariant();
        return res;
    }

    QJsonArray ar;
    for (int i=0; i<PMgroups->Groups.at(igroup)->PMS.size(); i++)
        if (PMgroups->Groups.at(igroup)->PMS.at(i).member) ar << i;

    QJsonValue jv = ar;
    QVariant res = jv.toVariant();
    return res;
}

bool ARec_SI::isStaticPassive(int ipm)
{
    return PMgroups->isStaticPassive(ipm);
}

void ARec_SI::setStaticPassive(int ipm)
{
    PMgroups->setStaticPassive(ipm);
}

void ARec_SI::clearStaticPassive(int ipm)
{
    PMgroups->clearStaticPassive(ipm);
}

void ARec_SI::selectSensorGroup(int igroup)
{
    PMgroups->selectActiveSensorGroup(igroup);
}

void ARec_SI::clearSensorGroupSelection()
{
    PMgroups->clearActiveSensorGroups();
}

void ARec_SI::SaveAsTree(QString fileName, bool IncludePMsignals, bool IncludeRho, bool IncludeTrue, int SensorGroup)
{
  if (SensorGroup<0 || SensorGroup>PMgroups->countPMgroups()-1)
  {
      abort("Wrong sensor group!");
      return;
  }
  EventsDataHub->saveReconstructionAsTree(fileName, Config->GetDetector()->PMs, IncludePMsignals, IncludeRho, IncludeTrue, SensorGroup);
}

void ARec_SI::SaveAsText(QString fileName)
{
  EventsDataHub->saveReconstructionAsText(fileName);
}

void ARec_SI::ClearManifestItems()
{
    EventsDataHub->clearManifest();
    emit RequestUpdateGuiForManifest();
}

void ARec_SI::AddRoundManisfetItem(double x, double y, double Diameter)
{
    ManifestItemHoleClass* m = new ManifestItemHoleClass(Diameter);
    m->X = x;
    m->Y = y;
    EventsDataHub->Manifest.append(m);
    emit RequestUpdateGuiForManifest();
}

void ARec_SI::AddRectangularManisfetItem(double x, double y, double dX, double dY, double Angle)
{
    ManifestItemSlitClass* m = new ManifestItemSlitClass(Angle, dX, dY);
    m->X = x;
    m->Y = y;
    EventsDataHub->Manifest.append(m);
    emit RequestUpdateGuiForManifest();
}

int ARec_SI::CountManifestItems()
{
    return EventsDataHub->Manifest.size();
}

void ARec_SI::SetManifestItemLineProperties(int i, int color, int width, int style)
{
    if (i<0 || i>EventsDataHub->Manifest.size()-1) return;
    EventsDataHub->Manifest[i]->LineColor = color;
    EventsDataHub->Manifest[i]->LineStyle = style;
    EventsDataHub->Manifest[i]->LineWidth = width;
}

const QVariant ARec_SI::Peaks_GetSignalPerPhE() const
{
    QVariantList vl;
    const int numPMs = TmpHub->ChPerPhEl_Peaks.size();
    for (int i=0; i<numPMs; i++)
        vl.append(TmpHub->ChPerPhEl_Peaks.at(i));
    return vl;
}

#include "acalibratorsignalperphel.h"
void ARec_SI::Peaks_PrepareData()
{
    bool bOK = RManager->Calibrator_Peaks->PrepareData();
    if (!bOK)
        abort("Failed to prepare data for SigPerPhE calibration from peaks");
}

void ARec_SI::Peaks_Configure(int bins, double from, double to, double sigmaPeakfinder, double thresholdPeakfinder, int maxPeaks)
{
    RManager->Calibrator_Peaks->SetNumBins(bins);
    RManager->Calibrator_Peaks->SetRange(from, to);
    RManager->Calibrator_Peaks->SetSigma(sigmaPeakfinder);
    RManager->Calibrator_Peaks->SetThreshold(thresholdPeakfinder);
    RManager->Calibrator_Peaks->SetMaximumPeaks(maxPeaks);
}

double ARec_SI::Peaks_Extract_NoAbortOnFail(int ipm)
{
    const int numPMs = EventsDataHub->getNumPMs();
    if (ipm >=0 && ipm < numPMs)
    {
        bool bOK = RManager->Calibrator_Peaks->Extract(ipm);
        if (bOK) return TmpHub->ChPerPhEl_Peaks.at(ipm);
    }
    return std::numeric_limits<double>::quiet_NaN();
}

void ARec_SI::Peaks_ExtractAll()
{
    const int numPMs = EventsDataHub->getNumPMs();

    bool bThereWereErrors = false;
    for (int ipm=0; ipm<numPMs; ipm++)
    {
         bool bOK = RManager->Calibrator_Peaks->Extract(ipm);
         if (!bOK) bThereWereErrors = true;
    }

    if (bThereWereErrors)
        abort("Failed to extract peaks: " + RManager->Calibrator_Peaks->GetLastError());
}

QVariant ARec_SI::Peaks_GetPeakPositions(int ipm)
{
    const int numPMs = EventsDataHub->getNumPMs();
    QVariantList res;

    if (ipm >=0  &&  ipm < numPMs  &&  TmpHub->FoundPeaks.size() > ipm)
    {
        const QVector<double>& vec = TmpHub->FoundPeaks.at(ipm);
        for (const double& d : vec) res << d;
    }
    return res;
}

const QVariant ARec_SI::GetSignalPerPhE_stat() const
{
    QVariantList vl;
    const int numPMs = TmpHub->ChPerPhEl_Sigma2.size();
    for (int i=0; i<numPMs; i++)
        vl.append(TmpHub->ChPerPhEl_Sigma2.at(i));
    return vl;
}

