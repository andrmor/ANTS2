#include "localscriptinterfaces.h"
#include "amaterialparticlecolection.h"
#include "ageoobject.h"
#include "slab.h"
#include "asandwich.h"
#include "detectorclass.h"
#include "afiletools.h"
#include "ajavascriptmanager.h"

#include "QVector3D"

#ifdef GUI
//--------------------------------------------------------------
InterfaceToAddObjScript::InterfaceToAddObjScript(DetectorClass* Detector)
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
}

InterfaceToAddObjScript::~InterfaceToAddObjScript()
{
  clearGeoObjects();
}

bool InterfaceToAddObjScript::InitOnRun()
{
  //qDebug() << "Init on start for Geo script unit";
  clearGeoObjects();
  return true;
}

void InterfaceToAddObjScript::Box(QString name, double Lx, double Ly, double Lz, int iMat, QString container,
                                  double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoBox(0.5*Lx, 0.5*Ly, 0.5*Lz),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void InterfaceToAddObjScript::Cylinder(QString name, double D, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoTube(0.5*D, 0.5*h),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void InterfaceToAddObjScript::Polygone(QString name, int edges, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoPolygon(edges, 0.5*h, 0.5*Dbot, 0.5*Dtop),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void InterfaceToAddObjScript::Cone(QString name, double Dtop, double Dbot, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoCone(0.5*h, 0.5*Dbot, 0.5*Dtop),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void InterfaceToAddObjScript::Sphere(QString name, double D, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
{
  AGeoObject* o = new AGeoObject(name, container, iMat,
                                 new AGeoSphere(0.5*D),
                                 x,y,z, phi,theta,psi);
  GeoObjects.append(o);
}

void InterfaceToAddObjScript::Arb8(QString name, QVariant NodesX, QVariant NodesY, double h, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
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

  if (!AGeoObject::CheckPointsForArb8(V))
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

void InterfaceToAddObjScript::TGeo(QString name, QString GenerationString, int iMat, QString container, double x, double y, double z, double phi, double theta, double psi)
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

void InterfaceToAddObjScript::MakeStack(QString name, QString container)
{
    AGeoObject* o = new AGeoObject(name, container, 0, 0, 0,0,0, 0,0,0);
    delete o->ObjectType;
    o->ObjectType = new ATypeStackContainerObject();
    GeoObjects.append(o);
}

void InterfaceToAddObjScript::InitializeStack(QString StackName, QString Origin_MemberName)
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

void InterfaceToAddObjScript::MakeGroup(QString name, QString container)
{
    AGeoObject* o = new AGeoObject(name, container, 0, 0, 0,0,0, 0,0,0);
    delete o->ObjectType;
    o->ObjectType = new ATypeGroupContainerObject();
    GeoObjects.append(o);
}

void InterfaceToAddObjScript::RecalculateStack(QString name)
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

void InterfaceToAddObjScript::Array(QString name, int numX, int numY, int numZ, double stepX, double stepY, double stepZ, QString container, double x, double y, double z, double psi)
{
    AGeoObject* o = new AGeoObject(name, container, 0, 0, x,y,z, 0,0,psi);
    delete o->ObjectType;
    o->ObjectType = new ATypeArrayObject(numX, numY, numZ, stepX, stepY, stepZ);
    GeoObjects.append(o);
}

void InterfaceToAddObjScript::ReconfigureArray(QString name, int numX, int numY, int numZ, double stepX, double stepY, double stepZ)
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

void InterfaceToAddObjScript::SetLine(QString name, int color, int width, int style)
{
  AGeoObject* obj = 0;

  //first look for this object in GeoObjects
  for (int i=0; i<GeoObjects.size(); i++)
    {
      const QString GOname = GeoObjects.at(i)->Name;
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
      abort("Cannot find object "+name);
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

void InterfaceToAddObjScript::ClearAll()
{
  clearGeoObjects();

  Detector->Sandwich->World->recursiveSuicide();  // locked objects are not deleted!
  Detector->BuildDetector_CallFromScript();
}

void InterfaceToAddObjScript::Remove(QString Object)
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

void InterfaceToAddObjScript::RemoveRecursive(QString Object)
{
    AGeoObject* obj = Detector->Sandwich->World->findObjectByName(Object);
    if (!obj)
    {
        abort("Cannot find object "+Object);
        return;
    }
    obj->recursiveSuicide();
}

void InterfaceToAddObjScript::RemoveAllExceptWorld()
{
  Detector->Sandwich->clearWorld();
}

void InterfaceToAddObjScript::EnableObject(QString Object)
{
  AGeoObject* obj = Detector->Sandwich->World->findObjectByName(Object);
  if (!obj)
  {
      abort("Cannot find object "+Object);
      return;
  }

  obj->enableUp();
}

void InterfaceToAddObjScript::DisableObject(QString Object)
{
  AGeoObject* obj = Detector->Sandwich->World->findObjectByName(Object);
  if (!obj)
  {
      abort("Cannot find object "+Object);
      return;
  }

  obj->fActive = false;
}

//--------------------------------------------------------------

void InterfaceToAddObjScript::UpdateGeometry(bool CheckOverlaps)
{
  //checkup    
  for (int i=0; i<GeoObjects.size(); i++)
    {
      const QString name = GeoObjects.at(i)->Name;
      //qDebug() << "Checking"<<name;
      if (Detector->Sandwich->World->isNameExists(name))
        {         
          clearGeoObjects();          
          abort("Add geo object: Name already exists in detector geometry: "+name);
          return;
        }      
      for (int j=0; j<GeoObjects.size(); j++)
        {
          if (i==j) continue;
          if (name == GeoObjects.at(j)->Name)
            {
              clearGeoObjects();              
              abort("Add geo object: At least two objects have the same name: "+name);
              return;
            }
        }

      int imat = GeoObjects.at(i)->Material;
      if (imat<0 || imat>Detector->MpCollection->countMaterials()-1)
        {
          clearGeoObjects();
          abort("Add geo object: Wrong material index for object: "+name);
          return;
        }

      const QString cont = GeoObjects.at(i)->tmpContName;
      bool fFound = Detector->Sandwich->World->isNameExists(cont);
      if (!fFound)
        {
          //maybe it will be inside one of the GeoObjects defined ABOVE this one?
          for (int j=0; j<i; j++)
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
              abort("Add geo object: Container does not exist: "+cont);
              return;
          }
        }
    }

  //adding objects
  while (!GeoObjects.isEmpty())
  {
     QString name = GeoObjects.first()->Name;
     QString contName = GeoObjects.first()->tmpContName;
     AGeoObject* contObj = Detector->Sandwich->World->findObjectByName( contName );
     if (!contObj)
     {
         clearGeoObjects();         
         abort("Add geo object: Failed to add object "+name+" to container "+contName);
         return;
     }
     contObj->addObjectLast(GeoObjects.first());
     GeoObjects.removeFirst();
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

void InterfaceToAddObjScript::clearGeoObjects()
{
    for (int i=0; i<GeoObjects.size(); i++)
        delete GeoObjects[i];
    GeoObjects.clear();
}
#endif
//-----------------------------------------

InterfaceToPMscript::InterfaceToPMscript()
{
  H["pm"] = "Adds a PM at (x0,y0) coordinates";
  H["PM"] = "Adds a PM of model number 'type' at (x0,y0,z0) coordinates and with (phi, theta,psi) orientation";
}

void InterfaceToPMscript::pm(double x, double y)
{
  QJsonArray el; el<<x<<y; arr.append(el);
}

void InterfaceToPMscript::PM(double x, double y, double z, QString type)
{
  QJsonArray el; el<<x<<y<<z<<type; arr.append(el);
}

void InterfaceToPMscript::PM(double x, double y, double z, QString type, double phi, double theta, double psi)
{
  QJsonArray el;
  el<<x<<y<<z<<type<<phi<<theta<<psi;
  arr.append(el);
}

InterfaceToNodesScript::InterfaceToNodesScript()
{
  H["node"] = "Adds a node with (x,y,z) coordinates";
}

InterfaceToNodesScript::~InterfaceToNodesScript()
{
  //on normal operation nodes are empty - the addresses are transferred to CustomScanNodes container of the main window
  //qDebug() << "nodes still defined in destructor:"<<nodes.size();
  for (int i=0; i<nodes.size(); i++) delete nodes[i];
  nodes.clear();
}

void InterfaceToNodesScript::node(double x, double y, double z)
{
  nodes.append( new QVector3D(x,y,z));
}
