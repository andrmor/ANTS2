#include "asandwich.h"
#include "slab.h"
#include "ageoobject.h"
#include "amaterialparticlecolection.h"
#include "ajsontools.h"
#include "agridelementrecord.h"
//#include "amonitor.h"

#include <QDebug>

#include "TGeoManager.h"
#include "TVector3.h"

ASandwich::ASandwich()
{  
  SandwichState = CommonShapeSize;
  ZOriginType = 0;
  DefaultXY = new ASlabXYModel();

  World = new AGeoObject("World");
  World->Material = 0;
  World->makeItWorld();
  //qDebug() << "--World object in GeoTree created";
}

ASandwich::~ASandwich()
{
    clearModel();
    clearGridRecords();

    clearWorld();
    delete World;
}

void ASandwich::clearWorld()
{    
    //deletes all but World object
    for (int i=0; i<World->HostedObjects.size(); i++)
        World->HostedObjects[i]->clearAll();
    World->HostedObjects.clear();
}

void ASandwich::appendSlab(ASlabModel *slab)
{
    AGeoObject* slabObj = new AGeoObject(slab->name);    
    slabObj->setSlabModel(slab); //changes type to slab too
    slabObj->UpdateFromSlabModel(slab);
    slabObj->Container = World;
    World->HostedObjects.append(slabObj);
}

void ASandwich::insertSlab(int index, ASlabModel *slab)
{
    AGeoObject* slabObj = new AGeoObject(slab->name);
    slabObj->setSlabModel(slab); //changes type to slab too
    slabObj->UpdateFromSlabModel(slab);
    slabObj->Container = World;

    AGeoObject* objAtIndex = findSlabByIndex(index);
    int iHosted = World->HostedObjects.indexOf(objAtIndex);
    if (iHosted>-1)
      World->HostedObjects.insert(iHosted, slabObj);
    else
      World->HostedObjects.append(slabObj);
}

int ASandwich::countSlabs()
{
    int counter = 0;
    for (int i=0; i<World->HostedObjects.size(); i++)
        if (World->HostedObjects.at(i)->ObjectType->isSlab())
            counter++;
    return counter;
}

AGeoObject *ASandwich::findSlabByIndex(int index)
{
    if (index<0) return 0;

    int counter = 0;
    for (int i=0; i<World->HostedObjects.size(); i++)
        if (World->HostedObjects.at(i)->ObjectType->isSlab())
        {
            if (counter == index) return World->HostedObjects[i];
            counter++;
        }

    return 0;
}

AGeoObject *ASandwich::getUpperLightguide()
{
    for (int i=0; i<World->HostedObjects.size(); i++)
    {
        if (World->HostedObjects[i]->ObjectType->isUpperLightguide())
            return World->HostedObjects[i];
    }
    return 0;
}

AGeoObject *ASandwich::getLowerLightguide()
{
    for (int i=World->HostedObjects.size()-1; i>-1; i--)
    {
        if (World->HostedObjects[i]->ObjectType->isLowerLightguide())
            return World->HostedObjects[i];
    }
    return 0;
}

AGeoObject* ASandwich::addLightguide(bool upper)
{
    //qDebug() << "Add lightguide triggered. Upper?"<<upper;
    AGeoObject* obj = 0;
    AGeoObject* copyFromObj = (upper ?  World->getFirstSlab() : World->getLastSlab());
    //qDebug() << "Copy from object:" << copyFromObj;
    if (copyFromObj)
    {
        //copying properties
        obj = new AGeoObject(copyFromObj);
        //qDebug() << "--object properties copied from"<<copyFromObj->Name;
        //converting to slab
        delete obj->ObjectType;
        obj->ObjectType = new ATypeSlabObject();
        *obj->getSlabModel() = *copyFromObj->getSlabModel();
        obj->getSlabModel()->fCenter = false;
        //qDebug() << "--slab properties copied";
    }
    else
    {
        //creating new
        //qDebug() << "Creating new slab";
        obj = new AGeoObject();
        //converting to slab
        delete obj->ObjectType;
        obj->ObjectType = new ATypeSlabObject();
    }
    //qDebug() << "Base slab crated:" << obj->Name << obj->ObjectType->isSlab();
    //new name
    obj->Name = (upper ? "LGtop" : "LGbot");
    while (World->isNameExists(obj->Name))
       obj->Name = AGeoObject::GenerateRandomLightguideName();
    obj->getSlabModel()->name = obj->Name;

    //qDebug() << "Updating object properties from slab model";
    obj->UpdateFromSlabModel(obj->getSlabModel());

    //qDebug() << "Converting to lightguide";
    convertObjToLightguide(obj, upper);
    //qDebug() << obj->Name << obj->ObjectType->isLightguide();

    //qDebug() << "Adding it to the WorldTree";
    int insertTo;
    if (upper)
        insertTo = World->getIndexOfFirstSlab();
    else
    {
        insertTo = World->getIndexOfLastSlab();
        if (insertTo != -1) insertTo++;  //after last!
    }
    if (insertTo == -1) insertTo = World->HostedObjects.size(); //if no slabs are there, after the last object
    //qDebug() << "Will insert at position:"<<insertTo;
    obj->Container = World;   
    World->HostedObjects.insert(insertTo, obj);

    return obj;
}

bool ASandwich::convertObjToLightguide(AGeoObject* obj, bool upper)
{
      if (!obj->ObjectType->isSlab()) return false;
      //qDebug() << "Converting to lightguide..."<<upper;

      ATypeSlabObject* slabObj = static_cast<ATypeSlabObject*>(obj->ObjectType);
      ASlabModel* slab = slabObj->SlabModel;
      slabObj->SlabModel = 0;
      delete obj->ObjectType;

      ATypeLightguideObject* LG = new ATypeLightguideObject();
      LG->SlabModel = slab;
      obj->ObjectType = LG;
      LG->UpperLower = ( upper ? ATypeLightguideObject::Upper : ATypeLightguideObject::Lower );

      if (obj->HostedObjects.isEmpty())
        {
          AGeoObject* objL = new AGeoObject();
          do objL->Name = AGeoObject::GenerateRandomGuideName();
          while (World->isNameExists(objL->Name));
          objL->color = 15;
          obj->addObjectFirst(objL);
        }
      return true;
}

void ASandwich::convertObjToComposite(AGeoObject *obj)
{
    delete obj->ObjectType;
    ATypeCompositeObject* CO = new ATypeCompositeObject();
    obj->ObjectType = CO;

    AGeoObject* logicals = new AGeoObject();
    logicals->Name = "CompositeSet_"+obj->Name;
    delete logicals->ObjectType;
    logicals->ObjectType = new ATypeCompositeContainerObject();
    obj->addObjectFirst(logicals);


    AGeoObject* first = new AGeoObject();
    while (World->isNameExists(first->Name))
      first->AGeoObject::GenerateRandomObjectName();
    first->Shape = obj->Shape;    
    first->Material = obj->Material;
    logicals->addObjectLast(first);

    AGeoObject* second = new AGeoObject();
    while (World->isNameExists(second->Name))
      second->AGeoObject::GenerateRandomObjectName();    
    second->Material = obj->Material;
    second->Position[0] = 15;
    second->Position[1] = 15;
    logicals->addObjectLast(second);

    QStringList sl;
    sl << first->Name << second->Name;
    QString str = "TGeoCompositeShape( " + first->Name + " + " + second->Name + " )";
    obj->Shape = new AGeoComposite(sl, str);
}

void ASandwich::convertObjToGrid(AGeoObject *obj)
{
  delete obj->ObjectType;
  obj->ObjectType = new ATypeGridObject();
  QString name = obj->Name;

  //grid element inside
  AGeoObject* elObj = new AGeoObject();
  delete elObj->ObjectType;
  ATypeGridElementObject* GE = new ATypeGridElementObject();
  elObj->ObjectType = GE;
  GE->dz = obj->Shape->getHeight();
  if (GE->dz == 0) GE->dz = 1.001;
  elObj->Name = "GridElement_" + name;
  elObj->color = 1;
  obj->addObjectFirst(elObj);
  elObj->updateGridElementShape();
}

void ASandwich::shapeGrid(AGeoObject *obj, int shape, double p0, double p1, double p2)
{
    //qDebug() << "Grid shape request:"<<shape<<p0<<p1<<p2;
    if (!obj) return;

    if (!obj->ObjectType->isGrid()) return;
    AGeoObject* GEobj = obj->getGridElement();
    if (!GEobj) return;
    ATypeGridElementObject* GE = static_cast<ATypeGridElementObject*>(GEobj->ObjectType);

    int previousMat = 0;
    if (!GEobj->HostedObjects.isEmpty())
        previousMat = GEobj->HostedObjects.first()->Material;
    //clear anything which is inside grid element
    for (int i=GEobj->HostedObjects.size()-1; i>-1; i--)
    {
       //qDebug() << "...."<< GEobj->HostedObjects[i]->Name;
       GEobj->HostedObjects[i]->clearAll();
    }
    GEobj->HostedObjects.clear();

    obj->Shape->setHeight(0.5*p2 + 0.001);
    GE->shape = shape;
    GE->dz = 0.5*p2 + 0.001;
    GE->size1 = 0.5*p0;
    GE->size2 = 0.5*p1;

    switch (shape)
    {
    case 0: // parallel wires
    {
        AGeoObject* w = new AGeoObject(new ATypeSingleObject(), new AGeoTubeSeg(0, 0.5*p2, 0.5*p1, 90, 270));
        w->Position[0] = 0.5*p0;
        w->Orientation[1] = 90;
        w->Material = previousMat;
        do w->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w->Name));
        GEobj->addObjectFirst(w);
        w = new AGeoObject(new ATypeSingleObject(), new AGeoTubeSeg(0, 0.5*p2, 0.5*p1, -90, 90));
        w->Position[0] = -0.5*p0;
        w->Orientation[1] = 90;
        w->Material = previousMat;
        do w->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w->Name));
        GEobj->addObjectFirst(w);
        break;
    }
    case 1: // mesh
    {
        AGeoObject* com = new AGeoObject();
        do com->Name = AGeoObject::GenerateRandomCompositeName();
        while (World->isNameExists(com->Name));
        convertObjToComposite(com);
        com->Material = previousMat;
        AGeoObject* logicals = com->getContainerWithLogical();
        for (int i=0; i<logicals->HostedObjects.size(); i++)
          delete logicals->HostedObjects[i];
        logicals->HostedObjects.clear();

        AGeoObject* w1 = new AGeoObject();
        do w1->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w1->Name));
        delete w1->Shape;
        w1->Shape = new AGeoTubeSeg(0, 0.5*p2, 0.5*p1, -90, 90);
        w1->Position[0] = -0.5*p0;
        w1->Orientation[1] = 90;
        w1->Material = previousMat;
        logicals->addObjectFirst(w1);

        AGeoObject* w2 = new AGeoObject();
        do w2->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w2->Name));
        delete w2->Shape;
        w2->Shape = new AGeoTubeSeg(0, 0.5*p2, 0.5*p1, 90, 270);
        w2->Position[0] = 0.5*p0;
        w2->Orientation[1] = 90;
        w2->Material = previousMat;
        logicals->addObjectLast(w2);

        AGeoObject* w3 = new AGeoObject();
        do w3->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w3->Name));
        delete w3->Shape;
        w3->Shape = new AGeoTubeSeg(0, 0.5*p2, 0.5*p0, 90, 270);
        w3->Position[1] = 0.5*p1;
        w3->Orientation[0] = 90;
        w3->Orientation[1] = 90;
        w3->Material = previousMat;
        logicals->addObjectLast(w3);

        AGeoObject* w4 = new AGeoObject();
        do w4->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w4->Name));
        delete w4->Shape;
        w4->Shape = new AGeoTubeSeg(0, 0.5*p2, 0.5*p0, -90, 90);
        w4->Position[1] = -0.5*p1;
        w4->Orientation[0] = 90;
        w4->Orientation[1] = 90;
        w4->Material = previousMat;
        logicals->addObjectLast(w4);

        AGeoComposite* comSh = static_cast<AGeoComposite*>(com->Shape);
        comSh->GenerationString = "TGeoCompositeShape( " +
            w1->Name + " + " +
            w2->Name + " + " +
            w3->Name + " + " +
            w4->Name + " )";

        GEobj->addObjectFirst(com);
        break;
    }
    case 2: //hexagon
    {
        AGeoObject* com = new AGeoObject();
        do com->Name = AGeoObject::GenerateRandomCompositeName();
        while (World->isNameExists(com->Name));
        convertObjToComposite(com);
        com->Material = previousMat;
        AGeoObject* logicals = com->getContainerWithLogical();
        for (int i=0; i<logicals->HostedObjects.size(); i++)
          delete logicals->HostedObjects[i];
        logicals->HostedObjects.clear();

        //qDebug() << "p0, p1, p2"<<p0<<p1<<p2;
        double d = 0.5*p0/0.86603; //radius of circumscribed circle
        double delta = 0.5*(p0-p1); // wall thickness
        //qDebug() << "d, delta:"<<d << delta;
        double dd = d + 2.0*delta*0.57735;
        double x = 0.5*(p0-delta)*0.866025;

        AGeoObject* w1 = new AGeoObject();
        do w1->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w1->Name));
        delete w1->Shape;
        w1->Shape = new AGeoTrd1( 0.5*d, 0.5*dd, 0.5*p2, 0.5*delta );
        w1->Position[1] = 0.5*p0 - 0.5*delta;
        w1->Orientation[1] = 90;
        w1->Material = previousMat;
        logicals->addObjectFirst(w1);

        AGeoObject* w2 = new AGeoObject();
        do w2->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w2->Name));
        delete w2->Shape;
        w2->Shape = new AGeoTrd1( 0.5*d, 0.5*dd, 0.5*p2, 0.5*delta );
        w2->Position[1] = -0.5*p0 + 0.5*delta;
        w2->Orientation[0] = 180;
        w2->Orientation[1] = 90;
        w2->Material = previousMat;
        logicals->addObjectFirst(w2);

        AGeoObject* w3 = new AGeoObject();
        do w3->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w3->Name));
        delete w3->Shape;
        w3->Shape = new AGeoTrd1( 0.5*d, 0.5*dd, 0.5*p2, 0.5*delta );
        w3->Position[0] = -x;
        w3->Position[1] = 0.5*(0.5*p0 - 0.5*delta);
        w3->Orientation[0] = 60;
        w3->Orientation[1] = 90;
        w3->Material = previousMat;
        logicals->addObjectFirst(w3);

        AGeoObject* w4 = new AGeoObject();
        do w4->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w4->Name));
        delete w4->Shape;
        w4->Shape = new AGeoTrd1( 0.5*d, 0.5*dd, 0.5*p2, 0.5*delta );
        w4->Position[0] = x;
        w4->Position[1] = 0.5*(0.5*p0 - 0.5*delta);
        w4->Orientation[0] = -60;
        w4->Orientation[1] = 90;
        w4->Material = previousMat;
        logicals->addObjectFirst(w4);

        AGeoObject* w5 = new AGeoObject();
        do w5->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w5->Name));
        delete w5->Shape;
        w5->Shape = new AGeoTrd1( 0.5*d, 0.5*dd, 0.5*p2, 0.5*delta );
        w5->Position[0] = -x;
        w5->Position[1] = -0.5*(0.5*p0 - 0.5*delta);
        w5->Orientation[0] = 120;
        w5->Orientation[1] = 90;
        w5->Material = previousMat;
        logicals->addObjectFirst(w5);

        AGeoObject* w6 = new AGeoObject();
        do w6->Name = AGeoObject::GenerateRandomObjectName();
        while (World->isNameExists(w6->Name));
        delete w6->Shape;
        w6->Shape = new AGeoTrd1( 0.5*d, 0.5*dd, 0.5*p2, 0.5*delta );
        w6->Position[0] = x;
        w6->Position[1] = -0.5*(0.5*p0 - 0.5*delta);
        w6->Orientation[0] = -120;
        w6->Orientation[1] = 90;
        w6->Material = previousMat;
        logicals->addObjectFirst(w6);

        AGeoComposite* comSh = static_cast<AGeoComposite*>(com->Shape);
        comSh->GenerationString = "TGeoCompositeShape( " +
                w1->Name + " + " +
                w2->Name + " + " +
                w3->Name + " + " +
                w4->Name + " + " +
                w5->Name + " + " +
                w6->Name + " )";

        GEobj->addObjectFirst(com);
        break;
    }

    default:
        qWarning() << "Unknown grid element shape!";
        return;
    }
    GEobj->updateGridElementShape();
}

void ASandwich::addTGeoVolumeRecursively(AGeoObject* obj, TGeoVolume* parent, TGeoManager* GeoManager,
                                         AMaterialParticleCollection* MaterialCollection,
                                         QList<APMandDummy> *PMsAndDumPMs)
{
    //qDebug() << "Processing TGeo creation for object"<<obj->Name<<" which must be in"<<parent->GetName();
    if (!obj->fActive) return;

    TGeoVolume* vol = 0;
    TGeoCombiTrans *lTrans = 0;

    if (obj->ObjectType->isWorld())
    {   // just a shortcut, to resuse the cycle by HostedVolumes below
        vol = parent;
    }
    else if (obj->isCompositeMemeber() || obj->ObjectType->isCompositeContainer())
    {
        return; //do nothing with logicals, they also do not host anything real
    }
    else if (obj->ObjectType->isHandlingSet() || obj->ObjectType->isArray())
    {   // group objects are pure virtual, just pass the volume of the parent
        vol = parent;
    }
    else
    {
        TGeoMedium* med = (*MaterialCollection)[obj->Material]->GeoMed;

        //creating volume
        if (obj->ObjectType->isComposite())
        {
            //qDebug() << "Composite:"<<obj->Name;
            AGeoObject* logicals = obj->getContainerWithLogical();
            if (!logicals)
            {
                qWarning()<< "Composite object: Not found container with logical objects!";
                return;
            }
            AGeoComposite* cs = dynamic_cast<AGeoComposite*>(obj->Shape);
            if (!cs)
            {
                qWarning()<< "Composite: Shape object is not composite!!";
                return;
            }
            obj->refreshShapeCompositeMembers();

            for (int i=0; i<logicals->HostedObjects.size(); i++)
            {
                //registering building blocks
                QString name = logicals->HostedObjects[i]->Name;
                logicals->HostedObjects[i]->Shape->createGeoShape(name);
                QString RotName = "_r"+name;
                TGeoRotation *lRot = new TGeoRotation(RotName.toLatin1().data(),
                                                      logicals->HostedObjects.at(i)->Orientation[0],
                        logicals->HostedObjects.at(i)->Orientation[1],
                        logicals->HostedObjects.at(i)->Orientation[2]);
                lRot->RegisterYourself();
                QString TransName = "_m"+name;
                TGeoCombiTrans *lTrans = new TGeoCombiTrans(TransName.toLatin1().data(),
                                                            logicals->HostedObjects.at(i)->Position[0],
                        logicals->HostedObjects.at(i)->Position[1],
                        logicals->HostedObjects.at(i)->Position[2],
                        lRot);
                lTrans->RegisterYourself();
                //qDebug() << "  member name:"<<name<<"  trans name:"<<TransName;
            }           


            vol = new TGeoVolume(obj->Name.toLatin1().data(), obj->Shape->createGeoShape(), med);
        }
        else
        {
            //qDebug() << obj->Name << obj->Shape->getGenerationString();
            vol = new TGeoVolume(obj->Name.toLocal8Bit().data(), obj->Shape->createGeoShape(), med);
        }

        //creating positioning/rotation transformation
        TGeoRotation *lRot = new TGeoRotation("lRot", obj->Orientation[0], obj->Orientation[1], obj->Orientation[2]);
        lRot->RegisterYourself();
        lTrans = new TGeoCombiTrans("lTrans", obj->Position[0], obj->Position[1], obj->Position[2], lRot);

        //positioning node
        if (obj->ObjectType->isGrid())
          {
            int GridCounter = GridRecords.size();
            GridRecords.append(obj->createGridRecord());
            parent->AddNode(vol, GridCounter, lTrans);            
          }
        else if (obj->ObjectType->isMonitor())
          {
            int MonitorCounter = MonitorsRecords.size();
            MonitorsRecords.append(obj);
            (static_cast<ATypeMonitorObject*>(obj->ObjectType))->index = MonitorCounter;
            parent->AddNode(vol, MonitorCounter, lTrans);
          }
        else parent->AddNode(vol, 0, lTrans);
    }    

    //positioning of hosted items is different for lightguides, arrays and normal items!
    if (obj->ObjectType->isLightguide())
    {
        int ul = ( obj->ObjectType->isUpperLightguide() ? 0 : 1);
        for (int ipm=0; ipm<PMsAndDumPMs->size(); ipm++)
          if (PMsAndDumPMs->at(ipm).UpperLower == ul)
            {
              Double_t master[3]; //PM positions are in global coordinates! - there could be rotation of the slab
              master[0] = PMsAndDumPMs->at(ipm).X;
              master[1] = PMsAndDumPMs->at(ipm).Y;
              master[2] = 0;
              Double_t local[3];
              lTrans->MasterToLocal(master, local);

              for (int i=0; i<obj->HostedObjects.size(); i++)
                {
                  AGeoObject* GO = obj->HostedObjects[i];
                  if ( !GO->ObjectType->isHandlingSet() )
                    {
                      double tmpX = GO->Position[0];
                      double tmpY = GO->Position[1];
                      GO->Position[0] += local[0];
                      GO->Position[1] += local[1];
                      addTGeoVolumeRecursively(GO, vol, GeoManager, MaterialCollection, PMsAndDumPMs);
                      GO->Position[0] = tmpX; //recovering original positions
                      GO->Position[1] = tmpY;
                    }
                  else
                    {
                      for (int i=0; i<GO->HostedObjects.size(); i++)
                        {
                          AGeoObject* GObis = GO->HostedObjects[i];
                          double tmpX = GObis->Position[0];
                          double tmpY = GObis->Position[1];
                          GObis->Position[0] += local[0];
                          GObis->Position[1] += local[1];
                          addTGeoVolumeRecursively(GObis, vol, GeoManager, MaterialCollection, PMsAndDumPMs);
                          GObis->Position[0] = tmpX; //recovering original positions
                          GObis->Position[1] = tmpY;
                        }
                    }
                }
            }
        return;
    }
    if (obj->ObjectType->isArray())
      {
        ATypeArrayObject* array = static_cast<ATypeArrayObject*>(obj->ObjectType);

        for (int i=0; i<obj->HostedObjects.size(); i++)
          {
            AGeoObject* el = obj->HostedObjects[i];
            for (int ix = 0; ix<array->numX; ix++)
             for (int iy = 0; iy<array->numY; iy++)
               for (int iz = 0; iz<array->numZ; iz++)
                {
                  if ( !el->ObjectType->isHandlingSet() )
                      positionArrayElement(ix, iy, iz, el, obj, vol, GeoManager, MaterialCollection, PMsAndDumPMs);
                  else
                      for (int i=0; i<el->HostedObjects.size(); i++)
                          positionArrayElement(ix, iy, iz, el->HostedObjects[i], obj, vol, GeoManager, MaterialCollection, PMsAndDumPMs);
                }
          }
      }
    else
    {
        for (int i=0; i<obj->HostedObjects.size(); i++)
            addTGeoVolumeRecursively(obj->HostedObjects[i], vol, GeoManager, MaterialCollection, PMsAndDumPMs);
    }

    //Grids require specific title - they are recognized by it
    if (obj->ObjectType->isGrid()) vol->SetTitle("G");
    if (obj->ObjectType->isMonitor()) vol->SetTitle("M");
    else vol->SetTitle("-");
}

void ASandwich::clearGridRecords()
{
    for (int i=0; i<GridRecords.size(); i++) delete GridRecords[i];
    GridRecords.clear();
}

void ASandwich::clearMonitorRecords()
{
    MonitorsRecords.clear(); //dno delete - it is just pointers to world tree objects
}

void ASandwich::positionArrayElement(int ix, int iy, int iz, AGeoObject* el, AGeoObject* arrayObj,
                                     TGeoVolume* parent, TGeoManager* GeoManager, AMaterialParticleCollection* MaterialCollection, QList<APMandDummy> *PMsAndDumPMs)
{
    ATypeArrayObject* array = static_cast<ATypeArrayObject*>(arrayObj->ObjectType);

    //remembering original values
    double tmpX = el->Position[0];
    double tmpY = el->Position[1];
    double tmpZ = el->Position[2];
    //double tmpA = el->Orientation[2];

    TVector3 v(arrayObj->Position[0] + el->Position[0] + ix*array->stepX,
               arrayObj->Position[1] + el->Position[1] + iy*array->stepY,
               arrayObj->Position[2] + el->Position[2] + iz*array->stepZ);
    if (arrayObj->Orientation[2] != 0) v.RotateZ(3.1415926535*arrayObj->Orientation[2]/180.0);

    el->Position[0] = v[0];
    el->Position[1] = v[1];
    el->Position[2] = v[2];

    addTGeoVolumeRecursively(el, parent, GeoManager, MaterialCollection, PMsAndDumPMs);

    //recovering original position/orientation
    el->Position[0] = tmpX;
    el->Position[1] = tmpY;
    el->Position[2] = tmpZ;
    //el->Orientation[2] = tmpA;
}

void ASandwich::clearModel()
{
  delete DefaultXY;
  clearWorld();
}

void ASandwich::UpdateDetector()
{
  //qDebug() << "Update detector triggered";
  enforceCommonProperties();

  //check that Z=0 layer is active unless it is the last active one
  if (countSlabs()>0)
    {
      int iZ = -1, numActive = 0, iFirstActive = -1;
      for (int i=0; i<World->HostedObjects.size(); i++)  //slabIndex not needed, work with raw HostedObject indexes
        {
          AGeoObject* obj = World->HostedObjects[i];
          if (!obj->ObjectType->isSlab()) continue;

          if (obj->getSlabModel()->fCenter)
            {
              if (iZ!=-1)
                { //already found center layer
                  QString s("Attempt to declare multiple center slabs!");
                  emit WarningMessage(s);
                  qWarning() << s;
                  obj->getSlabModel()->fCenter = false;
                }
              else iZ = i;
            }
          if (obj->getSlabModel()->fActive)
            {
              numActive++;
              if (iFirstActive == -1) iFirstActive = i;
            }
        }      
      //qDebug() << "iZ, numAct, firstAct"<< iZ << numActive << iFirstActive;

      if (iZ == -1)
        { //no center slab -> only allowed if number actives = 0
          if (numActive>0)
            {
              //QString s("If there are enabled slabs, one of them has to be center!");
              //emit WarningMessage(s);
              //qWarning() << s;
              World->HostedObjects[iFirstActive]->getSlabModel()->fCenter = true;
              iZ = iFirstActive;
            }
        }
      else if (!World->HostedObjects[iZ]->getSlabModel()->fActive)
        { //center slab can be declared inactive only if there are no more axctive slabs
          if (numActive>0)
            {
              QString s("Slab defining Z=0 can be disabled/removed only if there are no other enabled slabs!");
              emit WarningMessage(s);
              qWarning() << s;
              World->HostedObjects[iZ]->getSlabModel()->fActive = true;
            }
        }
    }

  emit RequestGuiUpdate();

  //Claculating Z positions of layers
  CalculateZofSlabs();

  //in GUI mode triggers RebuildDetector in MainWindow
  emit RequestRebuildDetector();
}

void ASandwich::ChangeState(ASandwich::SlabState State)
{
  SandwichState = State;
  enforceCommonProperties();

  UpdateDetector();
}

void ASandwich::enforceCommonProperties()
{
  // for common states, ensure XY shape or sizes are proper
  if (SandwichState != ASandwich::Individual)
    for (int i=0; i<World->HostedObjects.size(); i++)
      {
        AGeoObject* obj = World->HostedObjects[i];
        if (!obj->ObjectType->isSlab()) continue;

        obj->getSlabModel()->XYrecord.shape = DefaultXY->shape;
        obj->getSlabModel()->XYrecord.sides = DefaultXY->sides;
        obj->getSlabModel()->XYrecord.angle = DefaultXY->angle;

        if (SandwichState == ASandwich::CommonShapeSize)
          {
            obj->getSlabModel()->XYrecord.size1 = DefaultXY->size1;
            obj->getSlabModel()->XYrecord.size2 = DefaultXY->size2;
          }

        obj->UpdateFromSlabModel(obj->getSlabModel());
      }
}

bool ASandwich::CalculateZofSlabs()
{
  Z_UpperBound = Z_LowerBound = 0;
  LastError = "";

  // find the Z=0 slab
  int iZeroSlab;
  int FoundActiveLayers = 0;
  int i;
  for (i=0; i<World->HostedObjects.size(); i++)
    {
      AGeoObject* obj = World->HostedObjects[i];
      if (!obj->ObjectType->isSlab()) continue;

      if (obj->getSlabModel()->fActive) FoundActiveLayers++;
      if (obj->getSlabModel()->fCenter)
        {
          iZeroSlab = i;
          break;
        }
    }
  if (i == World->HostedObjects.size())
    { // potential problem - no center slab detected
      if (FoundActiveLayers == 0)
        return true; // its OK: there are no active layers, Z bounds will be both zero
      else
        {
          LastError = "There are active slabs, but no slab set as Z=0 found!";
          qCritical() << LastError;
          return false;
        }
    }

  // calculating Z of Slabs assuming center of the iZeroSlab is Z=0
  //Slabs[iZeroSlab]->Z = 0;
  World->HostedObjects[iZeroSlab]->Position[2] = 0;
  //Z_UpperBound = 0.5*Slabs[iZeroSlab]->height;
  Z_UpperBound = 0.5*World->HostedObjects[iZeroSlab]->getSlabModel()->height;
  for (int i=iZeroSlab-1; i>-1; i--)
    {
      AGeoObject* obj = World->HostedObjects[i];
      if (!obj->ObjectType->isSlab()) continue;
      if (obj->getSlabModel()->fActive)
        {
          //Slabs[i]->Z = Z_UpperBound + 0.5*Slabs[i]->height;
          obj->Position[2] = Z_UpperBound + 0.5*obj->getSlabModel()->height;
          //Z_UpperBound += Slabs[i]->height;
          Z_UpperBound += obj->getSlabModel()->height;
        }
    }
  //Z_LowerBound = -0.5*Slabs[iZeroSlab]->height;
  Z_LowerBound = -0.5*World->HostedObjects[iZeroSlab]->getSlabModel()->height;
  //for (int i=iZeroSlab+1; i<Slabs.size(); i++)
  for (int i=iZeroSlab+1; i<World->HostedObjects.size(); i++)
    {
      AGeoObject* obj = World->HostedObjects[i];
      if (!obj->ObjectType->isSlab()) continue;
      if (obj->getSlabModel()->fActive)
        {
          obj->Position[2] = Z_LowerBound - 0.5*obj->getSlabModel()->height;
          Z_LowerBound -= obj->getSlabModel()->height;
        }
    }

  //adjusting for TOP/MID/BOT of Z=0
  if (ZOriginType !=0 )
    {
      //double Shift = 0.5*Slabs[iZeroSlab]->height * ZOriginType;
      double Shift = 0.5*World->HostedObjects[iZeroSlab]->getSlabModel()->height * ZOriginType;
      //for (int i=0; i<Slabs.size(); i++)
      for (int i=0; i<World->HostedObjects.size(); i++)
      {
          AGeoObject* obj = World->HostedObjects[i];
          if (!obj->ObjectType->isSlab()) continue;
          //Slabs[i]->Z += Shift;
          obj->Position[2] += Shift;
      }

      Z_UpperBound += Shift;
      Z_LowerBound += Shift;
    }

  //DEBUG
  //for (int i=0; i<Slabs.size(); i++)
  //  qDebug() << i << Slabs.at(i)->name << Slabs.at(i)->height<< "--->" << Slabs.at(i)->Z;

  return true;
}

bool ASandwich::isMaterialInUse(int imat)
{
   return World->isMaterialInUse(imat);
}

void ASandwich::DeleteMaterial(int imat)
{
    World->DeleteMaterialIndex(imat);
}

bool ASandwich::isVolumeExist(QString name)
{
   return (World->findObjectByName(name) != 0);
}

void ASandwich::writeToJson(QJsonObject &json)
{
  QJsonObject js;

  QString state;
  switch (SandwichState)
    {
    case (ASandwich::CommonShapeSize): state = "CommonShapeSize"; break;
    case (ASandwich::CommonShape): state = "CommonShape"; break;
    case (ASandwich::Individual): state = "Individual"; break;
    default: qWarning() << "Unknown Sandwich state!";
    }
  js["State"] = state;

  QString ZeroZ;
  if (ZOriginType == -1) ZeroZ = "Top";
  else if (ZOriginType == 0) ZeroZ = "Center";
  else if (ZOriginType == 1) ZeroZ = "Bottom";
  else qWarning() << "Unknown Zero plane type!";
  js["ZeroZ"] = ZeroZ;

  QJsonObject XYjson;
  DefaultXY->writeToJson(XYjson);
  js["DefaultXY"] = XYjson;

  QJsonArray arr;
  for (int i=0; i<World->HostedObjects.size(); i++)
    {
      const AGeoObject* obj = World->HostedObjects[i];
      if (!obj->ObjectType->isSlab()) continue;

      ATypeSlabObject* slabObj = static_cast<ATypeSlabObject*>(obj->ObjectType);
      ASlabModel* slab = slabObj->SlabModel;
      QJsonObject j;
      slab->writeToJson(j);
      arr.append(j);
    }
  js["Slabs"] = arr;

  js["WorldMaterial"] = World->Material;

  QJsonArray arrTree;
  World->writeAllToJarr(arrTree);
  js["WorldTree"] = arrTree;

  json["Sandwich"] = js;
}

void ASandwich::readFromJson(QJsonObject &json)
{
  if (!json.contains("Sandwich"))
    {
      if (json.contains("SandwichSettings"))
        {
          //it is old standard
          QJsonObject js = json["SandwichSettings"].toObject();
          bool PrScintCont = json.contains("PrScintCont") && json["PrScintCont"].toBool();
          importFromOldStandardJson(js, PrScintCont);
        }
      else
        {
          //critical error - missing Sandwich settings (new or old standard) in the config file!
          qCritical() << "Critical error: Sandwich (or SandwichSettings) object is missing in the config file!";
          exit(666);
        }
    }
  else
  {
      clearModel();
      DefaultXY = new ASlabXYModel();
      ZOriginType = 0;
      World->Material = 0;
      SandwichState = ASandwich::CommonShapeSize;

      QJsonObject js = json["Sandwich"].toObject();
      if (js.contains("State"))
        {
          QString state = js["State"].toString();
          if (state == "CommonShapeSize") SandwichState = ASandwich::CommonShapeSize;
          else if (state == "CommonShape") SandwichState = ASandwich::CommonShape;
          else if (state == "Individual") SandwichState = ASandwich::Individual;
          else qWarning() << "Unknown Sandwich state in json object!";
        }
      if (js.contains("ZeroZ"))
        {
          QString ZeroZ = js["ZeroZ"].toString();
          if (ZeroZ == "Top") ZOriginType = -1;
          else if (ZeroZ == "Center") ZOriginType = 0;
          else if (ZeroZ == "Bottom") ZOriginType = 1;
          else qWarning() << "Unknown Zero plane type in json object!";
        }
      if (js.contains("DefaultXY"))
        {
          QJsonObject XYjson = js["DefaultXY"].toObject();
          DefaultXY->readFromJson(XYjson);
        }
      if (js.contains("WorldTree"))
        {
          //qDebug() << "...Loading WorldTree";
          QJsonArray arrTree = js["WorldTree"].toArray();
          World->readAllFromJarr(World, arrTree);
          //qDebug() << "...done!";
        }
      else
          qDebug() << "...Json does not contain WordTree";


      //slabs properties are written in a separate container:
      //qDebug() << "Loading slab properties";
      if (js.contains("Slabs"))
        {
          QJsonArray arr = js["Slabs"].toArray();
          //qDebug() << "Slabs found:"<<arr.size();
          for (int i=0; i<arr.size(); i++)
            {
              QJsonObject j = arr[i].toObject();
              ASlabModel* r = new ASlabModel();
              r->readFromJson(j);

              AGeoObject* obj = World->findObjectByName(r->name);
              if (!obj)
              {
                  qWarning() << "Slab"<<r->name<<"object not found! Creating new slab!";
                  appendSlab(r);
              }
              else
              {
                  //qDebug() << "Updating slab:"<<obj->Name;
                  delete obj->getSlabModel();
                  obj->setSlabModel(r);
                  obj->UpdateFromSlabModel(r);
              }
            }
        }

      if (js.contains("WorldMaterial")) World->Material = js["WorldMaterial"].toInt();
  }

  if (json.contains("AddGeoObjects"))
  {
      qDebug() << "==Found old system of GeoObjects (replaced now by GeoTree)";
      QJsonObject js = json["AddGeoObjects"].toObject();
      bool fAllActive = js["MasterActivate"].toBool();
      qDebug() << "==All active flag:"<<fAllActive;
      QString script = js["Script"].toString();
      qDebug() << "==Script:"<<script;
      World->LastScript = script;
      QJsonArray ar = js["Objects"].toArray();
      qDebug() << "==Object array with"<<ar.size()<<"objects";
      for (int i=0; i<ar.size(); i++)
      {
          QJsonObject jo1 = ar[i].toObject();
          QJsonObject jo = jo1["AddGeoObj"].toObject();
          AGeoObject* obj = new AGeoObject();
          obj->Name = jo["Name"].toString();
          obj->fActive = fAllActive && jo["Activated"].toBool();
          obj->color = 28;
          obj->Material = jo["Material"].toInt();
          QJsonArray aOri = jo["Orientation"].toArray();
          obj->Orientation[0] = aOri[0].toDouble();
          obj->Orientation[1] = aOri[1].toDouble();
          obj->Orientation[2] = aOri[2].toDouble();
          QJsonArray aPos = jo["Position"].toArray();
          obj->Position[0] = aPos[0].toDouble();
          obj->Position[1] = aPos[1].toDouble();
          obj->Position[2] = aPos[2].toDouble();
          int Shape = jo["Shape"].toInt();
          QJsonArray aSize = jo["Size"].toArray();

          switch (Shape)
          {
          case 0: //Box
              delete obj->Shape;
              obj->Shape = new AGeoBox(0.5*aSize[0].toDouble(), 0.5*aSize[1].toDouble(), 0.5*aSize[2].toDouble());
              break;
          case 1: //Cylinder
              delete obj->Shape;
              obj->Shape = new AGeoTube(0.5*aSize[0].toDouble(), 0.5*aSize[2].toDouble());
              break;
          case 2: //Cone
              delete obj->Shape;
              obj->Shape = new AGeoCone(0.5*aSize[2].toDouble(), 0.5*aSize[1].toDouble(), 0.5*aSize[0].toDouble());
              break;
          case 3: //Sphere
              delete obj->Shape;
              obj->Shape = new AGeoSphere(0.5*aSize[0].toDouble());
              break;
          case 4: //8vert
          {
              QJsonArray aVert = jo["Vertices"].toArray();
              QList<QPair<double,double> > vl;
              for (int i=0; i<aVert.size(); i++)
              {
                  QJsonArray el = aVert[i].toArray();
                  QPair<double,double> pair;
                  pair.first = el[0].toDouble();
                  pair.second = el[1].toDouble();
                  vl << pair;
              }
              double size = jo["Arb8height"].toDouble();
              obj->Shape = new AGeoArb8(0.5*size, vl);
              break;
          }
          case 5: //Polygon
              delete obj->Shape;
              obj->Shape = new AGeoPolygon(jo["Sides"].toInt(), 0.5*aSize[2].toDouble(), 0.5*aSize[1].toDouble(), 0.5*aSize[0].toDouble());
              break;
          default:
              qWarning()<< "Unknown shape!";
              delete obj;
              continue;
          }

          QString cont = jo["Container"].toString();
          if (cont == "Top") cont = "World";
          AGeoObject* contObj = World->findObjectByName(cont);
          if (!contObj)
          {
              qWarning() << "Cannot find container obj"<<cont<<"for old style GeoObj"<<obj->Name;
              delete obj;
              continue;
          }
          contObj->addObjectLast(obj);
      }
  }

  if (json.contains("Lightguides"))
  {
      qDebug() << "==Found old style lightguide data, importing";
      QJsonArray lgArr = json["Lightguides"].toArray();
      QJsonObject jsUpperC = lgArr[0].toObject();
      QJsonObject jsUpper = jsUpperC["Lightguide"].toObject();
      QJsonObject jsLowerC = lgArr[1].toObject();
      QJsonObject jsLower = jsLowerC["Lightguide"].toObject();
      importOldLightguide(jsUpper, true);
      importOldLightguide(jsLower, false);
  }

  if (json.contains("Mask"))
  {
      qDebug() << "==Found old style phantom(mask) data, importing";
      QJsonArray Arr = json["Mask"].toArray();
      for (int i=0; i<Arr.size(); i++)
      {
          QJsonObject js1 = Arr[i].toObject();
          QJsonObject js = js1["Mask"].toObject();
          importOldMask(js);
      }
  }

  if (json.contains("OpticalGrids"))
  {
      qDebug() << "== Found old style optical grids data, importing";
      QJsonArray Arr = json["OpticalGrids"].toArray();
      for (int i=0; i<Arr.size(); i++)
        {
          QJsonObject js1 = Arr[i].toObject();
          QJsonObject js = js1["OpticalGrid"].toObject();
          importOldGrid(js);
        }
  }

  //qDebug() << "Finished, requesting GUI update";
  emit RequestGuiUpdate(); // DOES NOT updates the detector!
  //UpdateDetector();
}

void ASandwich::importOldMask(QJsonObject &json)
{
    bool fActive = false;
    parseJson(json, "Active", fActive);
    if (!fActive) return;

    QString ContainerName;
    parseJson(json, "Container", ContainerName);
    AGeoObject* contObj = World->findObjectByName(ContainerName);
    if (!contObj) return;

    AGeoObject* bulkObj = new AGeoObject();
    do
    {
        bulkObj->Name = AGeoObject::GenerateRandomMaskName();
    }
    while (World->isNameExists(bulkObj->Name));

    contObj->addObjectFirst(bulkObj);
    parseJson(json, "MaterialBulk", bulkObj->Material);
    parseJson(json, "OriginX", bulkObj->Position[0]);
    parseJson(json, "OriginY", bulkObj->Position[1]);
    parseJson(json, "OriginZ", bulkObj->Position[2]);
    parseJson(json, "Phi", bulkObj->Orientation[0]);
    parseJson(json, "Theta", bulkObj->Orientation[1]);
    parseJson(json, "Psi", bulkObj->Orientation[2]);
    double thick = 10;
    parseJson(json, "Thickness", thick);
    double size = 100;
    parseJson(json, "Size", size);
    int shape = 0;
    parseJson(json, "Shape", shape);
    delete bulkObj->Shape;
    if (shape == 0)
    { // square
      bulkObj->Shape = new AGeoBox(0.5*size, 0.5*size, 0.5*thick);
    }
    else
    { // round
      bulkObj->Shape = new AGeoTube(0, 0.5*size, 0.5*thick);
    }

    // "holes"
    int mat = 0;
    parseJson(json, "MaterialHoles", mat);
    QJsonArray elA = json["Elements"].toArray();
    for (int i=0; i<elA.size(); i++)
    {
        QJsonObject js1 = elA[i].toObject();
        QJsonObject js = js1["MaskElement"].toObject();
        AGeoObject* obj = new AGeoObject();
        int shape = 0;
        parseJson(js, "Type", shape);
        delete obj->Shape;
        if (shape == 0)
        { // square
          obj->Shape = new AGeoBox(0.5*js["SizeX"].toDouble(), 0.5*js["SizeY"].toDouble(), 0.5*thick);
          double angle = 0;
          parseJson(js, "Angle", angle);
          obj->Orientation[2] = angle;
        }
        else
        { // round
          obj->Shape = new AGeoTube(0, js["Radius"].toDouble(), 0.5*thick);
        }
        parseJson(js, "DX", obj->Position[0]);
        parseJson(js, "DY", obj->Position[1]);

        bulkObj->addObjectLast(obj);
    }

    //script?
    QString Script;
    parseJson(json, "Script", Script);
    if (!Script.isEmpty())
    {
        Script = "//Conversion info:\n"
                 "//Tube(x,y, diameter)\n"
                 "//Box(x,y, sizeX,sizeY)\n"
                 "//Box(x,y, sizeX,sizeY, Angle)\n" + Script;
        bulkObj->LastScript = Script;
    }
}

void ASandwich::importOldGrid(QJsonObject &json)
{
    int Shape = 0;
    parseJson(json, "Shape", Shape);
    double SizeX = 100;
    parseJson(json, "SizeX", SizeX);
    double SizeY = 100;
    parseJson(json, "SizeY", SizeY);
    int Sides = 6;
    parseJson(json, "Sides", Sides);
    double Diameter;
    parseJson(json, "Diameter", Diameter);

    AGeoShape* geoShape;
    if (Shape<0 || Shape>2)
    {
        qWarning() << "==Unknow shape of grid container, skipping";
        return;
    }
    switch (Shape)
    {
    case 0:
        geoShape = new AGeoBox(0.5*SizeX, 0.5*SizeY, 0.5*Diameter); break;
    case 1:
        geoShape = new AGeoTube(0.5*SizeX, 0.5*Diameter); break;
    case 2:
        geoShape = new AGeoPolygon(Sides, 0.5*Diameter, 0.5*SizeX, 0.5*SizeX);
    }

    AGeoObject* gr = new AGeoObject();
    convertObjToGrid(gr);
    delete gr->Shape;
    gr->Shape = geoShape;

    double Pitch = 20;
    parseJson(json, "Pitch", Pitch);
    int Type = 0;
    parseJson(json, "Type", Type);
    //    WireAngle = 0;                //compatibility
    //    parseJson(json, "WireAngle", WireAngle);


    switch (Type)
    {
    case 0: shapeGrid(gr, 0, Pitch, SizeY, Diameter); break;
    case 1: shapeGrid(gr, 1, Pitch, Pitch, Diameter); break;
    default:
        qWarning() << "Unknown grid type!";
    }


    parseJson(json, "Active", gr->fActive);
    parseJson(json, "Container", gr->tmpContName);
    parseJson(json, "Phi", gr->Orientation[2]);
    parseJson(json, "Z", gr->Position[2]);
    gr->color = 1;

    AGeoObject* cont = World->findObjectByName(gr->tmpContName);
    if (!cont)
    {
        qWarning() << "Container"<<gr->tmpContName<< "not found! Adding grid to World";
        cont = World;
    }
    cont->addObjectLast(gr);
}

void ASandwich::importOldLightguide(QJsonObject &json, bool upper)
{
   bool fActive = false;
   parseJson(json, "Active", fActive);
   if (!fActive) return;

   AGeoObject* obj = addLightguide(upper); //this will be with autoshape
   if (!obj) return;
   ATypeLightguideObject* LG = static_cast<ATypeLightguideObject*>(obj->ObjectType);
   ASlabModel* slab = LG->SlabModel;
   parseJson(json, "BulkMaterial", slab->material);
   parseJson(json, "Thickness", slab->height);
   parseJson(json, "Phi", slab->XYrecord.angle);
   parseJson(json, "Shape", slab->XYrecord.shape);
   if (json.contains("SizeX"))
   {
       parseJson(json, "SizeX", slab->XYrecord.size1);
       slab->XYrecord.size1 *= 2.0;
   }
   if (json.contains("SizeY"))
   {
       parseJson(json, "SizeY", slab->XYrecord.size2);
       slab->XYrecord.size2 *= 2.0;
   }

   obj->UpdateFromSlabModel(slab);

   AGeoObject* GuideObject = obj->HostedObjects.first(); //it is created by "AddLightguide"
   int OpeningShape = 0;
   parseJson(json, "OpeningsShape", OpeningShape);
   parseJson(json, "OpeningsMaterial", GuideObject->Material);
   parseJson(json, "Angle", GuideObject->Orientation[2]);
   double size = 10;
   parseJson(json, "OpeningsSize", size);
   double cone = 1;
   if (json["Cone"].toBool())
     parseJson(json, "ConeFactor", cone);

   double rlow = 0.5*size * (  upper ? cone : 1.0);
   double rhi =  0.5*size * ( !upper ? cone : 1.0);

   delete GuideObject->Shape;
   if (OpeningShape == 0)
   { // trap
     GuideObject->Shape = new AGeoTrd2(rlow, rhi, rlow, rhi, 0.5*slab->height);
   }
   else
   { // cone
     GuideObject->Shape = new AGeoCone(0.5*slab->height, rlow, rhi)  ;
   }
}

void ASandwich::importFromOldStandardJson(QJsonObject &json, bool fPrScintCont)
{
  //clear phase
  clearModel();
  DefaultXY = new ASlabXYModel();
  ZOriginType = 0;
  World->Material = 0;
  SandwichState = ASandwich::CommonShapeSize;

  if (json.isEmpty()) return;
  if (!json.contains("UseDefaultSizes")) return; //means wrong file

  //always MID
  ZOriginType = 0;

  //Layers
  bool fLayersFound = false;
  QJsonArray layers = json["Layers"].toArray();
  for (int i=0; i<layers.size(); i++)
    {
      QJsonObject j = layers[i].toObject();
      ASlabModel* r = new ASlabModel();
      r->importFromOldJson(j);
      if (!r->fActive)
        {
          //do not load inactive layers
          delete r;
          continue;
        }
      r->fCenter = (i==5); //this is PrimScint
      QString name;
      switch (i)
        {
        case 0: name = "Sp1"; break;
        case 1: name = "UpWin"; break;
        case 2: name = "Sp2"; break;
        case 3: name = "SecScint"; break;
        case 4: name = "Sp3"; break;
        case 5: name = fPrScintCont ? "PrScintCont" : "PrScint"; break;
        case 6: name = "Sp4"; break;
        case 7: name = "LowWin"; break;
        case 8: name = "Sp5"; break;
        default: name = "Undefined";
        }
      r->name = name;
      //Slabs.append(r);
      appendSlab(r);
      fLayersFound = true;
    }

  // State and default XY
  bool fCommon = json["UseDefaultSizes"].toBool();
  if (fCommon)
    {
      SandwichState = ASandwich::CommonShapeSize;
      if (json.contains("Shape")) DefaultXY->shape = json["Shape"].toInt();
      if (json.contains("Sides")) DefaultXY->sides = json["Sides"].toInt();
      if (json.contains("DefaultSize1")) DefaultXY->size1 = json["DefaultSize1"].toDouble();
      if (json.contains("DefaultSize2")) DefaultXY->size2 = json["DefaultSize2"].toDouble();
      if (json.contains("Angle")) DefaultXY->angle = json["Angle"].toDouble();
    }
  else
    {
      SandwichState = ASandwich::Individual;
      //just for consistency take defaultXY from top slab
      //if (Slabs.size()>0) *DefaultXY = Slabs.at(0)->XYrecord;
      if (fLayersFound &&  World->HostedObjects.at(0)->ObjectType->isSlab())  //in old system there are only slabs in the tree
          *DefaultXY = World->HostedObjects.at(0)->getSlabModel()->XYrecord;
    }

  if (json.contains("TopMaterial")) World->Material = json["TopMaterial"].toInt();

  //check if can change "Individual" to "CommonShape"
  if (SandwichState == ASandwich::Individual && fLayersFound)
    {
      bool fSame = true;
      for (int i=1; i<World->HostedObjects.size(); i++)
        {
          if ( !World->HostedObjects.at(i)->getSlabModel()->XYrecord.isSameShape(World->HostedObjects.at(0)->getSlabModel()->XYrecord) )
            {
              fSame = false;
              break;
            }
        }

      if (fSame) SandwichState = ASandwich::CommonShape;
  }
}

void ASandwich::onMaterialsChanged(const QStringList MaterialList)
{
  Materials = MaterialList;  
  emit RequestGuiUpdate();  
}

