#include "apointsourcesimulator.h"
#include "detectorclass.h"
#include "asimulationmanager.h"
#include "eventsdataclass.h"
#include "aoneevent.h"
#include "amaterialparticlecolection.h"
#include "ageoobject.h"
#include "anoderecord.h"
#include "apositionenergyrecords.h"
#include "asandwich.h"
#include "ageneralsimsettings.h"
#include "ajsontools.h"
#include "acommonfunctions.h"
#include "photon_generator.h"
#include "aphotontracer.h"
#include "apmhub.h"
#include "alrfmoduleselector.h"
#include "aphotonsimsettings.h"

#include <QDebug>
#include <QJsonObject>

#include "TH1D.h"
#include "TRandom2.h"
#include "TGeoManager.h"
#include "TGeoNavigator.h"

APointSourceSimulator::APointSourceSimulator(const ASimSettings & SimSet, const DetectorClass &detector, const std::vector<ANodeRecord*> & Nodes, int threadID, int startSeed) :
    ASimulator(SimSet, detector, threadID, startSeed),
    PhotSimSettings(SimSet.photSimSet),
    Nodes(Nodes) {}

APointSourceSimulator::~APointSourceSimulator()
{
    delete CustomHist;
}

int APointSourceSimulator::getEventCount() const
{
    if (PhotSimSettings.GenMode == APhotonSimSettings::Single)
        return eventEnd - eventBegin;
    else
        return (eventEnd - eventBegin) * NumRuns;
}

bool APointSourceSimulator::setup()
{
    ASimulator::setup();

    NumRuns = PhotSimSettings.getActiveRuns();

    bLimitToVolume = PhotSimSettings.bLimitToVol;
    if (bLimitToVolume)
    {
        const QString & Vol = PhotSimSettings.LimitVolume;
        if ( !Vol.isEmpty() && detector.Sandwich->World->findObjectByName(Vol) )
            LimitToVolume = PhotSimSettings.LimitVolume.toLocal8Bit().data();
        else bLimitToVolume = false;
    }

    if (PhotSimSettings.PerNodeSettings.Mode == APhotonSim_PerNodeSettings::Custom)
    {
        delete CustomHist; CustomHist = nullptr;

        const QVector<ADPair> Dist = PhotSimSettings.PerNodeSettings.CustomDist;
        const int size = Dist.size();
        if (size == 0)
        {
            ErrorString = "Config does not contain per-node photon distribution";
            return false;
        }

        double X[size];
        for (int i = 0; i < size; i++) X[i] = Dist.at(i).first;

        CustomHist = new TH1D("", "NumPhotDist", size-1, X);
        for (int i = 1; i < size + 1; i++) CustomHist->SetBinContent(i, Dist.at(i-1).second);
        CustomHist->GetIntegral(); //will be thread safe after this
    }

    const APhotonSim_FixedPhotSettings & FS = PhotSimSettings.FixedPhotSettings;
    Photon.waveIndex = FS.FixWaveIndex;
    if (!GenSimSettings.fWaveResolved) Photon.waveIndex = -1;
    bIsotropic = FS.bIsotropic;
    if (!bIsotropic)
    {
        if (FS.DirectionMode == APhotonSim_FixedPhotSettings::Vector)
        {
            bCone = false;
            Photon.v[0] = FS.FixDX;
            Photon.v[1] = FS.FixDY;
            Photon.v[2] = FS.FixDZ;
            NormalizeVectorSilent(Photon.v);
        }
        else //Cone
        {
            bCone = true;
            double v[3];
            v[0] = FS.FixDX;
            v[1] = FS.FixDY;
            v[2] = FS.FixDZ;
            NormalizeVectorSilent(v);
            ConeDir = TVector3(v[0], v[1], v[2]);
            CosConeAngle = cos(FS.FixConeAngle * TMath::Pi() / 180.0);
        }
    }

    switch (PhotSimSettings.GenMode)
    {
    case APhotonSimSettings::Single :
        TotalEvents = NumRuns; //the only case when we can split runs between threads
        break;
    case APhotonSimSettings::Scan :
    {
        TotalEvents = PhotSimSettings.ScanSettings.countEvents();
        break;
    }
    case APhotonSimSettings::Flood :
        TotalEvents = PhotSimSettings.FloodSettings.Nodes;
        break;
    case APhotonSimSettings::File :
        if (PhotSimSettings.CustomNodeSettings.Mode == APhotonSim_CustomNodeSettings::CustomNodes)
             TotalEvents = Nodes.size();
        else TotalEvents = PhotSimSettings.CustomNodeSettings.NumEventsInFile;
        break;
    case APhotonSimSettings::Script :
        TotalEvents = Nodes.size();
        break;
    default:
        ErrorString = "Unknown or not implemented photon simulation mode";
        return false;
    }
    return true;
}

void APointSourceSimulator::simulate()
{
    assureNavigatorPresent();  // here navigator for this thread is created

    fStopRequested = false;
    fHardAbortWasTriggered = false;

    ReserveSpace(getEventCount());

    switch (PhotSimSettings.GenMode)
    {
    case APhotonSimSettings::Single :
        fSuccess = simulateSingle();
        break;
    case APhotonSimSettings::Scan :
        fSuccess = simulateRegularGrid();
        break;
    case APhotonSimSettings::Flood :
        fSuccess = simulateFlood();
        break;
    case APhotonSimSettings::File :
    {
        if (PhotSimSettings.CustomNodeSettings.Mode == APhotonSim_CustomNodeSettings::CustomNodes)
             fSuccess = simulateCustomNodes();
        else
             fSuccess = simulatePhotonsFromFile();
        break;
    }
    case APhotonSimSettings::Script :
        fSuccess = simulateCustomNodes();
        break;
    default:
        fSuccess = false;
        break;
    }

    if (fHardAbortWasTriggered) fSuccess = false;
}

void APointSourceSimulator::appendToDataHub(EventsDataClass *dataHub)
{
    //qDebug() << "Thread #"<<ID << " PhotSim ---> appending data";
    ASimulator::appendToDataHub(dataHub);
    dataHub->ScanNumberOfRuns = this->NumRuns;
}

bool APointSourceSimulator::simulateSingle()
{
    const APhotonSim_SingleSettings & SingSet = PhotSimSettings.SingleSettings;
    std::unique_ptr<ANodeRecord> node(ANodeRecord::createS(SingSet.X, SingSet.Y, SingSet.Z));

    int eventsToDo = eventEnd - eventBegin; //runs are split between the threads, since node position is fixed for all
    double updateFactor = (eventsToDo < 1) ? 100.0 : 100.0/eventsToDo;
    progress = 0;
    eventCurrent = 0;
    for (int irun = 0; irun < eventsToDo; ++irun)
    {
        simulateOneNode(*node);
        eventCurrent = irun;
        progress = irun * updateFactor;
        if (fStopRequested) return false;
    }
    return true;
}

bool APointSourceSimulator::simulateRegularGrid()
{
    const APhotonSim_ScanSettings & ScanSet = PhotSimSettings.ScanSettings;

    //extracting grid parameters
    double RegGridOrigin[3]; //grid origin
    RegGridOrigin[0] = ScanSet.X0;
    RegGridOrigin[1] = ScanSet.Y0;
    RegGridOrigin[2] = ScanSet.Z0;
    //
    double RegGridStep[3][3]; //vector [axis] [step]
    int RegGridNodes[3]; //number of nodes along the 3 axes
    bool RegGridFlagPositive[3]; //Axes option
    //
    for (int ic = 0; ic < 3; ic++)
    {
        const APhScanRecord & rec = ScanSet.ScanRecords.at(ic);
        if (rec.bEnabled)
        {
            RegGridStep[ic][0] = rec.DX;
            RegGridStep[ic][1] = rec.DY;
            RegGridStep[ic][2] = rec.DZ;
            RegGridNodes[ic]   = rec.Nodes;
            RegGridFlagPositive[ic] = rec.bBiDirect;
        }
        else
        {
            RegGridStep[ic][0] = 0;
            RegGridStep[ic][1] = 0;
            RegGridStep[ic][2] = 0;
            RegGridNodes[ic] = 1; //1 is disabled axis
            RegGridFlagPositive[ic] = true;
        }
    }

    std::unique_ptr<ANodeRecord> node(ANodeRecord::createS(0, 0, 0));
    int currentNode = 0;
    eventCurrent = 0;
    double updateFactor = 100.0 / ( NumRuns * (eventEnd - eventBegin) );
    //Do the scan
    int iAxis[3];
    for (iAxis[0]=0; iAxis[0]<RegGridNodes[0]; iAxis[0]++)
        for (iAxis[1]=0; iAxis[1]<RegGridNodes[1]; iAxis[1]++)
            for (iAxis[2]=0; iAxis[2]<RegGridNodes[2]; iAxis[2]++)  //iAxis - counters along the axes!!!
            {
                if (currentNode < eventBegin)
                { //this node is taken care of by another thread
                    currentNode++;
                    continue;
                }

                //calculating node coordinates
                for (int i=0; i<3; i++) node->R[i] = RegGridOrigin[i];
                //shift from the origin
                for (int axis=0; axis<3; axis++)
                { //going axis by axis
                    double ioffset = 0;
                    if (!RegGridFlagPositive[axis]) ioffset = -0.5*( RegGridNodes[axis] - 1 );
                    for (int i=0; i<3; i++) node->R[i] += (ioffset + iAxis[axis]) * RegGridStep[axis][i];
                }

                //running this node
                for (int irun = 0; irun < NumRuns; irun++)
                {
                    simulateOneNode(*node);
                    eventCurrent++;
                    progress = eventCurrent * updateFactor;
                    if (fStopRequested) return false;
                }
                currentNode++;
                if (currentNode >= eventEnd) return true;
            }
    return true;
}

bool APointSourceSimulator::simulateFlood()
{
    const APhotonSim_FloodSettings & FloodSet = PhotSimSettings.FloodSettings;

    //extracting flood parameters
    double Xfrom, Xto, Yfrom, Yto, CenterX, CenterY, RadiusIn, RadiusOut;
    double Rad2in, Rad2out;
    if (FloodSet.Shape == APhotonSim_FloodSettings::Rectangular)
    {
        Xfrom = FloodSet.Xfrom;
        Xto =   FloodSet.Xto;
        Yfrom = FloodSet.Yfrom;
        Yto =   FloodSet.Yto;
    }
    else
    {
        CenterX = FloodSet.X0;
        CenterY = FloodSet.Y0;
        RadiusIn = 0.5 * FloodSet.InnerD;
        RadiusOut = 0.5 * FloodSet.OuterD;

        Rad2in  = RadiusIn  * RadiusIn;
        Rad2out = RadiusOut * RadiusOut;
        Xfrom   = CenterX - RadiusOut;
        Xto     = CenterX + RadiusOut;
        Yfrom   = CenterY - RadiusOut;
        Yto     = CenterY + RadiusOut;
    }
    double Zfixed, Zfrom, Zto;
    if (FloodSet.ZMode == APhotonSim_FloodSettings::Fixed)
    {
        Zfixed = FloodSet.Zfixed;
    }
    else
    {
        Zfrom = FloodSet.Zfrom;
        Zto =   FloodSet.Zto;
    }

    //Do flood
    std::unique_ptr<ANodeRecord> node(ANodeRecord::createS(0, 0, 0));
    int nodeCount = (eventEnd - eventBegin);
    eventCurrent = 0;
    int WatchdogThreshold = 100000;
    double updateFactor = 100.0 / (NumRuns*nodeCount);
    for (int inode = 0; inode < nodeCount; inode++)
    {
        if(fStopRequested) return false;

        //choosing node coordinates
        node->R[0] = Xfrom + (Xto - Xfrom) * RandGen->Rndm();
        node->R[1] = Yfrom + (Yto - Yfrom) * RandGen->Rndm();

        //running this node
        if (FloodSet.Shape == APhotonSim_FloodSettings::Ring)
        {
            double r2  = (node->R[0] - CenterX)*(node->R[0] - CenterX) + (node->R[1] - CenterY)*(node->R[1] - CenterY);
            if ( r2 > Rad2out || r2 < Rad2in )
            {
                inode--;
                continue;
            }
        }

        if (FloodSet.ZMode == APhotonSim_FloodSettings::Fixed)
            node->R[2] = Zfixed;
        else
            node->R[2] = Zfrom + (Zto - Zfrom) * RandGen->Rndm();

        if (bLimitToVolume && !isInsideLimitingObject(node->R))
        {
            WatchdogThreshold--;
            if (WatchdogThreshold < 0 && inode == 0)
            {
                ErrorString = "100000 attempts to generate a point inside the limiting object has failed!";
                return false;
            }

            inode--;
            continue;
        }

        for (int irun = 0; irun < NumRuns; irun++)
        {
            simulateOneNode(*node);
            eventCurrent++;
            progress = eventCurrent * updateFactor;
            if (fStopRequested) return false;
        }
    }
    return true;
}

bool APointSourceSimulator::simulateCustomNodes()
{
    int nodeCount = (eventEnd - eventBegin);
    int currentNode = eventBegin;
    eventCurrent = 0;
    double updateFactor = 100.0 / ( NumRuns * nodeCount );

    for (int inode = 0; inode < nodeCount; inode++)
    {
        ANodeRecord * thisNode = Nodes.at(currentNode);

        for (int irun = 0; irun<NumRuns; irun++)
        {
            simulateOneNode(*thisNode);
            eventCurrent++;
            progress = eventCurrent * updateFactor;
            if(fStopRequested) return false;
        }
        currentNode++;
    }
    return true;
}

bool APointSourceSimulator::simulatePhotonsFromFile()
{
    eventCurrent = 0;
    double updateFactor = 100.0 / (eventEnd - eventBegin);

    const QString FileName = PhotSimSettings.CustomNodeSettings.FileName;
    QFile file(FileName);
    if (!file.open(QIODevice::ReadOnly | QFile::Text))
    {
        ErrorString = "Cannot open file " + FileName;
        return false;
    }
    QTextStream in(&file);

    OneEvent->clearHits();

    for (int iEvent = -1; iEvent < eventEnd; iEvent++)
    {
        while (!in.atEnd())
        {
            if (fStopRequested)
            {
                in.flush();
                file.close();
                return false;
            }

            QString line = in.readLine().simplified();
            if (line.startsWith('#')) break; //next event in the next line of the file

            if (iEvent < eventBegin) continue;

            QStringList sl = line.split(' ', QString::SkipEmptyParts);
            if (sl.size() < 8)
            {
                ErrorString = "Format error in the file with photon records";
                file.close();
                return false;
            }

            APhoton p;
            p.r[0]      = sl.at(0).toDouble();
            p.r[1]      = sl.at(1).toDouble();
            p.r[2]      = sl.at(2).toDouble();
            p.v[0]      = sl.at(3).toDouble();
            p.v[1]      = sl.at(4).toDouble();
            p.v[2]      = sl.at(5).toDouble();
            p.waveIndex = sl.at(6).toInt();
            p.time      = sl.at(7).toDouble();

            p.scint_type = 0;
            p.SimStat    = OneEvent->SimStat;

            if (p.waveIndex < -1 || p.waveIndex >= GenSimSettings.WaveNodes) p.waveIndex = -1;

            photonTracker->TracePhoton(&p);
        }

        if (iEvent < eventBegin) continue;

        OneEvent->HitsToSignal();
        dataHub->Events.append(OneEvent->PMsignals);
        if (GenSimSettings.fTimeResolved)
            dataHub->TimedEvents.append(OneEvent->TimedPMsignals);

        AScanRecord * sr = new AScanRecord();
        sr->Points.Reinitialize(0);
        sr->ScintType = 0;
        dataHub->Scan.append(sr);

        OneEvent->clearHits();
        progress = eventCurrent * updateFactor;
        eventCurrent++;
    }
    return true;
}

int APointSourceSimulator::getNumPhotToRun()
{
    int num = 0;

    switch (PhotSimSettings.PerNodeSettings.Mode)
    {
    case APhotonSim_PerNodeSettings::Constant :
        num = PhotSimSettings.PerNodeSettings.Number;
        break;
    case APhotonSim_PerNodeSettings::Uniform :
        num = RandGen->Rndm() * (PhotSimSettings.PerNodeSettings.Max - PhotSimSettings.PerNodeSettings.Min + 1) + PhotSimSettings.PerNodeSettings.Min;
        break;
    case APhotonSim_PerNodeSettings::Gauss :
        num = std::round( RandGen->Gaus(PhotSimSettings.PerNodeSettings.Mean, PhotSimSettings.PerNodeSettings.Sigma) );
        break;
    case APhotonSim_PerNodeSettings::Custom :
        num = CustomHist->GetRandom();
        break;
    case APhotonSim_PerNodeSettings::Poisson :
        num = RandGen->Poisson(PhotSimSettings.PerNodeSettings.PoisMean);
        break;
    }

    if (num < 0) num = 0;
    return num;
}

void APointSourceSimulator::generateAndTracePhotons(AScanRecord *scs, double time0, int iPoint)
{
    TGeoNavigator * navigator = detector.GeoManager->GetCurrentNavigator();
    if (!navigator)
    {
        qWarning() << "UNEXPECTED - report this issue please: Navigator not found, adding new one";
        detector.GeoManager->AddNavigator();
    }

    //if secondary scintillation, finding the secscint boundaries
    double z1, z2, timeOfDrift, driftSpeedSecScint;
    if (scs->ScintType == 2)
    {
        if (!findSecScintBounds(scs->Points[iPoint].r, z1, z2, timeOfDrift, driftSpeedSecScint))
        {
            scs->zStop = scs->Points[iPoint].r[2];
            return; //no sec_scintillator for these XY, so no photon generation
        }
    }

    Photon.r[0] = scs->Points[iPoint].r[0];
    Photon.r[1] = scs->Points[iPoint].r[1];
    Photon.r[2] = scs->Points[iPoint].r[2];
    Photon.scint_type = scs->ScintType;
    Photon.time = time0;

    for (int i=0; i<scs->Points[iPoint].energy; i++)
    {
        //photon direction
        if (bIsotropic) photonGenerator->GenerateDirection(&Photon);
        else if (bCone)
        {
            double z = CosConeAngle + RandGen->Rndm() * (1.0 - CosConeAngle);
            double tmp = sqrt(1.0 - z*z);
            double phi = RandGen->Rndm()*3.1415926535*2.0;
            TVector3 K1(tmp*cos(phi), tmp*sin(phi), z);
            TVector3 Coll(ConeDir);
            K1.RotateUz(Coll);
            Photon.v[0] = K1[0];
            Photon.v[1] = K1[1];
            Photon.v[2] = K1[2];
        }
        //else it is already set

        Photon.time = time0;

        int thisMatIndex;
        TGeoNode* node = navigator->FindNode(Photon.r[0], Photon.r[1], Photon.r[2]);
        if (node) thisMatIndex = node->GetVolume()->GetMaterial()->GetIndex();
        else
        {
            thisMatIndex = detector.top->GetMaterial()->GetIndex(); //get material of the world
            qWarning() << "Node not found when generating photons, using material of the world";
        }

        if (!PhotSimSettings.FixedPhotSettings.bFixWave)
            photonGenerator->GenerateWave(&Photon, thisMatIndex);//if directly given wavelength -> waveindex is already set in PhotonOnStart

        photonGenerator->GenerateTime(&Photon, thisMatIndex);

        if (scs->ScintType == 2)
        {
            const double dist = (z2 - z1) * RandGen->Rndm();
            Photon.r[2] = z1 + dist;
            Photon.time = time0 + timeOfDrift;
            if (driftSpeedSecScint != 0)
                Photon.time += dist / driftSpeedSecScint;
        }

        Photon.SimStat = OneEvent->SimStat;

        if (PhotSimSettings.SpatialDistSettings.bEnabled) applySpatialDist(scs->Points[iPoint].r, Photon);

        photonTracker->TracePhoton(&Photon);
    }

    if (scs->ScintType == 2)
    {
        scs->Points[iPoint].r[2] = z1;
        scs->zStop = z2;
    }
}

void APointSourceSimulator::applySpatialDist(double * center, APhoton & photon) const
{
    //qDebug() << center[0] <<center[1] <<center[2];
    const QVector<A3DPosProb> & Matrix = PhotSimSettings.SpatialDistSettings.Matrix;
    const int size = Matrix.size();

    const double rnd = RandGen->Rndm();
    double val = 0;
    int iSelectedCell = 0;
    for (; iSelectedCell < size; iSelectedCell++)
    {
        val += Matrix.at(iSelectedCell).Probability;
        if (rnd < val) break;
    }
    qDebug() << "Found" << iSelectedCell << " of " << size;

    for (int i = 0; i < 3; i++)
        photon.r[i] = center[i] + Matrix.at(iSelectedCell).R[i];
}

bool APointSourceSimulator::findSecScintBounds(double *r, double & z1, double & z2, double & timeOfDrift, double & driftSpeedInSecScint)
{
    TGeoNavigator *navigator = detector.GeoManager->GetCurrentNavigator();
    if (!navigator)
    {
        qWarning() << "UNEXPECTED - report this issue please! Navigator not found, adding new one";
        detector.GeoManager->AddNavigator();
    }

    navigator->SetCurrentPoint(r);
    navigator->SetCurrentDirection(0, 0, 1.0);
    TGeoNode * node = navigator->FindNode();
    if (!node) return false;

    timeOfDrift = 0;
    double iMat = navigator->GetCurrentVolume()->GetMaterial()->GetIndex();
    //going up until exit geometry or find SecScint
    do
    {
        navigator->FindNextBoundaryAndStep();
        if (navigator->IsOutside()) return false;

        double DriftVelocity = detector.MpCollection->getDriftSpeed(iMat); // still previous iMat
        iMat = navigator->GetCurrentVolume()->GetMaterial()->GetIndex();    // next iMat
        if (DriftVelocity != 0)
        {
            double Step = navigator->GetStep();
            timeOfDrift += Step / DriftVelocity;
        }
    }
    while (navigator->GetCurrentVolume()->GetName() != SecScintName);

    driftSpeedInSecScint = detector.MpCollection->getDriftSpeed(iMat);
    z1 = navigator->GetCurrentPoint()[2];
    navigator->FindNextBoundary(); //does not step - we are still inside SecScint!
    double SecScintWidth = navigator->GetStep();
    z2 = z1 + SecScintWidth;
    return true;
}

void APointSourceSimulator::simulateOneNode(const ANodeRecord & node)
{
    const ANodeRecord * thisNode = &node;
    std::unique_ptr<ANodeRecord> outNode(ANodeRecord::createS(1e10, 1e10, 1e10)); // if outside will use this instead of thisNode

    const int numPoints = 1 + thisNode->getNumberOfLinkedNodes();
    OneEvent->clearHits();
    AScanRecord * sr = new AScanRecord();
    sr->Points.Reinitialize(numPoints);
    sr->ScintType = ( PhotSimSettings.ScintType == APhotonSimSettings::Primary ? 1 : 2 );

    for (int iPoint = 0; iPoint < numPoints; iPoint++)
    {
        const bool bInside = !(bLimitToVolume && !isInsideLimitingObject(thisNode->R));
        if (bInside)
        {
            for (int i=0; i<3; i++) sr->Points[iPoint].r[i] = thisNode->R[i];
            sr->Points[iPoint].energy = (thisNode->NumPhot == -1 ? getNumPhotToRun() : thisNode->NumPhot);
        }
        else
        {
            for (int i=0; i<3; i++) sr->Points[iPoint].r[i] =  outNode->R[i];
            sr->Points[iPoint].energy = 0;
        }

        if (!GenSimSettings.fLRFsim)
        {
            generateAndTracePhotons(sr, thisNode->Time, iPoint);
        }
        else
        {
            if (bInside)
            {
                // NumPhotElPerLrfUnity - scaling: LRF of 1.0 corresponds to NumPhotElPerLrfUnity photo-electrons
                int numPhots = sr->Points[iPoint].energy;  // photons (before scaling) to generate at this node
                double energy = 1.0 * numPhots / GenSimSettings.NumPhotsForLrfUnity; // NumPhotsForLRFunity corresponds to the total number of photons per event for unitary LRF

                //Generating events
                for (int ipm = 0; ipm < detector.PMs->count(); ipm++)
                {
                    double avSignal = detector.LRFs->getLRF(ipm, thisNode->R) * energy;
                    double avPhotEl = avSignal * GenSimSettings.NumPhotElPerLrfUnity;
                    double numPhotEl = RandGen->Poisson(avPhotEl);

                    double signal = numPhotEl / GenSimSettings.NumPhotElPerLrfUnity;  // back to LRF units
                    OneEvent->PMsignals[ipm] += signal;
                }
            }
            //if outside nothing to do
        }

        //if exists, continue to work with the linked node(s)
        thisNode = thisNode->getLinkedNode();
        if (!thisNode) break; //paranoic
    }

    if (!GenSimSettings.fLRFsim) OneEvent->HitsToSignal();

    dataHub->Events.append(OneEvent->PMsignals);
    if (GenSimSettings.fTimeResolved)
        dataHub->TimedEvents.append(OneEvent->TimedPMsignals);  //LRF sim for time-resolved will give all zeroes!

    dataHub->Scan.append(sr);
}

bool APointSourceSimulator::isInsideLimitingObject(const double * r)
{
    TGeoNavigator * navigator = detector.GeoManager->GetCurrentNavigator();
    if (!navigator)
    {
        qWarning() << "UNEXPECTED - report this issue please!! Navigator not found, adding new one";
        detector.GeoManager->AddNavigator();
    }

    TGeoNode* node = navigator->FindNode(r[0], r[1], r[2]);
    if (!node) return false;
    return (node->GetVolume() && node->GetVolume()->GetName() == LimitToVolume);
}
