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
#include "generalsimsettings.h"
#include "ajsontools.h"
#include "acommonfunctions.h"
#include "photon_generator.h"
#include "aphotontracer.h"
#include "apmhub.h"
#include "alrfmoduleselector.h"

#include <QDebug>
#include <QJsonObject>

#include "TH1I.h" //to change
#include "TRandom2.h"
#include "TGeoManager.h"
#include "TGeoNavigator.h"

APointSourceSimulator::APointSourceSimulator(ASimulationManager & simMan, int ID) :
    ASimulator(simMan, ID) {}

APointSourceSimulator::~APointSourceSimulator()
{
    delete CustomHist;
}

int APointSourceSimulator::getEventCount() const
{
    if (PointSimMode == 0) return eventEnd - eventBegin;
    else return (eventEnd - eventBegin)*NumRuns;
}

bool APointSourceSimulator::setup(QJsonObject &json)
{
    if (!json.contains("PointSourcesConfig"))
    {
        ErrorString = "Json sent to simulator does not contain generation config data!";
        return false;
    }
    if(!ASimulator::setup(json)) return false;

    QJsonObject js = json["PointSourcesConfig"].toObject();
    //reading main control options
    QJsonObject cjson = js["ControlOptions"].toObject();
    PointSimMode = cjson["Single_Scan_Flood"].toInt();
    ScintType = 1 + cjson["Primary_Secondary"].toInt(); //0 - primary(1), 1 - secondary(2)
    NumRuns = cjson["MultipleRunsNumber"].toInt();
    if (!cjson["MultipleRuns"].toBool()) NumRuns = 1;

    fLimitNodesToObject = false;
    if (cjson.contains("GenerateOnlyInPrimary"))  //just in case it is an old config file run directly
    {
        if (detector.Sandwich->World->findObjectByName("PrScint"))
        {
            fLimitNodesToObject = true;
            LimitNodesToObject = "PrScint";
        }
    }
    if (cjson.contains("LimitNodesTo"))
    {
        if (cjson.contains("LimitNodes")) //new system
            fLimitNodesToObject = cjson["LimitNodes"].toBool();
        else
            fLimitNodesToObject = true;  //semi-old
        QString Obj = cjson["LimitNodesTo"].toString();

        if (fLimitNodesToObject && !Obj.isEmpty() && detector.Sandwich->World->findObjectByName(Obj))
        {
            fLimitNodesToObject = true;
            LimitNodesToObject = Obj.toLocal8Bit().data();
        }
    }

    //reading photons per node info
    QJsonObject ppnjson = js["PhotPerNodeOptions"].toObject();
    numPhotMode = ppnjson["PhotPerNodeMode"].toInt();
    if (numPhotMode < 0 || numPhotMode > 3)
    {
        ErrorString = "Unknown photons per node mode!";
        return false;
    }
    //numPhotsConst = ppnjson["PhotPerNodeConstant"].toInt();
    parseJson(ppnjson, "PhotPerNodeConstant", numPhotsConst);
    //numPhotUniMin = ppnjson["PhotPerNodeUniMin"].toInt();
    parseJson(ppnjson, "PhotPerNodeUniMin", numPhotUniMin);
    //numPhotUniMax = ppnjson["PhotPerNodeUniMax"].toInt();
    parseJson(ppnjson, "PhotPerNodeUniMax", numPhotUniMax);
    parseJson(ppnjson, "PhotPerNodeGaussMean", numPhotGaussMean);
    parseJson(ppnjson, "PhotPerNodeGaussSigma", numPhotGaussSigma);

    if (numPhotMode == 3)
    {
        if (!ppnjson.contains("PhotPerNodeCustom"))
        {
            ErrorString = "Generation config does not contain custom photon distribution data!";
            return false;
        }
        QJsonArray ja = ppnjson["PhotPerNodeCustom"].toArray();
        int size = ja.size();
        double* xx = new double[size];
        int* yy    = new int[size];
        for (int i=0; i<size; i++)
        {
            xx[i] = ja[i].toArray()[0].toDouble();
            yy[i] = ja[i].toArray()[1].toInt();
        }
        TString hName = "hPhotDistr";
        hName += ID;
        CustomHist = new TH1I(hName, "Photon distribution", size-1, xx);
        for (int i = 1; i<size+1; i++)  CustomHist->SetBinContent(i, yy[i-1]);
        CustomHist->GetIntegral(); //will be thread safe after this
        delete[] xx;
        delete[] yy;
    }

    //reading wavelength/decay options
    QJsonObject wdjson = js["WaveTimeOptions"].toObject();
    fUseGivenWaveIndex = false; //compatibility
    parseJson(wdjson, "UseFixedWavelength", fUseGivenWaveIndex);
    PhotonOnStart.waveIndex = wdjson["WaveIndex"].toInt();
    if (!simSettings.fWaveResolved) PhotonOnStart.waveIndex = -1;

    //reading direction info
    QJsonObject pdjson = js["PhotonDirectionOptions"].toObject();
    fRandomDirection =  pdjson["Random"].toBool();
    if (!fRandomDirection)
    {
        PhotonOnStart.v[0] = pdjson["FixedX"].toDouble();
        PhotonOnStart.v[1] = pdjson["FixedY"].toDouble();
        PhotonOnStart.v[2] = pdjson["FixedZ"].toDouble();
        NormalizeVector(PhotonOnStart.v);
        //qDebug() << "  ph dir:"<<PhotonOnStart.v[0]<<PhotonOnStart.v[1]<<PhotonOnStart.v[2];
    }
    fCone = false;
    if (pdjson.contains("Fixed_or_Cone"))
    {
        int i = pdjson["Fixed_or_Cone"].toInt();
        fCone = (i==1);
        if (fCone)
        {
            double v[3];
            v[0] = pdjson["FixedX"].toDouble();
            v[1] = pdjson["FixedY"].toDouble();
            v[2] = pdjson["FixedZ"].toDouble();
            NormalizeVector(v);
            ConeDir = TVector3(v[0], v[1], v[2]);
            double ConeAngle = pdjson["Cone"].toDouble()*3.1415926535/180.0;
            CosConeAngle = cos(ConeAngle);
        }
    }

    //calculating eventCount and storing settings specific to each simulation mode
    switch (PointSimMode)
    {
    case 0:
        //fOK = SimulateSingle(json);
        simOptions = js["SinglePositionOptions"].toObject();
        totalEventCount = NumRuns; //the only case when we can split runs between threads
        break;
    case 1:
    {
        //fOK = SimulateRegularGrid(json);
        simOptions = js["RegularScanOptions"].toObject();
        int RegGridNodes[3]; //number of nodes along the 3 axes
        QJsonArray rsdataArr = simOptions["AxesData"].toArray();
        for (int ic=0; ic<3; ic++)
        {
            if (ic >= rsdataArr.size())
                RegGridNodes[ic] = 1; //disabled axis
            else
                RegGridNodes[ic] = rsdataArr[ic].toObject()["Nodes"].toInt();
        }
        totalEventCount = 1;
        for (int i=0; i<3; i++) totalEventCount *= RegGridNodes[i]; //progress reporting knows we do NumRuns per each node
        break;
    }
    case 2:
        //fOK = SimulateFlood(json);
        simOptions = js["FloodOptions"].toObject();
        totalEventCount = simOptions["Nodes"].toInt();//progress reporting knows we do NumRuns per each node
        break;
    case 3:
        simOptions = js["CustomNodesOptions"].toObject();
        //totalEventCount = simOptions["Nodes"].toArray().size();//progress reporting knows we do NumRuns per each node
        totalEventCount = simMan.Nodes.size();
        break;
    case 4:
        totalEventCount = simMan.Nodes.size();
        break;
    default:
        ErrorString = "Unknown or not implemented simulation mode: "+QString().number(PointSimMode);
        return false;
    }
    return true;
}

void APointSourceSimulator::simulate()
{
    checkNavigatorPresent();

    fStopRequested = false;
    fHardAbortWasTriggered = false;

    ReserveSpace(getEventCount());
    switch (PointSimMode)
    {
    case 0: fSuccess = SimulateSingle(); break;
    case 1: fSuccess = SimulateRegularGrid(); break;
    case 2: fSuccess = SimulateFlood(); break;
    case 3: fSuccess = SimulateCustomNodes(); break;
    case 4: fSuccess = SimulateCustomNodes(); break;
    default: fSuccess = false; break;
    }
    if (fHardAbortWasTriggered) fSuccess = false;
}

void APointSourceSimulator::appendToDataHub(EventsDataClass *dataHub)
{
    //qDebug() << "Thread #"<<ID << " PhotSim ---> appending data";
    ASimulator::appendToDataHub(dataHub);
    dataHub->ScanNumberOfRuns = this->NumRuns;
}

bool APointSourceSimulator::SimulateSingle()
{
    double x = simOptions["SingleX"].toDouble();
    double y = simOptions["SingleY"].toDouble();
    double z = simOptions["SingleZ"].toDouble();
    std::unique_ptr<ANodeRecord> node(ANodeRecord::createS(x, y, z));

    int eventsToDo = eventEnd - eventBegin; //runs are split between the threads, since node position is fixed for all
    double updateFactor = (eventsToDo<1) ? 100.0 : 100.0/eventsToDo;
    progress = 0;
    eventCurrent = 0;
    for (int irun = 0; irun<eventsToDo; ++irun)
    {
        OneNode(*node);
        eventCurrent = irun;
        progress = irun * updateFactor;
        if (fStopRequested) return false;
    }
    return true;
}

bool APointSourceSimulator::SimulateRegularGrid()
{
    //extracting grid parameters
    double RegGridOrigin[3]; //grid origin
    RegGridOrigin[0] = simOptions["ScanX0"].toDouble();
    RegGridOrigin[1] = simOptions["ScanY0"].toDouble();
    RegGridOrigin[2] = simOptions["ScanZ0"].toDouble();
    //
    double RegGridStep[3][3]; //vector [axis] [step]
    int RegGridNodes[3]; //number of nodes along the 3 axes
    bool RegGridFlagPositive[3]; //Axes option
    //
    QJsonArray rsdataArr = simOptions["AxesData"].toArray();
    for (int ic=0; ic<3; ic++)
    {
        if(ic < rsdataArr.count())
        {
            QJsonObject adjson = rsdataArr[ic].toObject();
            RegGridStep[ic][0] = adjson["dX"].toDouble();
            RegGridStep[ic][1] = adjson["dY"].toDouble();
            RegGridStep[ic][2] = adjson["dZ"].toDouble();
            RegGridNodes[ic] = adjson["Nodes"].toInt(); //1 is disabled axis
            RegGridFlagPositive[ic] = adjson["Option"].toInt();
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
    double updateFactor = 100.0/( NumRuns*(eventEnd - eventBegin) );
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
                for (int irun = 0; irun<NumRuns; irun++)
                {
                    OneNode(*node);
                    eventCurrent++;
                    progress = eventCurrent * updateFactor;
                    if(fStopRequested) return false;
                }
                currentNode++;
                if(currentNode >= eventEnd) return true;
            }
    return true;
}

bool APointSourceSimulator::SimulateFlood()
{
    //extracting flood parameters
    int Shape = simOptions["Shape"].toInt(); //0-square, 1-ring/circle
    double Xfrom, Xto, Yfrom, Yto, CenterX, CenterY, RadiusIn, RadiusOut;
    double Rad2in, Rad2out;
    if (Shape == 0)
    {
        Xfrom = simOptions["Xfrom"].toDouble();
        Xto =   simOptions["Xto"].toDouble();
        Yfrom = simOptions["Yfrom"].toDouble();
        Yto =   simOptions["Yto"].toDouble();
    }
    else
    {
        CenterX = simOptions["CenterX"].toDouble();
        CenterY = simOptions["CenterY"].toDouble();
        RadiusIn = 0.5*simOptions["DiameterIn"].toDouble();
        RadiusOut = 0.5*simOptions["DiameterOut"].toDouble();

        Rad2in = RadiusIn*RadiusIn;
        Rad2out = RadiusOut*RadiusOut;
        Xfrom = CenterX - RadiusOut;
        Xto =   CenterX + RadiusOut;
        Yfrom = CenterY - RadiusOut;
        Yto =   CenterY + RadiusOut;
    }
    int Zoption = simOptions["Zoption"].toInt(); //0-fixed, 1-range
    double Zfixed, Zfrom, Zto;
    if (Zoption == 0)
    {
        Zfixed = simOptions["Zfixed"].toDouble();
    }
    else
    {
        Zfrom = simOptions["Zfrom"].toDouble();
        Zto =   simOptions["Zto"].toDouble();
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
        //double r[3];
        node->R[0] = Xfrom + (Xto-Xfrom)*RandGen->Rndm();
        node->R[1] = Yfrom + (Yto-Yfrom)*RandGen->Rndm();
        //running this node
        if (Shape == 1)
        {
            double r2  = (node->R[0] - CenterX)*(node->R[0] - CenterX) + (node->R[1] - CenterY)*(node->R[1] - CenterY);
            if ( r2 > Rad2out || r2 < Rad2in )
            {
                inode--;
                continue;
            }
        }
        if (Zoption == 0) node->R[2] = Zfixed;
        else node->R[2] = Zfrom + (Zto-Zfrom)*RandGen->Rndm();
        if (fLimitNodesToObject && !isInsideLimitingObject(node->R))
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

        for (int irun = 0; irun<NumRuns; irun++)
        {
            OneNode(*node);
            eventCurrent++;
            progress = eventCurrent * updateFactor;
            if(fStopRequested) return false;
        }
    }
    return true;
}

bool APointSourceSimulator::SimulateCustomNodes()
{
    int nodeCount = (eventEnd - eventBegin);
    int currentNode = eventBegin;
    eventCurrent = 0;
    double updateFactor = 100.0 / ( NumRuns*nodeCount );

    for (int inode = 0; inode < nodeCount; inode++)
    {
        ANodeRecord * thisNode = simMan.Nodes.at(currentNode);

        for (int irun = 0; irun<NumRuns; irun++)
        {
            OneNode(*thisNode);
            eventCurrent++;
            progress = eventCurrent * updateFactor;
            if(fStopRequested) return false;
        }
        currentNode++;
    }
    return true;
}

int APointSourceSimulator::PhotonsToRun()
{
    int numPhotons = 0; //protection
    switch (numPhotMode)
    {
    case 0: //constant
        numPhotons = numPhotsConst;
        break;
    case 1:  //uniform
        numPhotons = RandGen->Rndm()*(numPhotUniMax-numPhotUniMin+1) + numPhotUniMin;
        break;
    case 2: //gauss
        numPhotons = RandGen->Gaus(numPhotGaussMean, numPhotGaussSigma);
        break;
    case 3: //custom
        numPhotons = CustomHist->GetRandom();
        break;
    }

    return numPhotons;
}

void APointSourceSimulator::GenerateTraceNphotons(AScanRecord *scs, double time0, int iPoint)
//scs contains coordinates and number of photons to run
//iPoint - number of this scintillation center (can be not zero when e.g. double events are simulated)
{
    //if secondary scintillation, finding the secscint boundaries
    double z1, z2, timeOfDrift, driftSpeedSecScint;
    if (scs->ScintType == 2)
    {
        if (!FindSecScintBounds(scs->Points[iPoint].r, z1, z2, timeOfDrift, driftSpeedSecScint))
        {
            scs->zStop = scs->Points[iPoint].r[2];
            return; //no sec_scintillator for these XY, so no photon generation
        }
    }

    PhotonOnStart.r[0] = scs->Points[iPoint].r[0];
    PhotonOnStart.r[1] = scs->Points[iPoint].r[1];
    PhotonOnStart.r[2] = scs->Points[iPoint].r[2];
    PhotonOnStart.scint_type = scs->ScintType;
    PhotonOnStart.time = time0;

    //================================
    for (int i=0; i<scs->Points[iPoint].energy; i++)
    {
        //photon direction
        if (fRandomDirection)
        {
            if (scs->ScintType == 2)
            {
                photonGenerator->GenerateDirectionSecondary(&PhotonOnStart);
                if (PhotonOnStart.fSkipThisPhoton)
                {
                    PhotonOnStart.fSkipThisPhoton = false;
                    continue;  //skip this photon - direction didn't pass the criterium set for the secondary scintillation
                }
            }
            else photonGenerator->GenerateDirectionPrimary(&PhotonOnStart);
        }
        else if (fCone)
        {
            double z = CosConeAngle + RandGen->Rndm() * (1.0 - CosConeAngle);
            double tmp = sqrt(1.0 - z*z);
            double phi = RandGen->Rndm()*3.1415926535*2.0;
            TVector3 K1(tmp*cos(phi), tmp*sin(phi), z);
            TVector3 Coll(ConeDir);
            K1.RotateUz(Coll);
            PhotonOnStart.v[0] = K1[0];
            PhotonOnStart.v[1] = K1[1];
            PhotonOnStart.v[2] = K1[2];

        }
        //else it is already set

        //configure  wavelength index and emission time
        PhotonOnStart.time = time0;   //reset time offset
        TGeoNavigator *navigator = detector.GeoManager->GetCurrentNavigator();

        int thisMatIndex;
        TGeoNode* node = navigator->FindNode(PhotonOnStart.r[0], PhotonOnStart.r[1], PhotonOnStart.r[2]);
        if (!node)
        {
            //PhotonOnStart.waveIndex = -1;
            thisMatIndex = detector.top->GetMaterial()->GetIndex(); //get material of the world
            //qWarning() << "Node not found when generating photons (photon sources) - assuming material of the world!"<<thisMatIndex;
        }
        else
            thisMatIndex = node->GetVolume()->GetMaterial()->GetIndex();

        if (!fUseGivenWaveIndex) photonGenerator->GenerateWave(&PhotonOnStart, thisMatIndex);//if directly given wavelength -> waveindex is already set in PhotonOnStart
        photonGenerator->GenerateTime(&PhotonOnStart, thisMatIndex);

        if (scs->ScintType == 2)
        {
            const double dist = (z2 - z1) * RandGen->Rndm();
            PhotonOnStart.r[2] = z1 + dist;
            PhotonOnStart.time = time0 + timeOfDrift;
            if (driftSpeedSecScint != 0)
                PhotonOnStart.time += dist / driftSpeedSecScint;
        }

        PhotonOnStart.SimStat = OneEvent->SimStat;

        photonTracker->TracePhoton(&PhotonOnStart);
    }
    //================================

    //filling scs for secondary
    if (scs->ScintType == 2)
    {
        scs->Points[iPoint].r[2] = z1;
        scs->zStop = z2;
    }
}

bool APointSourceSimulator::FindSecScintBounds(double *r, double & z1, double & z2, double & timeOfDrift, double & driftSpeedInSecScint)
{
    TGeoNavigator *navigator = detector.GeoManager->GetCurrentNavigator();
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

void APointSourceSimulator::OneNode(ANodeRecord & node)
{
    ANodeRecord * thisNode = &node;
    std::unique_ptr<ANodeRecord> outNode(ANodeRecord::createS(1e10, 1e10, 1e10)); // if outside will use this instead of thisNode

    const int numPoints = 1 + thisNode->getNumberOfLinkedNodes();
    OneEvent->clearHits();
    AScanRecord* sr = new AScanRecord();
    sr->Points.Reinitialize(numPoints);
    sr->ScintType = ScintType;

    for (int iPoint = 0; iPoint < numPoints; iPoint++)
    {
        const bool bInside = !(fLimitNodesToObject && !isInsideLimitingObject(thisNode->R));
        if (bInside)
        {
            for (int i=0; i<3; i++) sr->Points[iPoint].r[i] = thisNode->R[i];
            sr->Points[iPoint].energy = (thisNode->NumPhot == -1 ? PhotonsToRun() : thisNode->NumPhot);
        }
        else
        {
            for (int i=0; i<3; i++) sr->Points[iPoint].r[i] =  outNode->R[i];
            sr->Points[iPoint].energy = 0;
        }

        if (!simSettings.fLRFsim)
        {
            GenerateTraceNphotons(sr, thisNode->Time, iPoint);
        }
        else
        {
            if (bInside)
            {
                // NumPhotElPerLrfUnity - scaling: LRF of 1.0 corresponds to NumPhotElPerLrfUnity photo-electrons
                int numPhots = sr->Points[iPoint].energy;  // photons (before scaling) to generate at this node
                double energy = 1.0 * numPhots / simSettings.NumPhotsForLrfUnity; // NumPhotsForLRFunity corresponds to the total number of photons per event for unitary LRF

                //Generating events
                for (int ipm = 0; ipm < detector.PMs->count(); ipm++)
                {
                    double avSignal = detector.LRFs->getLRF(ipm, thisNode->R) * energy;
                    double avPhotEl = avSignal * simSettings.NumPhotElPerLrfUnity;
                    double numPhotEl = RandGen->Poisson(avPhotEl);

                    double signal = numPhotEl / simSettings.NumPhotElPerLrfUnity;  // back to LRF units
                    OneEvent->PMsignals[ipm] += signal;
                }
            }
            //if outside nothing to do
        }

        //if exists, continue to work with the linked node(s)
        thisNode = thisNode->getLinkedNode();
        if (!thisNode) break; //paranoic
    }

    if (!simSettings.fLRFsim) OneEvent->HitsToSignal();

    dataHub->Events.append(OneEvent->PMsignals);
    if (simSettings.fTimeResolved)
        dataHub->TimedEvents.append(OneEvent->TimedPMsignals);  //LRF sim for time-resolved will give all zeroes!

    dataHub->Scan.append(sr);
}

bool APointSourceSimulator::isInsideLimitingObject(const double *r)
{
    TGeoNavigator *navigator = detector.GeoManager->GetCurrentNavigator();

    if (!navigator)
    {
        qWarning() << "Navigator not found!";
        detector.GeoManager->AddNavigator();
    }

    TGeoNode* node = navigator->FindNode(r[0], r[1], r[2]);
    if (!node) return false;
    return (node->GetVolume() && node->GetVolume()->GetName()==LimitNodesToObject);
}

void APointSourceSimulator::ReserveSpace(int expectedNumEvents)
{
    ASimulator::ReserveSpace(expectedNumEvents);
}

