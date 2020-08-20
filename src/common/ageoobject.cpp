#include "ageoobject.h"
#include "ageoshape.h"
#include "atypegeoobject.h"
#include "aslab.h"
#include "ajsontools.h"
#include "agridelementrecord.h"
#include "ageoconsts.h"

#include <cmath>

#include <QDebug>

void AGeoObject::constructorInit()
{
  Name = AGeoObject::GenerateRandomObjectName();

  Position[0] = Position[1] = Position[2] = 0;
  Orientation[0] = Orientation[1] =Orientation[2] = 0;
}

AGeoObject::AGeoObject(QString name, QString ShapeGenerationString)
{
  constructorInit();

  if (!name.isEmpty()) Name = name;
  color = -1;

  ObjectType = new ATypeSingleObject();

  Shape = new AGeoBox();
  if (!ShapeGenerationString.isEmpty())
    readShapeFromString(ShapeGenerationString);
}

AGeoObject::AGeoObject(const AGeoObject *objToCopy)
{
    constructorInit();

    Container = objToCopy->Container;    
    ObjectType = new ATypeSingleObject();    

    Material = objToCopy->Material;

    Position[0] = objToCopy->Position[0];
    Position[1] = objToCopy->Position[1];
    Position[2] = objToCopy->Position[2] + 10;  // !!! to not overlap
    Orientation[0] = objToCopy->Orientation[0];
    Orientation[1] = objToCopy->Orientation[1];
    Orientation[2] = objToCopy->Orientation[2];

    style = objToCopy->style;
    width = objToCopy->style;

    bool ok = readShapeFromString(objToCopy->Shape->getGenerationString());
    if (!ok) Shape = new AGeoBox();
}

AGeoObject::AGeoObject(const QString & name, const QString & container, int iMat, AGeoShape * shape,  double x, double y, double z,  double phi, double theta, double psi)
{
    constructorInit();

    ObjectType = new ATypeSingleObject();    

    Name = name;
    tmpContName = container;
    Material = iMat;

    Position[0] = x;
    Position[1] = y;
    Position[2] = z;
    Orientation[0] = phi;
    Orientation[1] = theta;
    Orientation[2] = psi;

    Shape = shape;
}

AGeoObject::AGeoObject(ATypeGeoObject *objType, AGeoShape* shape)
{
    constructorInit();

    ObjectType = objType;
    if (shape) Shape = shape;
    else Shape = new AGeoBox();
}

AGeoObject::~AGeoObject()
{
    delete Shape;
    delete ObjectType;
}

bool AGeoObject::readShapeFromString(const QString & GenerationString, bool OnlyCheck)
{  
  QStringList sl = GenerationString.split(QString("("), QString::SkipEmptyParts);
  if (sl.size()<2)
    {
      qWarning() << "Format error in Shape generation string!";
      return false;
    }
  QString type = sl.first().simplified();

  AGeoShape* newShape = AGeoShape::GeoShapeFactory(type);
  if (!newShape)
    {
      qWarning() << "Failed to create Shape using generation string!";
      return false;
    }

  //Composite shape requires additional cofiguration, and it is not possible to convert to composite in this way!
  if (newShape->getShapeType() == "TGeoCompositeShape")
  {
     if (!ObjectType->isComposite())
     {
         qWarning() << "Cannot convert to composite in this way, use context menu!";
         delete newShape;
         return false;
     }
     refreshShapeCompositeMembers(newShape);
  }

  bool ok = newShape->readFromString(GenerationString);
  if (!ok)
    {
      qWarning() << "Failed to read shape properties from the generation string!";
      delete newShape;
      return false;
    }

  if (OnlyCheck)
    {
      delete newShape;
      return true;
    }

  if (Shape) delete Shape;
  Shape = newShape;
  return true;
}

void AGeoObject::DeleteMaterialIndex(int imat)
{
    if (Material > imat) Material--;

    ASlabModel* slab = getSlabModel();
    if (slab)
        if (slab->material > imat) slab->material--;

    for (int i=0; i<HostedObjects.size(); i++)
        HostedObjects[i]->DeleteMaterialIndex(imat);
}

void AGeoObject::makeItWorld()
{
    if (isWorld()) return;

    Name = "World";
    delete ObjectType; ObjectType = new ATypeWorldObject();
    Container = nullptr;
}

bool AGeoObject::isWorld() const
{
    if (!ObjectType) return false;
    return ObjectType->isWorld();
}

int AGeoObject::getMaterial() const
{
    if (ObjectType->isHandlingArray() || ObjectType->isHandlingSet())
        return Container->getMaterial();
    return Material;
}

const AGeoObject * AGeoObject::isGeoConstInUse(const QRegExp & nameRegExp) const
{
    for (int i = 0; i < 3; i++)
    {
        if (PositionStr[i]   .contains(nameRegExp)) return this;
        if (OrientationStr[i].contains(nameRegExp)) return this;
    }
    if (Shape && Shape->isGeoConstInUse(nameRegExp)) return this;
    if (ObjectType && ObjectType->isGeoConstInUse(nameRegExp)) return this;  //good?
    return nullptr;
}

void AGeoObject::replaceGeoConstName(const QRegExp & nameRegExp, const QString & newName)
{
    for (int i = 0; i < 3; i++)
    {
        PositionStr[i]   .replace(nameRegExp, newName);
        OrientationStr[i].replace(nameRegExp, newName);
    }
    if (Shape)      Shape->replaceGeoConstName(nameRegExp, newName);
    if (ObjectType) ObjectType->replaceGeoConstName(nameRegExp, newName);
}

const AGeoObject *AGeoObject::isGeoConstInUseRecursive(const QRegExp & nameRegExp) const
{
    qDebug() <<"name of current "<<this->Name;
    if (isGeoConstInUse(nameRegExp)) return this;

    for (AGeoObject * hosted : HostedObjects)
    {
        const AGeoObject * obj = hosted->isGeoConstInUseRecursive(nameRegExp);
        if (obj) return obj;
    }
    return nullptr;
}

void AGeoObject::replaceGeoConstNameRecursive(const QRegExp & nameRegExp, const QString & newName)
{
    replaceGeoConstName(nameRegExp, newName);

    for (AGeoObject * hosted : HostedObjects)
        hosted->replaceGeoConstNameRecursive(nameRegExp, newName);
}

void AGeoObject::writeToJson(QJsonObject &json)
{
  json["Name"] = Name;

  QJsonObject jj;
  ObjectType->writeToJson(jj);
  json["ObjectType"] = jj;
  json["Container"] = Container ? Container->Name : "";
  json["Locked"] = fLocked;
  json["fActive"] = fActive;
  json["fExpanded"] = fExpanded;

  if ( ObjectType->isHandlingStandard() || ObjectType->isWorld())
  {
      json["Material"] = Material;
      json["color"] = color;
      json["style"] = style;
      json["width"] = width;

      if (!ObjectType->isMonitor()) // monitor shape is in ObjectType
      {
          json["Shape"] = Shape->getShapeType();
          QJsonObject js;
          Shape->writeToJson(js);
          json["ShapeSpecific"] = js;
      }
  }

  if ( ObjectType->isHandlingStandard() || ObjectType->isArray() )
  {
      json["X"] = Position[0];
      json["Y"] = Position[1];
      json["Z"] = Position[2];
      json["Phi"]   = Orientation[0];
      json["Theta"] = Orientation[1];
      json["Psi"]   = Orientation[2];

      if (!PositionStr[0].isEmpty()) json["strX"] = PositionStr[0];
      if (!PositionStr[1].isEmpty()) json["strY"] = PositionStr[1];
      if (!PositionStr[2].isEmpty()) json["strZ"] = PositionStr[2];

      if (!OrientationStr[0].isEmpty()) json["strPhi"]   = OrientationStr[0];
      if (!OrientationStr[1].isEmpty()) json["strTheta"] = OrientationStr[1];
      if (!OrientationStr[2].isEmpty()) json["strPsi"]   = OrientationStr[2];
  }

  if (!LastScript.isEmpty())
    json["LastScript"] = LastScript;
}

void AGeoObject::readFromJson(const QJsonObject & json)
{
    Name = json["Name"].toString();

    parseJson(json, "color", color);
    parseJson(json, "style", style);
    parseJson(json, "width", width);

    parseJson(json, "Locked", fLocked);
    parseJson(json, "fActive", fActive);
    parseJson(json, "fExpanded", fExpanded);

    parseJson(json, "Container", tmpContName);
    //Hosted are not loaded, they are populated later

    parseJson(json, "Material", Material);
    parseJson(json, "X",     Position[0]);
    parseJson(json, "Y",     Position[1]);
    parseJson(json, "Z",     Position[2]);
    parseJson(json, "Phi",   Orientation[0]);
    parseJson(json, "Theta", Orientation[1]);
    parseJson(json, "Psi",   Orientation[2]);

    for (int i = 0; i < 3; i++)
    {
        PositionStr[i].clear();
        OrientationStr[i].clear();
    }

    const AGeoConsts & GC = AGeoConsts::getConstInstance();

    // TODO: control of error on formula call

    if (parseJson(json, "strX",     PositionStr[0]))    GC.evaluateFormula(PositionStr[0], Position[0]);
    if (parseJson(json, "strY",     PositionStr[1]))    GC.evaluateFormula(PositionStr[1], Position[1]);
    if (parseJson(json, "strZ",     PositionStr[2]))    GC.evaluateFormula(PositionStr[2], Position[2]);
    if (parseJson(json, "strPhi",   OrientationStr[0])) GC.evaluateFormula(OrientationStr[0], Orientation[0]);
    if (parseJson(json, "strTheta", OrientationStr[1])) GC.evaluateFormula(OrientationStr[1], Orientation[1]);
    if (parseJson(json, "strPsi",   OrientationStr[2])) GC.evaluateFormula(OrientationStr[2], Orientation[2]);

    //Shape
    if (json.contains("Shape"))
    {
            QString ShapeType = json["Shape"].toString();
            QJsonObject js = json["ShapeSpecific"].toObject();

            //compatibility: old TGeoPgon is new TGeoPolygon
            if (ShapeType == "TGeoPgon")
                if (js.contains("rminL")) ShapeType = "TGeoPolygon";

            Shape = AGeoShape::GeoShapeFactory(ShapeType);
            Shape->readFromJson(js);

            //composite: cannot update memebers at this phase - HostedObjects are not set yet!
    }

    //ObjectType
    QJsonObject jj = json["ObjectType"].toObject();
    if (!jj.isEmpty())
    {
        QString tmpType;
        parseJson(jj, "Type", tmpType);
        ATypeGeoObject * newType = ATypeGeoObject::TypeObjectFactory(tmpType);
        if (newType)
        {
            delete ObjectType;
            ObjectType = newType;
            ObjectType->readFromJson(jj);
            if (ObjectType->isMonitor()) updateMonitorShape();
        }
        else qDebug() << "ObjectType read failed for object:" << Name << ", keeping default type";
    }
    else qDebug() << "ObjectType is empty for object:" << Name << ", keeping default type";

    parseJson(json, "LastScript", LastScript);
}

void AGeoObject::writeAllToJarr(QJsonArray &jarr)
{
  QJsonObject js;
  writeToJson(js);
  jarr << js;

  for (int i=0; i<HostedObjects.size(); i++)
    HostedObjects[i]->writeAllToJarr(jarr);
}

void AGeoObject::readAllFromJarr(AGeoObject * World, const QJsonArray & jarr)
{
   int size = jarr.size();
   //qDebug() << "Read all from World tree array, size:" << size;

   if (size < 1)
   {
       qWarning() << "Read World tree: size cannot be < 1";
       return;
   }

   QJsonObject worldJS = jarr[0].toObject();
   World->readFromJson(worldJS);

   AGeoObject * prevObj = World;
   for (int iob=1; iob<size; iob++)
     {
       AGeoObject * newObj = new AGeoObject();
       //qDebug() << "--record in array:"<<json;
       QJsonObject json = jarr[iob].toObject();
       newObj->readFromJson(json);
       //qDebug() << "--read success, object name:"<<newObj->Name<< "Container:"<<newObj->tmpContName;

       AGeoObject* cont = prevObj->findContainerUp(newObj->tmpContName);
       if (cont)
         {
           newObj->Container = cont;
           cont->HostedObjects.append(newObj);
           //qDebug() << "Success! Registered"<<newObj->Name<<"in"<<newObj->tmpContName;
           if (newObj->isCompositeMemeber())
             {
               //qDebug() << "---It is composite member!";
               if (cont->Container)
                 cont->Container->refreshShapeCompositeMembers(); //register in the Shape
             }
           prevObj = newObj;
         }
       else
         {
           qWarning() << "ERROR reading geo objects! Not found container:"<<newObj->tmpContName;
           delete newObj;
         }
     }
}

void AGeoObject::UpdateFromSlabModel(ASlabModel* SlabModel)
{
    fActive  = SlabModel->fActive;
    Name     = SlabModel->name;
    Material = SlabModel->material;
    color    = SlabModel->color;
    width    = SlabModel->width;
    style    = SlabModel->style;

    if (Shape) delete Shape;
    switch (SlabModel->XYrecord.shape)
    {
      case 0:
      {
        AGeoBox * box = new AGeoBox(0.5*SlabModel->XYrecord.size1, 0.5*SlabModel->XYrecord.size2, 0.5*SlabModel->height);
        if (!SlabModel->XYrecord.strSize1.isEmpty()) box->str2dx = SlabModel->XYrecord.strSize1;
        if (!SlabModel->XYrecord.strSize2.isEmpty()) box->str2dy = SlabModel->XYrecord.strSize2;
        if (!SlabModel->strHeight.isEmpty())         box->str2dz = SlabModel->strHeight;
        box->updateShape();
        Shape = box;

        Orientation[2] = SlabModel->XYrecord.angle;
        if (!SlabModel->XYrecord.strAngle.isEmpty()) OrientationStr[2] = SlabModel->XYrecord.strAngle;
        break;
      }
      case 1:
      {
        AGeoTube * tube = new AGeoTube(0.5*SlabModel->XYrecord.size1, 0.5*SlabModel->height);
        if (!SlabModel->XYrecord.strSize1.isEmpty()) tube->str2rmax = SlabModel->XYrecord.strSize1;
        if (!SlabModel->strHeight.isEmpty())         tube->str2dz   = SlabModel->strHeight;
        tube->updateShape();
        Shape = tube;
        break;
      }
      case 2:
      {
        AGeoPolygon * poly = new AGeoPolygon(SlabModel->XYrecord.sides, 0.5*SlabModel->height, 0.5*SlabModel->XYrecord.size1, 0.5*SlabModel->XYrecord.size1);
        if (!SlabModel->XYrecord.strSides.isEmpty()) poly->strNedges = SlabModel->XYrecord.strSides;
        if (!SlabModel->XYrecord.strSize1.isEmpty()) poly->str2rmaxL = poly->str2rmaxU = SlabModel->XYrecord.strSize1;
        if (!SlabModel->strHeight.isEmpty())         poly->str2dz    = SlabModel->strHeight;
        poly->updateShape();
        Shape = poly;

        Orientation[2] = SlabModel->XYrecord.angle;
        if (!SlabModel->XYrecord.strAngle.isEmpty()) OrientationStr[2] = SlabModel->XYrecord.strAngle;
        break;        
      }
    }
    AGeoConsts::getConstInstance().evaluateFormula(OrientationStr[2], Orientation[2]);
}

ASlabModel * AGeoObject::getSlabModel()
{
    if (!ObjectType->isSlab()) return nullptr;
    ATypeSlabObject* SO = static_cast<ATypeSlabObject*>(ObjectType);
    return SO->SlabModel;
}

void AGeoObject::setSlabModel(ASlabModel *slab)
{
    if (!ObjectType->isSlab())
    {
        delete ObjectType;
        ObjectType = new ATypeSlabObject();
    }

    ATypeSlabObject* slabObject = static_cast<ATypeSlabObject*>(ObjectType);
    slabObject->SlabModel = slab;
}

AGeoObject *AGeoObject::getContainerWithLogical()
{
    if ( !ObjectType->isComposite()) return 0;

    for (int i=0; i<HostedObjects.size(); i++)
    {
        AGeoObject* obj = HostedObjects[i];
        if (obj->ObjectType->isCompositeContainer())
            return obj;
    }
    return 0;
}

bool AGeoObject::isCompositeMemeber() const
{
    if (Container)
      return Container->ObjectType->isCompositeContainer();
    return false;
}

void AGeoObject::refreshShapeCompositeMembers(AGeoShape* ExternalShape)
{
    if (ObjectType->isComposite())
    {
        AGeoShape* ShapeToUpdate = ( ExternalShape ? ExternalShape : Shape );
        AGeoComposite* shape = dynamic_cast<AGeoComposite*>(ShapeToUpdate);
        if (shape)
        {
            shape->members.clear();
            AGeoObject* logicals = getContainerWithLogical();
            if (logicals)
              for (int i=0; i<logicals->HostedObjects.size(); i++)
                shape->members << logicals->HostedObjects.at(i)->Name;
        }
    }
}

bool AGeoObject::isInUseByComposite()
{
    if (!Container) return false;
    if (!Container->ObjectType->isCompositeContainer()) return false;

    AGeoObject* compo = Container->Container;
    if (!compo) return false;
    AGeoComposite* shape = dynamic_cast<AGeoComposite*>(compo->Shape);
    if (!shape) return false;

    return shape->GenerationString.contains(Name);
}

void AGeoObject::clearCompositeMembers()
{
  AGeoObject* logicals = getContainerWithLogical();
  if (!logicals) return;

  for (int i=0; i<logicals->HostedObjects.size(); i++)
    logicals->HostedObjects[i]->clearAll(); //overkill here
  logicals->HostedObjects.clear();
}

void AGeoObject::removeCompositeStructure()
{
    clearCompositeMembers();

    delete ObjectType;
    ObjectType = new ATypeSingleObject();

    for (int i=0; i<HostedObjects.size(); i++)
    {
        AGeoObject* obj = HostedObjects[i];
        if (obj->ObjectType->isCompositeContainer())
        {
            delete obj;
            HostedObjects.removeAt(i);
            return;
        }
    }
}

AGeoObject *AGeoObject::getGridElement()
{
  if (!ObjectType->isGrid()) return 0;
  if (HostedObjects.isEmpty()) return 0;

  AGeoObject* obj = HostedObjects.first();
  if (!obj->ObjectType->isGridElement()) return 0;
  return obj;
}

void AGeoObject::updateGridElementShape()
{
  if (!ObjectType->isGridElement()) return;

  ATypeGridElementObject* GE = static_cast<ATypeGridElementObject*>(ObjectType);

  if (Shape) delete Shape;
  if (GE->shape == 0 || GE->shape == 1)
    Shape = new AGeoBox(GE->size1, GE->size2, GE->dz);
  else
      Shape = new AGeoPolygon(6, GE->dz, GE->size1, GE->size1);
}

AGridElementRecord *AGeoObject::createGridRecord()
{
  AGeoObject* geObj = getGridElement();
  if (!geObj) return 0;
  if (!geObj->ObjectType->isGridElement()) return 0;

  ATypeGridElementObject* GE = static_cast<ATypeGridElementObject*>(geObj->ObjectType);
  return new AGridElementRecord(GE->shape, GE->size1, GE->size2);
}

void AGeoObject::updateMonitorShape()
{
    if (!ObjectType->isMonitor())
    {
        qWarning() << "Attempt to update monitor shape for non-monitor object";
        return;
    }

    ATypeMonitorObject* mon = static_cast<ATypeMonitorObject*>(ObjectType);
    delete Shape;

    qDebug() << "TAJNA-----"<<mon->config.size1 << mon->config.size2;

    if (mon->config.shape == 0) //rectangular
        Shape = new AGeoBox(mon->config.size1, mon->config.size2, mon->config.dz);
    else //round
        Shape = new AGeoTube(0, mon->config.size1, mon->config.dz);
}

const AMonitorConfig * AGeoObject::getMonitorConfig() const
{
    if (!ObjectType) return nullptr;
    ATypeMonitorObject * mon = dynamic_cast<ATypeMonitorObject*>(ObjectType);
    if (!mon) return nullptr;

    return &mon->config;
}

AGeoObject * AGeoObject::findObjectByName(const QString & name)
{
    if (Name == name) return this;

    for (int i=0; i<HostedObjects.size(); i++)
    {
        AGeoObject * tmp = HostedObjects[i]->findObjectByName(name);
        if (tmp) return tmp;
    }
    return nullptr; //not found
}

void AGeoObject::findObjectsByWildcard(const QString & name, QVector<AGeoObject*> & foundObjs)
{
    if (Name.startsWith(name, Qt::CaseSensitive)) foundObjs << this;

    for (AGeoObject * obj : HostedObjects)
        obj->findObjectsByWildcard(name, foundObjs);
}

void AGeoObject::changeLineWidthRecursive(int delta)
{
    width += delta;
    if (width < 1) width = 1;

    for (int i=0; i<HostedObjects.size(); i++)
        HostedObjects[i]->changeLineWidthRecursive(delta);
}

bool AGeoObject::isNameExists(const QString &name)
{
  return (findObjectByName(name)) ? true : false;
}

bool AGeoObject::isContainsLocked()
{
  for (int i=0; i<HostedObjects.size(); i++)
    {
      if (HostedObjects[i]->fLocked) return true;
      if (HostedObjects[i]->isContainsLocked()) return true;
    }
  return false;
}

bool AGeoObject::isDisabled() const
{
    if (ObjectType->isWorld()) return false;

    if (!fActive) return true;
    if (!Container) return false;
    return Container->isDisabled();
}

void AGeoObject::enableUp()
{
    if (ObjectType->isWorld()) return;

    fActive = true;
    if (ObjectType->isSlab()) getSlabModel()->fActive = true;

    if (Container) Container->enableUp();
}

bool AGeoObject::isFirstSlab()
{
   if (!ObjectType->isSlab()) return false;
   if (!Container) return false;
   if (!Container->ObjectType->isWorld()) return false;

   for (int i=0; i<Container->HostedObjects.size(); i++)
     {
       AGeoObject* obj = Container->HostedObjects[i];
       if (obj == this) return true;
       if (obj->ObjectType->isSlab()) return false;
     }
   return false;
}

bool AGeoObject::isLastSlab()
{
  if (!ObjectType->isSlab()) return false;
  if (!Container) return false;
  if (!Container->ObjectType->isWorld()) return false;

  for (int i=Container->HostedObjects.size()-1; i>-1; i--)
    {
      AGeoObject* obj = Container->HostedObjects[i];
      if (obj == this) return true;
      if (obj->ObjectType->isSlab()) return false;
    }
  return false;
}

bool AGeoObject::containsUpperLightGuide()
{
    for (int i=0; i<HostedObjects.size(); i++)
    {
        if (HostedObjects[i]->ObjectType->isUpperLightguide())
            return true;
    }
    return false;
}

bool AGeoObject::containsLowerLightGuide()
{
    for (int i=HostedObjects.size()-1; i>-1; i--)
    {
        if (HostedObjects[i]->ObjectType->isLowerLightguide())
            return true;
    }
    return false;
}

AGeoObject *AGeoObject::getFirstSlab()
{
    for (int i=0; i<HostedObjects.size(); i++)
    {
        if (HostedObjects[i]->ObjectType->isSlab())
            return HostedObjects[i];
    }
    return 0;
}

AGeoObject *AGeoObject::getLastSlab()
{
    for (int i=HostedObjects.size()-1; i>-1; i--)
    {
        if (HostedObjects[i]->ObjectType->isSlab())
            return HostedObjects[i];
    }
    return 0;
}

int AGeoObject::getIndexOfFirstSlab()
{
    for (int i=0; i<HostedObjects.size(); i++)
    {
        if (HostedObjects[i]->ObjectType->isSlab())
            return i;
    }
    return -1;
}

int AGeoObject::getIndexOfLastSlab()
{
    for (int i=HostedObjects.size()-1; i>-1; i--)
    {
        if (HostedObjects[i]->ObjectType->isSlab())
            return i;
    }
    return -1;
}

void AGeoObject::addObjectFirst(AGeoObject *Object)
{
  int index = 0;
  if (getContainerWithLogical()) index++;
  if (getGridElement()) index++;
  HostedObjects.insert(index, Object);
  Object->Container = this;
}

void AGeoObject::addObjectLast(AGeoObject *Object)
{
  Object->Container = this;

  if (this->ObjectType->isWorld())
  {
      //put before slabs
      int i=0;
      for (;i<HostedObjects.size(); i++)
          if (HostedObjects.at(i)->ObjectType->isSlab()) break;
      HostedObjects.insert(i, Object);
  }
  else HostedObjects.append(Object);
}

bool AGeoObject::migrateTo(AGeoObject *objTo)
{   
  if (objTo == this->Container) return true;

  //check: cannot migrate down the chain (including to itself)
  //assuming nobody asks to migrate slabs and world
  if ( !objTo->ObjectType->isSlab() && !objTo->ObjectType->isWorld())
    {
      AGeoObject* obj = objTo;
      do
        {
          if (obj == this) return false;
          obj = obj->Container;
        }
      while ( !obj->ObjectType->isSlab() && !obj->ObjectType->isWorld());
    }

  Container->HostedObjects.removeOne(this);  

  //Container = objTo;
  //objTo->HostedObjects.insert(0, this);
  objTo->addObjectFirst(this);
  return true;
}

bool AGeoObject::repositionInHosted(AGeoObject *objTo, bool fAfter)
{
  if (!objTo) return false;
  if (Container != objTo->Container) return false;
  int iTo = Container->HostedObjects.indexOf(objTo);
  int iThis = Container->HostedObjects.indexOf(this);
  if (iTo==-1 || iThis ==-1) return false;

  if (fAfter) iTo++;

  Container->HostedObjects.removeOne(this);
  if (iThis<iTo) iTo--;
  Container->HostedObjects.insert(iTo, this);
  return true;
}

bool AGeoObject::suicide(bool SlabsToo)
{
  if (fLocked) return false;
  if (ObjectType->isWorld()) return false; //cannot delete world
  if (ObjectType->isHandlingStatic() && !SlabsToo) return false;
  if (ObjectType->isSlab() && getSlabModel()->fCenter) return false; //cannot kill center slab

  //cannot remove logicals used by composite (and the logicals container itself); the composite kills itself!
  if (ObjectType->isCompositeContainer()) return false;
  if (isInUseByComposite()) return false;
  //cannot remove grid elementary - it is deleted when grid bulk is deleted
  if (ObjectType->isGridElement()) return false;

  //qDebug() << "!!--Suicide triggered for object:"<<Name;
  AGeoObject* ObjectContainer = Container;
  int index = Container->HostedObjects.indexOf(this);
  if (index == -1) return false;

  Container->HostedObjects.removeAt(index);

  //for composite, clear all unused logicals then kill the composite container
  if (ObjectType->isComposite())
    {
      AGeoObject* cl = this->getContainerWithLogical();
      for (int i=0; i<cl->HostedObjects.size(); i++)
        delete cl->HostedObjects[i];
      delete cl;
      HostedObjects.removeAll(cl);
    }
  if (ObjectType->isGrid())
  {
      AGeoObject* ge = getGridElement();
      if (ge)
      {
          for (int i=0; i<ge->HostedObjects.size(); i++)
              delete ge->HostedObjects[i];
          ge->HostedObjects.clear();

          delete ge;
          HostedObjects.removeAll(ge);
      }
  }

  for (int i=0; i<HostedObjects.size(); i++)
    {
      HostedObjects[i]->Container = ObjectContainer;
      Container->HostedObjects.insert(index, HostedObjects[i]);
      index++;
    }

  delete this;
  return true;
}

void AGeoObject::recursiveSuicide()
{
  for (int i=HostedObjects.size()-1; i>-1; i--)
    HostedObjects[i]->recursiveSuicide();

  //note that the current paradigm is that an unlocked object cannot have locked objects inside
  suicide();
}

void AGeoObject::lockUpTheChain()
{  
  if (ObjectType->isWorld()) return;

  fLocked = true;
  lockBuddies();
  Container->lockUpTheChain();
}

void AGeoObject::lockBuddies()
{
    if (!Container) return;

    if (isCompositeMemeber())
    {
        for (int i=0; i<Container->HostedObjects.size(); i++)
            Container->HostedObjects[i]->fLocked = true;
        if (Container->Container)
            Container->Container->fLocked = true;
    }
    else if (Container->ObjectType->isHandlingSet())
    {
        for (int i=0; i<Container->HostedObjects.size(); i++)
            Container->HostedObjects[i]->fLocked = true;
    }
}

void AGeoObject::lockRecursively()
{
  fLocked = true;
  lockBuddies();
  //if it is a grouped object, lock group buddies  ***!!!

  //for individual:
  for (int i=0; i<HostedObjects.size(); i++)
    HostedObjects[i]->lockRecursively();
}

void AGeoObject::unlockAllInside()
{
  fLocked = false;
  //if it is a grouped object, unlock group buddies  ***!!!

  //for individual:
  for (int i=0; i<HostedObjects.size(); i++)
    HostedObjects[i]->unlockAllInside();
}

void AGeoObject::updateStack()
{
  qDebug() << "Stack update triggered for:"<<Name;
  double Z = Position[2];
  double Edge = 0;
  for (int i=0; i<Container->HostedObjects.size(); i++)
    {
      AGeoObject* o = Container->HostedObjects[i];
      //qDebug() << " --"<<o->Name;

      o->Orientation[0] = o->Orientation[1] = 0;
      o->Orientation[2] = Orientation[2];

      o->Position[0] = Position[0];
      o->Position[1] = Position[1];

      o->Position[2] = Edge - o->Shape->getHeight();
      Edge -= 2.0*o->Shape->getHeight();
    }

  //shift in Z
  double dZ = Z - Position[2];
  for (int i=0; i<Container->HostedObjects.size(); i++)
    {
      AGeoObject* o = Container->HostedObjects[i];
      o->Position[2] += dZ;
  }
}

AGeoObject * AGeoObject::findContainerUp(const QString & name)
{
  //qDebug() << "Looking for:"<<name<<"  Now in:"<<Name;
  if (Name == name) return this;
  //qDebug() << Container;
  if (!Container) return nullptr;
  return Container->findContainerUp(name);
}

void AGeoObject::clearAll()
{
    for (int i=0; i<HostedObjects.size(); i++)
      HostedObjects[i]->clearAll();

    delete this;
}

void AGeoObject::updateWorldSize(double &XYm, double &Zm)
{    
    if (!isWorld())
    {
        double mp = std::max(fabs(Position[0]),fabs(Position[1]));
        mp = std::max(fabs(mp), fabs(Position[2]));
        mp += Shape->maxSize();
        XYm = std::max(mp, XYm);
        Zm = std::max(mp, Zm);
    }

    if (ObjectType->isArray())
      {
        ATypeArrayObject* a = static_cast<ATypeArrayObject*>(this->ObjectType);
        Zm = std::max(Zm, fabs(Position[2]) +  a->stepZ * (a->numZ + 1));

        double X = a->stepX * (1 + a->numX);
        double Y = a->stepY * (1 + a->numY);
        double XY = std::sqrt(X*X + Y*Y);
        XYm = std::max(XYm, std::max(fabs(Position[0]), fabs(Position[1])) + XY);
      }

    for (int i=0; i<HostedObjects.size(); i++)
        HostedObjects[i]->updateWorldSize(XYm, Zm);
}

bool AGeoObject::isMaterialInUse(int imat) const
{
    //qDebug() << Name << "--->"<<Material;

    if (ObjectType->isMonitor()) return false; //monitors are always made of Container's material and cannot host objects

    //if (ObjectType->isGridElement()) qDebug() << "----Grid element!";
    //if (ObjectType->isCompositeContainer()) qDebug() << "----Composite container!";

    if (Material == imat)
    {
        if ( !ObjectType->isGridElement() && !ObjectType->isCompositeContainer() )
            return true;
    }

    for (int i=0; i<HostedObjects.size(); i++)
        if (HostedObjects[i]->isMaterialInUse(imat)) return true;

    return false;
}

bool AGeoObject::isMaterialInActiveUse(int imat) const
{
    if (!fActive) return false;

    if (ObjectType->isMonitor()) return false; //monitors are always made of Container's material and cannot host objects
    if (Material == imat)
    {
        if ( !ObjectType->isGridElement() && !ObjectType->isCompositeContainer() )
            return true;
    }

    for (int i=0; i<HostedObjects.size(); i++)
        if (HostedObjects[i]->isMaterialInActiveUse(imat)) return true;

    return false;
}

void AGeoObject::collectContainingObjects(QVector<AGeoObject *> & vec) const
{
    for (AGeoObject * obj : HostedObjects)
    {
        vec << obj;
        obj->collectContainingObjects(vec);
    }
}

QString randomString(int lettLength, int numLength)
{
  //const QString possibleLett("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  const QString possibleLett("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  const QString possibleNum("0123456789");


  QString randomString;
  for(int i=0; i<lettLength; i++)
    {
      int index = qrand() % possibleLett.length();
      QChar nextChar = possibleLett.at(index);
      randomString.append(nextChar);
    }
  for(int i=0; i<numLength; i++)
    {
      int index = qrand() % possibleNum.length();
      QChar nextChar = possibleNum.at(index);
      randomString.append(nextChar);
    }
  return randomString;
}

QString AGeoObject::GenerateRandomName()
{
    return randomString(2, 1);
}

QString AGeoObject::GenerateRandomObjectName()
{
  QString str = randomString(2, 1);
  str = "New_" + str;
  return str;
}

QString AGeoObject::GenerateRandomLightguideName()
{
  QString str = randomString(0, 1);
  str = "LG" + str;
  return str;
}

QString AGeoObject::GenerateRandomCompositeName()
{
    QString str = randomString(2, 1);
    str = "Composite_" + str;
    return str;
}

QString AGeoObject::GenerateRandomArrayName()
{
  QString str = randomString(1, 1);
  str = "Array_" + str;
  return str;
}

QString AGeoObject::GenerateRandomGridName()
{
  QString str = randomString(1, 1);
  str = "Grid_" + str;
  return str;
}

QString AGeoObject::GenerateRandomMaskName()
{
    QString str = randomString(1, 1);
    str = "Mask_" + str;
    return str;
}

QString AGeoObject::GenerateRandomGuideName()
{
    QString str = randomString(1, 1);
    str = "Guide_" + str;
    return str;
}

QString AGeoObject::GenerateRandomGroupName()
{
  QString str = randomString(2, 1);
  str = "Group_" + str;
  return str;
}

QString AGeoObject::GenerateRandomStackName()
{
  QString str = randomString(2, 1);
  str = "Stack_" + str;
  return str;
}

QString AGeoObject::GenerateRandomMonitorName()
{
  QString str = randomString(2, 1);
  str = "Monitor_" + str;
  return str;
}
