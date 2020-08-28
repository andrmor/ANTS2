#include "ageo_si.h"
#include "amaterialparticlecolection.h"
#include "ageoobject.h"
#include "ageoshape.h"
#include "atypegeoobject.h"
#include "aslab.h"
#include "asandwich.h"
#include "detectorclass.h"
#include "afiletools.h"
#include "ajavascriptmanager.h"

#include <QDebug>

AGeo_SI::AGeo_SI(DetectorClass* Detector)
  : Detector(Detector)
{
  Description = "Allows to configure detector geometry. Based on CERN ROOT TGeoManager";

  QString s = "It is filled with the given material (material index iMat)\n"
              "and positioned inside 'container' object\n"
              "at coordinates (x,y,z) and orientation angles (phi,thetha,psi)\n"
              "in the local frame of the container";

  H["Box"] = "Adds to geometry a box 'name' with the sizes (sizeX,sizeY,sizeZ)\n." + s;
  H["Cylinder"] = "Adds to geometry a cylinder 'name' with the given diameter and height\n" + s;
  H["Cone"] =  "Adds to geometry a cone 'name' with the given top and bottom diameters and height.\n" +s;
  H["Polygone"] =  "Adds to geometry a polygon 'name' with the given number of edges, top and bottom diameters of the inscribed "
               "circles and height.\n" +s;
  H["Sphere"] = "Adds to geometry a sphere 'name' with the given diameter.\n" + s;
  H["Arb8"] = "Adds to geometry a TGeoArb8 object with name 'name' and the given height and two arrays,\n"
              "containing X and Y coordinates of the nodes.\n"+ s;
  H["TGeo"] = "Adds to geometry an object with name 'name' and the shape generated using the CERN ROOT geometry system.\n" + s +
              "\nFor example, to generate a box of (10,10,10) half sizes use the generation string \"TGeoBBox(10, 10, 10)\".\n"
              "To generate a composite object, first create logical volumes (using TGeo command or \"Box\" etc), and then "
      "create the composite using, e.g., the generation string \"TGeoCompositeShape( name1 + name2 )\". Note that the logical volume is removed "
      "from the generation list after it was used by composite object generator!";

  H["SetLine"] = "Sets color, width and style of the line for visualisation of the object \"name\".";
  H["ClearAll"] = "Removes all unlocked objects (with exception of slabs) defined in the current geometry";
  H["Remove"] = "Removes the Object (if not locked) from the geometry.\nAll objects hosted inside "
                "are transfered to the container of the Object.\n"
                "If all objects inside also have to be deleted, use RemoveRecursive function.";
  H["RemoveRecursive"] = "Removes the Object (if not locked) and all non-locked objects hosted inside.";

  H["RemoveAllExceptWorld"] = "Removes all slabs and configured objects, ignoring the lock status!";

  H["UpdateGeometry"] = "Updates geometry and do optional check for geometry definition errors.\n"
                        "It is performed automatically for the script called from Advanced Settings window.";

  H["MakeStack"] = "Adds empty stack object. Volumes can be added normally to this object, stating its name as the container.\n"
                   "After the last element is added, call InitializeStack(StackName, Origin) function. "
                   "It will automatically calculate x,y and z positions of all elements, keeping user-configured xyz position of the Origin element.";
  H["InitializeStack"] = "Call this function after the last element has been added to the stack."
                   "It will automatically calculate x,y and z positions of all elements, keeping user-configured xyz position of the Origin element.";

  H["RecalculateStack"] = "Recalculates xyz positions of the stack elements. Has to be called if config.Replace() was used to change thickness of the elements.";

  H["setEnable"] = "Enable or disable the volume with the providfed name, or, if the name ends with '*', all volumes with the name starting with the provided string.)";
  H["getPassedVoulumes"] = "Go through the defined geometry in a straight line from startXYZ in the direction startVxVyVz\n"
          "and return array of [X Y Z MaterualIndex VolumeName NodeIndex] for all volumes on the way until final exit to the World\n"
          "the X Y Z are coordinates of the entrance points";
}

AGeo_SI::~AGeo_SI()
{
  clearGeoObjects();
}

bool AGeo_SI::InitOnRun()
{
  //qDebug() << "Init on start for Geo script unit";
  clearGeoObjects();
  return true;
}

void AGeo_SI::Box(QString name, double Lx, double Ly, double Lz, int iMat, QString container,
                                  double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoBox(0.5*Lx, 0.5*Ly, 0.5*Lz),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void AGeo_SI::Cylinder(QString name, double D, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoTube(0.5*D, 0.5*h),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void AGeo_SI::Polygone(QString name, int edges, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoPolygon(edges, 0.5*h, 0.5*Dbot, 0.5*Dtop),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void AGeo_SI::Cone(QString name, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoCone(0.5*h, 0.5*Dbot, 0.5*Dtop),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void AGeo_SI::Sphere(QString name, double D, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoSphere(0.5*D),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void AGeo_SI::Arb8(QString name, QVariant NodesX, QVariant NodesY, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  //qDebug() << NodesX << NodesY;
  QStringList lx = NodesX.toStringList();
  QStringList ly = NodesY.toStringList();
  //qDebug() << lx;
  //qDebug() << ly;
  if (lx.size()!=8 || ly.size()!=8)
    {
      clearGeoObjects();
      abort("Node arrays should contain 8 points each");
      return;
    }

  QList<QPair<double, double> > V;
  bool ok1, ok2;
  for (int i=0; i<8; i++)
    {
      double x = lx[i].toDouble(&ok1);
      double y = ly[i].toDouble(&ok2);
      if (!ok1 || !ok2)
        {
          clearGeoObjects();
          abort("Arb8 node array - conversion to double error");
          return;
        }
      V.append( QPair<double,double>(x,y) );
    }

  if (!AGeoShape::CheckPointsForArb8(V))
    {
      clearGeoObjects();
      abort("Arb8 nodes should be define clockwise for both planes");
      return;
    }

  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoArb8(0.5*h, V),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void AGeo_SI::Monitor(QString name, int shape, double size1, double size2, QString container, double x, double y, double z, double phi, double theta, double psi, bool SensitiveTop, bool SensitiveBottom, bool StopsTraking)
{
    AGeoObject* o = new AGeoObject(name, container, 0,    // no material -> it will be updated on build
                                   0,                     // no shape yet
                                   x,y,z, phi,theta,psi);

    ATypeMonitorObject* mto = new ATypeMonitorObject();
    delete o->ObjectType; o->ObjectType = mto;

    AMonitorConfig& mc = mto->config;
    mc.shape = shape;
    mc.size1 = 0.5 * size1;
    mc.size2 = 0.5 * size2;
    mc.bUpper = SensitiveTop;
    mc.bLower = SensitiveBottom;
    mc.bStopTracking = StopsTraking;

    o->updateMonitorShape();
    o->color = 1;

    GeoObjects.append(o);
}

void AGeo_SI::Monitor_ConfigureForPhotons(QString MonitorName, QVariant Position, QVariant Time, QVariant Angle, QVariant Wave)
{
    AGeoObject* o = nullptr;
    for (AGeoObject* obj : GeoObjects)
    {
        if (obj->Name == MonitorName)
        {
            o = obj;
            break;
        }
    }

    if (!o)
    {
        abort("Cannot find monitor \"" + MonitorName + "\"");
        return;
    }

    if (!o->ObjectType || !o->ObjectType->isMonitor())
    {
        abort(MonitorName + " is not a monitor object!");
        return;
    }

    ATypeMonitorObject* m = static_cast<ATypeMonitorObject*>(o->ObjectType);
    AMonitorConfig& mc = m->config;

    mc.PhotonOrParticle = 0;

    QVariantList pos = Position.toList();
    if (!pos.isEmpty())
    {
        if (pos.size() == 2)
        {
            mc.xbins = pos.at(0).toInt();
            mc.ybins = pos.at(1).toInt();
        }
        else
        {
            abort("Monitor config: Position argument should be either an empty array for default settings or an array of two integers (binsx and binsy)");
            return;
        }
    }

    QVariantList time = Time.toList();
    if (!time.isEmpty())
    {
        if (time.size() == 3)
        {
            mc.timeBins = time.at(0).toInt();
            mc.timeFrom = time.at(1).toDouble();
            mc.timeTo   = time.at(2).toDouble();
        }
        else
        {
            abort("Monitor config: Time argument should be either an empty array for default settings or an array of [bins, from, to]");
            return;
        }
    }

    QVariantList a = Angle.toList();
    if (!a.isEmpty())
    {
        if (a.size() == 3)
        {
            mc.angleBins = a.at(0).toInt();
            mc.angleFrom = a.at(1).toDouble();
            mc.angleTo   = a.at(2).toDouble();
        }
        else
        {
            abort("Monitor config: Angle argument should be either an empty array for default settings or an array of [bins, degreesFrom, degreesTo]");
            return;
        }
    }

    QVariantList w = Wave.toList();
    if (!w.isEmpty())
    {
        if (w.size() == 3)
        {
            mc.waveBins = w.at(0).toInt();
            mc.waveFrom = w.at(1).toDouble();
            mc.waveTo   = w.at(2).toDouble();
        }
        else
        {
            abort("Monitor config: Wave argument should be either an empty array for default settings or an array of [bins, from, to]");
            return;
        }
    }
}

void AGeo_SI::Monitor_ConfigureForParticles(QString MonitorName, int ParticleIndex, int Both_Primary_Secondary, int Both_Direct_Indirect,
                                                            QVariant Position, QVariant Time, QVariant Angle, QVariant Energy)
{
    AGeoObject* o = 0;
    for (AGeoObject* obj : GeoObjects)
        if (obj->Name == MonitorName)
        {
            o = obj;
            break;
        }

    if (!o)
    {
        abort("Cannot find monitor \"" + MonitorName + "\"");
        return;
    }

    if (!o->ObjectType || !o->ObjectType->isMonitor())
    {
        abort(MonitorName + " is not a monitor object!");
        return;
    }

    ATypeMonitorObject* m = static_cast<ATypeMonitorObject*>(o->ObjectType);
    AMonitorConfig& mc = m->config;

    mc.PhotonOrParticle = 1;
    mc.ParticleIndex = ParticleIndex;

    switch (Both_Primary_Secondary)
    {
    case 0: mc.bPrimary = true;  mc.bSecondary = true;  break;
    case 1: mc.bPrimary = true;  mc.bSecondary = false; break;
    case 2: mc.bPrimary = false; mc.bSecondary = true;  break;
    default: abort("Both_Primary_Secondary: 0 - sensitive to both, 1 - sensetive only to primary, 2 - sensitive only to secondary"); return;
    }
    switch (Both_Direct_Indirect)
    {
    case 0: mc.bDirect = true;  mc.bIndirect = true;  break;
    case 1: mc.bDirect = true;  mc.bIndirect = false; break;
    case 2: mc.bDirect = false; mc.bIndirect = true;  break;
    default: abort("Both_Direct_Indirect: 0 - sensitive to both, 1 - sensitive only to direct, 2 - sensitive only to indirect"); return;
    }

    QVariantList pos = Position.toList();
    if (!pos.isEmpty())
    {
        if (pos.size() == 2)
        {
            mc.xbins = pos.at(0).toInt();
            mc.ybins = pos.at(1).toInt();
        }
        else
        {
            abort("Monitor config: Position argument should be either an empty array for default settings or an array of two integers (binsx and binsy)");
            return;
        }
    }

    QVariantList time = Time.toList();
    if (!time.isEmpty())
    {
        if (time.size() == 3)
        {
            mc.timeBins = time.at(0).toInt();
            mc.timeFrom = time.at(1).toDouble();
            mc.timeTo   = time.at(2).toDouble();
        }
        else
        {
            abort("Monitor config: Time argument should be either an empty array for default settings or an array of [bins, from, to]");
            return;
        }
    }

    QVariantList a = Angle.toList();
    if (!a.isEmpty())
    {
        if (a.size() == 3)
        {
            mc.angleBins = a.at(0).toInt();
            mc.angleFrom = a.at(1).toDouble();
            mc.angleTo   = a.at(2).toDouble();
        }
        else
        {
            abort("Monitor config: Angle argument should be either an empty array for default settings or an array of [bins, degreesFrom, degreesTo]");
            return;
        }
    }

    QVariantList e = Energy.toList();
    if (!e.isEmpty())
    {
        if (e.size() == 4 && e.at(3).toInt() >= 0 && e.at(3).toInt() < 4)
        {
            mc.energyBins = e.at(0).toInt();
            mc.energyFrom = e.at(1).toDouble();
            mc.energyTo   = e.at(2).toDouble();
            mc.energyUnitsInHist = e.at(3).toInt();
        }
        else
        {
            abort("Monitor config: Energy argument should be either an empty array for default settings or an array of [bins, from, to, units]\n"
                  "Energy units: 0,1,2,3 -> meV, eV, keV, MeV;");
            return;
        }
    }
}

void AGeo_SI::TGeo(QString name, QString GenerationString, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
    AGeoObject* o = new AGeoObject(name, container, iMat,
                                   0,
                                   x,y,z, phi,theta,psi);

    if (GenerationString.simplified().startsWith("TGeoCompositeShape"))
      {
        //qDebug() << "It is a composite!";
        QString s = GenerationString.simplified();
        s.remove("TGeoCompositeShape");
        s.remove("(");
        s.remove(")");
        s.remove("+");
        s.remove("*");
        s.remove("-");
        QStringList members = s.split(" ", QString::SkipEmptyParts);
        //qDebug() << "Requested logicals:"<<members;

        //create an empty composite object
        Detector->Sandwich->convertObjToComposite(o);
        o->clearCompositeMembers();

        //attempt to add logicals
        for (int iMem=0; iMem<members.size(); iMem++)
          {
            int index = -1;
            for (int iObj=0; iObj<GeoObjects.size(); iObj++)
             {
               if (members[iMem] == GeoObjects[iObj]->Name)
                 {
                   index = iObj;
                   break;
                 }
             }
            if (index == -1)
              {
                delete o;
                clearGeoObjects();
                abort("Error in composite object generation: logical volume "+members[iMem]+" not found!");
                return;
              }
            //found logical, transferring it to logicals container of the compsoite
            o->getContainerWithLogical()->addObjectLast(GeoObjects[index]);
            GeoObjects.removeAt(index);
          }
        o->refreshShapeCompositeMembers();
      }

    bool ok = o->readShapeFromString(GenerationString);
    if (!ok)
    {
        delete o;
        clearGeoObjects();
        abort(name+": failed to create shape using generation string: "+GenerationString);
        return;
    }
    GeoObjects.append(o);
}

/*
void AGeo_SI::Slab(QString name, int imat, double height, double size1, double size2, int shape, double angle, int sides)
{
    AGeoObject * o = new AGeoObject(name, "World", imat,  nullptr,  0, 0, 0,  0, 0, 0);

    ATypeSlabObject * slab = new ATypeSlabObject();
    delete o->ObjectType; o->ObjectType = slab;

    ASlabModel * m = slab->SlabModel;
    m->name = name;
    m->material = imat;
    m->height = height;

    ASlabXYModel & xy = m->XYrecord;
    xy.shape = shape;
    xy.size1 = size1;
    xy.size2 = size2;
    xy.angle = angle;
    xy.sides = sides;
    if (xy.sides < 3)
    {
        delete o;
        abort("Error in creating " + name + " slab:\nNumber of sides should be at least 3");
        return;
    }

    bool ok = o->UpdateFromSlabModel(m);
    if (!ok)
    {
        delete o;
        abort("Failed to create slab object: " + name);
        return;
    }

    GeoObjects.append(o);
}
*/

void AGeo_SI::SlabRectangular(QString name, int imat, double height, double size1, double size2, double angle)
{
    AGeoObject * o = new AGeoObject(name, "World", imat,  nullptr,  0, 0, 0,  0, 0, 0);

    ATypeSlabObject * slab = new ATypeSlabObject();
    delete o->ObjectType; o->ObjectType = slab;

    ASlabModel * m = slab->SlabModel;
    m->name = name;
    m->material = imat;
    m->height = height;

    ASlabXYModel & xy = m->XYrecord;
    xy.shape = 0;
    xy.size1 = size1;
    xy.size2 = size2;
    xy.angle = angle;
    xy.sides = 4;

    bool ok = o->UpdateFromSlabModel(m);
    if (!ok)
    {
        delete o;
        abort("Failed to create rectangular slab object: " + name);
        return;
    }

    GeoObjects.append(o);
}

void AGeo_SI::SlabRound(QString name, int imat, double height, double diameter)
{
    AGeoObject * o = new AGeoObject(name, "World", imat,  nullptr,  0, 0, 0,  0, 0, 0);

    ATypeSlabObject * slab = new ATypeSlabObject();
    delete o->ObjectType; o->ObjectType = slab;

    ASlabModel * m = slab->SlabModel;
    m->name     = name;
    m->material = imat;
    m->height   = height;

    ASlabXYModel & xy = m->XYrecord;
    xy.shape = 1;
    xy.size1 = xy.size2 = diameter;
    xy.angle = 0;
    xy.sides = 4;

    bool ok = o->UpdateFromSlabModel(m);
    if (!ok)
    {
        delete o;
        abort("Failed to create round slab object: " + name);
        return;
    }

    GeoObjects.append(o);
}

void AGeo_SI::SlabPolygon(QString name, int imat, double height, double outsideDiamater, double angle, int sides)
{
    AGeoObject * o = new AGeoObject(name, "World", imat,  nullptr,  0, 0, 0,  0, 0, 0);

    ATypeSlabObject * slab = new ATypeSlabObject();
    delete o->ObjectType; o->ObjectType = slab;

    ASlabModel * m = slab->SlabModel;
    m->name = name;
    m->material = imat;
    m->height = height;

    ASlabXYModel & xy = m->XYrecord;
    xy.shape = 2;
    xy.size1 = xy.size2 = outsideDiamater;
    xy.angle = angle;
    xy.sides = sides;
    if (xy.sides < 3)
    {
        delete o;
        abort("Error in creating " + name + " polygon slab:\nNumber of sides should be at least 3");
        return;
    }

    bool ok = o->UpdateFromSlabModel(m);
    if (!ok)
    {
        delete o;
        abort("Failed to create polygon slab object: " + name);
        return;
    }

    GeoObjects.append(o);
}

void AGeo_SI::SetCenterSlab(QString name, int iType)
{
    if (iType < -1 || iType > 1)
    {
        abort("ZeroSlabType can be -1 (upper bound), 0 (slab center) or 1 (lower bound)");
        return;
    }

    AGeoObject * obj = nullptr;

    for (int i = 0; i < GeoObjects.size(); i++)
    {
        const QString & GOname = GeoObjects.at(i)->Name;
        if (GOname == name)
        {
            obj = GeoObjects[i];
            break;
        }
    }

    if (!obj)
    {
        //looking through already defined objects in the geometry
        obj = Detector->Sandwich->World->findObjectByName(name);
    }
    if (!obj)
    {
        abort("Cannot find object " + name);
        return;
    }

    ASlabModel * slab = obj->getSlabModel();
    if (!slab)
    {
        abort("This is not a slab: " + name);
        return;
    }

    for (AGeoObject * obj : Detector->Sandwich->World->HostedObjects)
    {
        ASlabModel * m = obj->getSlabModel();
        if (m) m->fCenter = false;
    }
    slab->fCenter = true;
    Detector->Sandwich->ZOriginType = iType;
}

void AGeo_SI::MakeStack(QString name, QString container)
{
    AGeoObject* o = new AGeoObject(name, container, 0, 0, 0,0,0, 0,0,0);
    delete o->ObjectType;
    o->ObjectType = new ATypeStackContainerObject();
    GeoObjects.append(o);
}

void AGeo_SI::InitializeStack(QString StackName, QString Origin_MemberName)
{
    AGeoObject* StackObj = 0;
    for (AGeoObject* obj : GeoObjects)
        if (obj->Name == StackName && obj->ObjectType->isStack())
        {
            StackObj = obj;
            break;
        }

    if (!StackObj)
    {
        abort("Stack with name " + StackName + " not found!");
        return;
    }

    bool bFound = false;
    AGeoObject* origin_obj = 0;
    for (int io=0; io<GeoObjects.size(); io++)
    {
        AGeoObject* obj = GeoObjects.at(io);
        if (obj->Name == Origin_MemberName)
        {
            bFound = true;
            origin_obj = obj;
        }
        if (obj->tmpContName == StackName)
            StackObj->HostedObjects << obj;
    }

    if (!bFound)
    {
        abort("Stack element with name " + Origin_MemberName + " not found!");
        return;
    }

   origin_obj->Container = StackObj;
   origin_obj->updateStack();

   origin_obj->Container = 0;
   StackObj->HostedObjects.clear();
}

void AGeo_SI::MakeGroup(QString name, QString container)
{
    AGeoObject* o = new AGeoObject(name, container, 0, 0, 0,0,0, 0,0,0);
    delete o->ObjectType;
    o->ObjectType = new ATypeGroupContainerObject();
    GeoObjects.append(o);
}

void AGeo_SI::RecalculateStack(QString name)
{
  AGeoObject* obj = Detector->Sandwich->World->findObjectByName(name);
  if (!obj)
  {
      abort("Stack or stack element with name " + name + " not found!");
      return;
  }

  if (obj->ObjectType->isStack())
  {
      if (obj->HostedObjects.isEmpty()) return; //nothing to recalculate
      else obj = obj->HostedObjects.first();
  }
  else if (!obj->Container->ObjectType->isStack())
  {
      abort("Object with name " + name + " is not a stack or stack element!");
      return;
  }

  obj->updateStack();
  Detector->BuildDetector_CallFromScript();
}

void AGeo_SI::Array(QString name, int numX, int numY, int numZ, double stepX, double stepY, double stepZ, QString container, double x, double y, double z, double psi)
{
    AGeoObject* o = new AGeoObject(name, container, 0, 0, x,y,z, 0,0,psi);
    delete o->ObjectType;
    o->ObjectType = new ATypeArrayObject(numX, numY, numZ, stepX, stepY, stepZ);
    GeoObjects.append(o);
}

void AGeo_SI::ReconfigureArray(QString name, int numX, int numY, int numZ, double stepX, double stepY, double stepZ)
{
    AGeoObject* obj = Detector->Sandwich->World->findObjectByName(name);
    if (!obj)
    {
        abort("Cannot find object "+name);
        return;
    }

    if (obj->ObjectType->isArray())
    {
        abort("This object is not an array: "+name);
        return;
    }

    ATypeArrayObject* a = static_cast<ATypeArrayObject*>(obj->ObjectType);
    a->Reconfigure(numX, numY, numZ, stepX, stepY, stepZ);
}

void AGeo_SI::SetLine(QString name, int color, int width, int style)
{
  AGeoObject * obj = nullptr;

  for (int i = 0; i < GeoObjects.size(); i++)
  {
      const QString & GOname = GeoObjects.at(i)->Name;
      if (GOname == name)
      {
          obj = GeoObjects[i];
          break;
      }
  }

  if (!obj)
  {
      //looking through already defined objects in the geometry
      obj = Detector->Sandwich->World->findObjectByName(name);
  }
  if (!obj)
  {
      abort("Cannot find object " + name);
      return;
  }

  //changing line style
  obj->color = color;
  obj->width = width;
  obj->style = style;

  ASlabModel* slab = obj->getSlabModel();
  if (slab)
  {
      slab->color = color;
      slab->width = width;
      slab->style = style;
  }
}

void AGeo_SI::ClearAll()
{
  clearGeoObjects();

  Detector->Sandwich->World->recursiveSuicide();  // locked objects are not deleted!
  Detector->BuildDetector_CallFromScript();
}

void AGeo_SI::Remove(QString Object)
{
    AGeoObject* obj = Detector->Sandwich->World->findObjectByName(Object);
    if (!obj)
    {
        abort("Cannot find object "+Object);
        return;
    }
    bool ok = obj->suicide();
    if (!ok)
    {
        abort("Failed to remove object "+Object);
        return;
    }
}

void AGeo_SI::RemoveRecursive(QString Object)
{
    AGeoObject* obj = Detector->Sandwich->World->findObjectByName(Object);
    if (!obj)
    {
        abort("Cannot find object "+Object);
        return;
    }
    obj->recursiveSuicide();
}

void AGeo_SI::RemoveAllExceptWorld()
{
  Detector->Sandwich->clearWorld();
}

void AGeo_SI::EnableObject(QString Object)
{
  AGeoObject* obj = Detector->Sandwich->World->findObjectByName(Object);
  if (!obj)
  {
      abort("Cannot find object "+Object);
      return;
  }

  obj->enableUp();
}

void AGeo_SI::DisableObject(QString Object)
{
  AGeoObject* obj = Detector->Sandwich->World->findObjectByName(Object);
  if (!obj)
  {
      abort("Cannot find object "+Object);
      return;
  }

  if (!obj->isWorld() && !obj->ObjectType->isSlab())
    obj->fActive = false;
}

void AGeo_SI::setEnable(QString ObjectOrWildcard, bool flag)
{
    if (ObjectOrWildcard.endsWith('*'))
    {
        ObjectOrWildcard.chop(1);
        //qDebug() << "Looking for all objects starting with" << ObjectOrWildcard;
        QVector<AGeoObject*> foundObjs;
        Detector->Sandwich->World->findObjectsByWildcard(ObjectOrWildcard, foundObjs);

        for (AGeoObject * obj: foundObjs)
            if (!obj->isWorld() && !obj->ObjectType->isSlab())
                obj->fActive = flag;
    }
    else
    {
        AGeoObject* obj = Detector->Sandwich->World->findObjectByName(ObjectOrWildcard);
        if (!obj)
            abort("Cannot find object " + ObjectOrWildcard);
        else
        {
            if (!obj->isWorld() && !obj->ObjectType->isSlab())
                obj->fActive = flag;
        }
    }
}

void AGeo_SI::UpdateGeometry(bool CheckOverlaps)
{
  //checkup
  for (int i = 0; i < GeoObjects.size(); i++)
  {
      const QString & name = GeoObjects.at(i)->Name;
      //qDebug() << "Checking"<<name;
      if (Detector->Sandwich->World->isNameExists(name))
      {
          clearGeoObjects();
          abort("Add geo object: Name already exists in the detector geometry: " + name);
          return;
      }
      for (int j = 0; j < GeoObjects.size(); j++)
      {
          if (i == j) continue;
          if (name == GeoObjects.at(j)->Name)
          {
              clearGeoObjects();
              abort("Add geo object: At least two objects have the same name: " + name);
              return;
          }
      }

      int imat = GeoObjects.at(i)->Material;
      if (imat < 0 || imat > Detector->MpCollection->countMaterials()-1)
      {
          clearGeoObjects();
          abort("Add geo object: Wrong material index for object " + name);
          return;
      }

      const QString & cont = GeoObjects.at(i)->tmpContName;
      bool fFound = Detector->Sandwich->World->isNameExists(cont);
      if (!fFound)
      {
          //maybe it will be inside one of the GeoObjects defined ABOVE this one?
          for (int j = 0; j < i; j++)
          {
              if (cont == GeoObjects.at(j)->Name)
              {
                  fFound = true;
                  break;
              }
          }
          if (!fFound)
          {
              clearGeoObjects();
              abort("Add geo object: Container does not exist: " + cont);
              return;
          }
      }
  }

  //adding objects
  for (int i = 0; i < GeoObjects.size(); i++)
  {
     AGeoObject * obj = GeoObjects[i];
     const QString & name     = obj->Name;
     const QString & contName = obj->tmpContName;
     AGeoObject* contObj = Detector->Sandwich->World->findObjectByName(contName);
     if (!contObj)
     {
         clearGeoObjects();
         abort("Add geo object: Failed to add object "+name+" to container "+contName);
         return;
     }
     contObj->addObjectLast(obj);
     GeoObjects[i] = nullptr;
  }

  Detector->BuildDetector_CallFromScript();

  if (CheckOverlaps)
  {
      int overlaps = Detector->checkGeoOverlaps();
      if (overlaps > 0)
      {
          emit requestShowCheckUpWindow();
          abort("Add geo object: Overlap(s) detected!");
      }
  }
}

void AGeo_SI::clearGeoObjects()
{
    for (int i=0; i<GeoObjects.size(); i++)
        delete GeoObjects[i];
    GeoObjects.clear();
}

#include "aopticaloverride.h"
QString AGeo_SI::printOverrides()
{
    QString s("");
    int nmat = Detector->MpCollection->countMaterials();
    s += QString("%1\n").arg(nmat);

    for (int i=0; i<nmat; i++) {
    QVector<AOpticalOverride*> VecOvr = (*(Detector->MpCollection))[i]->OpticalOverrides;
    if (VecOvr.size() > 0)
        for (int j=0; j<VecOvr.size(); j++) {
            if (VecOvr[j]) {
                  s = s + QString("%1 ").arg(i) + QString("%1 ").arg(j);
//                  s = s + VecOvr[j]->getType() + " "
                  s = s + VecOvr[j]->getReportLine() + QString("\n");
             }
        }
    }
    return s;
}

#include "TGeoManager.h"
QVariantList AGeo_SI::getPassedVoulumes(QVariantList startXYZ, QVariantList startVxVyVz)
{
    QVariantList vl;

    if (startXYZ.length() != 3 || startVxVyVz.length() != 3)
    {
        abort("input arguments should be arrays of 3 numbers");
        return vl;
    }

    double r[3], v[3];
    for (int i=0; i<3; i++)
    {
        r[i] = startXYZ[i].toDouble();
        v[i] = startVxVyVz[i].toDouble();
    }

    TGeoNavigator * navigator = Detector->GeoManager->GetCurrentNavigator();
    if (!navigator)
    {
        qDebug() << "Tracking: Current navigator does not exist, creating new";
        navigator = Detector->GeoManager->AddNavigator();
    }

    navigator->SetCurrentPoint(r);
    navigator->SetCurrentDirection(v);
    navigator->FindNode();

    if (navigator->IsOutside())
    {
        abort("The starting point is outside the defined geometry");
        return vl;
    }

    do
    {
        QVariantList el;

        for (int i=0; i<3; i++)
            el << navigator->GetCurrentPoint()[i];
        TGeoNode * node = navigator->GetCurrentNode();
        TGeoVolume * vol = node->GetVolume();
        el << vol->GetMaterial()->GetIndex();
        el << vol->GetName();
        el << node->GetNumber();

        vl.push_back(el);

        navigator->FindNextBoundaryAndStep();
    }
    while (!navigator->IsOutside());

    return vl;
}

