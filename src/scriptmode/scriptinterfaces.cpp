#include "scriptinterfaces.h"
#include "amaterialparticlecolection.h"
#include "ageoobject.h"
#include "slab.h"
#include "asandwich.h"
#include "detectorclass.h"
#include "afiletools.h"
#include "ascriptmanager.h"

#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include <QVector3D>
#include <QDir>
#include <QJsonDocument>

CoreInterfaceClass::CoreInterfaceClass(AScriptManager* ScriptManager) :
    ScriptManager(ScriptManager)
{
  Description = "Unit handles general-purpose opeartions: abort script, basic text output and file save/load";

  H["str"] = "Converts double value to string with a given precision";
  H["print"] = "Print the argument (string) on the script output text field";
  H["clearText"] = "Clear the script output text field";
  H["abort"] = "Abort skript execution.\nOptional string argument is a message to be shown on the script output text field";
  H["save"] = "Add string (second argument) to the file with the name given by the first argument.\n"              
              "Save is not performed (and false is returned) if the file does not exist\n"
              "It is a very slow method!\n"
              "Use \"<br>\" or \"\\n\" to add a line break.\n"
              "For Windows users: pathes have to use \"/\" character, e.g. c:/tmp/file.txt\n";
  H["saveArray"] = "Appends an array (or array of arrays) with numeric data to the file.\n"
                   "Save is not performed (and false is returned) if the file does not exist.\n"
                   "For Windows users: pathes have to use \"/\" character, e.g. c:/tmp/file.txt\n";
  H["createFile"] = "Create new or clear an existent file.\n"
                    "The file is NOT rewritten if the second argument is true (or absent) and the file already exists\n"
                    "For Windows users: pathes have to use \"/\" character, e.g. c:/tmp/file.txt\n";
  H["isFileExists"] = "Return true if file exists";
  H["loadColumn"] = "Load a column with ascii numeric data from the file.\nSecond argument is the column number starting from 0.";
  H["loadArray"] = "Load an array of numerics (or an array of numeric arrays if columns>1).\nColumns parameter can be from 1 to 3.";

}

void CoreInterfaceClass::abort(QString message)
{
  qDebug() << "In-script abort triggered!";
  ScriptManager->AbortEvaluation(message);
}

void CoreInterfaceClass::sleep(int ms)
{
  if (ms == 0) return;
  QTime t;
  t.restart();
  do qApp->processEvents();
  while (t.elapsed()<ms);
}

void CoreInterfaceClass::print(QString text)
{ 
    emit ScriptManager->showMessage(text);
}

void CoreInterfaceClass::clearText()
{
    emit ScriptManager->clearText();
}

QString CoreInterfaceClass::str(double value, int precision)
{
    return QString::number(value, 'g', precision);
}

QString CoreInterfaceClass::GetTimeStamp()
{
    return QDateTime::currentDateTime().toString("H:m:s");
}

QString CoreInterfaceClass::GetDateTimeStamp()
{
    return QDateTime::currentDateTime().toString("d/M/yyyy H:m:s");
}

bool CoreInterfaceClass::save(QString fileName, QString str)
{
  if (!QFileInfo(fileName).exists())
    {
      //abort("File does not exist: " + fileName);
      qDebug() << "File does not exist: " << fileName;
      return false;
    }

  QFile file(fileName);
  if ( !file.open(QIODevice::Append ) )
  {
      //abort("Cannot open file for appending:" + fileName);
      qDebug() << "Cannot open file for appending:" << fileName;
      return false;
  }

  //qDebug() << str;
  str.replace("<br>", "\n");
  QTextStream outstream(&file);
  outstream << str;
  return true;
}

bool CoreInterfaceClass::saveArray(QString fileName, QVariant array)
{
    QString type = array.typeName();
    if (type != "QVariantList")
    {
        qDebug() << "Cannot extract array in saveColumns function";
        //abort("Cannot extract array in saveColumns function");
        return false;
    }

    if (!QFileInfo(fileName).exists())
      {
        //abort("File does not exist: " + fileName);
        qDebug() << "File does not exist: " << fileName;
        return false;
      }

    QFile file(fileName);
    if ( !file.open(QIODevice::Append ) )
    {
        //abort("Cannot open file for appending:" + fileName);
        qDebug() << "Cannot open file for appending:" << fileName;
        return false;
    }

    QTextStream s(&file);

    QVariantList vl = array.toList();
    QJsonArray ar = QJsonArray::fromVariantList(vl);
    //qDebug() << ar.size();

    for (int i=0; i<ar.size(); i++)
    {
        //qDebug() << ar[i];
        if (ar[i].isArray())
        {
            QJsonArray el = ar[i].toArray();
            //qDebug() << "   "<<el;
            for (int j=0; j<el.size(); j++) s << el[j].toDouble(1e10) << " ";
        }
        else
        {
            //qDebug() << "   "<< ar[i].toDouble();
            s << ar[i].toDouble(1e10);
        }
        s << "\n";
    }

    return true;
}

bool CoreInterfaceClass::saveObject(QString FileName, QVariant Object, bool CanOverride)
{
    QString type = Object.typeName();
    if (type != "QVariantMap")
    {
        qDebug() << "Not an object - cannt use saveObject function";
        //abort("Cannot extract array in saveColumns function");
        return false;
    }

    if (QFileInfo(FileName).exists() && !CanOverride)
    {
        //abort("File already exists: " + fileName);
        qDebug() << "File already exists: " << FileName << " Skipping!";
        return false;
    }

    QVariantMap mp = Object.toMap();
    QJsonObject json = QJsonObject::fromVariantMap(mp);
    QJsonDocument saveDoc(json);

    QFile saveFile(FileName);
    if (saveFile.open(QIODevice::WriteOnly))
    {
        saveFile.write(saveDoc.toJson());
        saveFile.close();
    }
    else
    {
        qDebug() << "Cannot open file for writing: " << FileName;
        return false;
    }
    return true;
}

QVariant CoreInterfaceClass::loadColumn(QString fileName, int column)
{
  if (column<0 || column>2)
    {
      abort ("Supported loadColumn with column # 0, 1 and 2");
      return QVariant();
    }

  if (!QFileInfo(fileName).exists())
  {
    //abort("File does not exist: " + fileName);
    qWarning() << "File does not exist: " << fileName;
    return QVariant();
  }

  QVector<double> v1, v2, v3;
  int res;
  if (column == 0)
     res = LoadDoubleVectorsFromFile(fileName, &v1);
  else if (column == 1)
     res = LoadDoubleVectorsFromFile(fileName, &v1, &v2);
  else if (column == 2)
     res = LoadDoubleVectorsFromFile(fileName, &v1, &v2, &v3);

  if (res != 0)
      {
        abort("Error reading from file: "+fileName);
        return QVariant();
      }
  QList<QVariant> l;
  for (int i=0; i<v1.size(); i++)
    {
      if (column == 0) l.append(v1[i]);
      else if (column == 1) l.append(v2[i]);
      else if (column == 2) l.append(v3[i]);
    }

  return l;
}

QVariant CoreInterfaceClass::loadArray(QString fileName, int columns)
{
  if (columns<0 || columns>2)
    {
      abort ("Supported 1, 2 and 3 columns");
      return QVariant();
    }

  if (!QFileInfo(fileName).exists())
  {
    //abort("File does not exist: " + fileName);
    qWarning() << "File does not exist: " << fileName;
    return QVariant();
  }

  QVector<double> v1, v2, v3;
  int res;
  if (columns == 1)
     res = LoadDoubleVectorsFromFile(fileName, &v1);
  else if (columns == 2)
     res = LoadDoubleVectorsFromFile(fileName, &v1, &v2);
  else if (columns == 3)
     res = LoadDoubleVectorsFromFile(fileName, &v1, &v2, &v3);

  if (res != 0)
      {
        abort("Error reading from file: "+fileName);
        return QVariant();
      }

  QList< QVariant > l;
  for (int i=0; i<v1.size(); i++)
    {
      QList<QVariant> ll;
      ll.append(v1[i]);
      if (columns > 1) ll.append(v2[i]);
      if (columns == 3) ll.append(v3[i]);

      QVariant r = ll;
      l << r;
    }
  return l;
}

QString CoreInterfaceClass::GetWorkDir()
{
  return ScriptManager->LastOpenDir;
}

QString CoreInterfaceClass::GetScriptDir()
{
  return ScriptManager->LibScripts;
}

QString CoreInterfaceClass::GetExamplesDir()
{
  return ScriptManager->ExamplesDir;
}

bool CoreInterfaceClass::createFile(QString fileName, bool AbortIfExists)
{
  if (QFileInfo(fileName).exists())
    {
      if (AbortIfExists)
        {
          //abort("File already exists: " + fileName);
          qDebug() << "File already exists: " << fileName;
          return false;
        }
    }

  //create or clear content of the file
  QFile file(fileName);
  if ( !file.open(QIODevice::WriteOnly) )
  {
      //abort("Cannot open file: "+fileName);
      qDebug() << "Cannot open file: " << fileName;
      return false;
  }
  return true;
}

bool CoreInterfaceClass::isFileExists(QString fileName)
{
    return QFileInfo(fileName).exists();
}

bool CoreInterfaceClass::deleteFile(QString fileName)
{
    return QFile(fileName).remove();
}

bool CoreInterfaceClass::createDir(QString path)
{
    QDir dir(path);
    return dir.mkdir(".");
}

QString CoreInterfaceClass::getCurrentDir()
{
    return QDir::currentPath();
}

bool CoreInterfaceClass::setCirrentDir(QString path)
{
    return QDir::setCurrent(path);
}

#ifdef GUI
//--------------------------------------------------------------
InterfaceToAddObjScript::InterfaceToAddObjScript(DetectorClass* Detector)
  : Detector(Detector)
{
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

  H["UpdateGeometry"] = "Updates geometry and do optional check for geometry definition errors.\n"
                        "It is performed automatically for the script called from Advanced Settings window.";

  H["Stack"] = "Adds a stack object. It's container and position is defined by the first object placed in the stack.\n"
               "An array with object names can be provided directly, or objects can be added later with AddToStack function.";

  H["AddToStack"] = "Adds an object or a list of objects to the stack (must be defined with Stack function).";
  H["RecalculateStack"] = "Recalculates Z positions. Has to be called if config.Replace() was used to change thickness of members.";
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

void InterfaceToAddObjScript::Stack(QString StackName, QVariant Members)
{
  QStringList list = Members.toStringList();
  list.insert(0, StackName);
  Stacks << list;
    //qDebug() << list << Stacks.size();
}

void InterfaceToAddObjScript::Stack(QString StackName)
{
  QStringList list;
  list << StackName;
  Stacks << list;
}

void InterfaceToAddObjScript::AddToStack(QString name, QString StackName)
{
  for (int i=0; i<Stacks.size(); i++)
    if (Stacks.at(i).first() == StackName)
      {
        Stacks[i] << name;
        return;
      }
}

void InterfaceToAddObjScript::AddToStack(QVariant names, QString StackName)
{
  QStringList list = names.toStringList();

  for (int i=0; i<Stacks.size(); i++)
    if (Stacks.at(i).first() == StackName)
      {
        Stacks[i] << list;
        return;
      }
}

void InterfaceToAddObjScript::RecalculateStack(QString StackName)
{
    //qDebug() << "Update stack triggered";
  AGeoObject* obj = Detector->Sandwich->World->findObjectByName(StackName);
    //qDebug() << "stack obj:"<<obj;
  if (obj)
    {
        //qDebug() << "Obj:"<<obj->Name <<"Hosted objects:"<<obj->HostedObjects.size();
      if (!obj->HostedObjects.isEmpty())
        obj->HostedObjects[0]->updateStack();
    }
  Detector->BuildDetector_CallFromScript();
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

//--------------------------------------------------------------

void InterfaceToAddObjScript::UpdateGeometry(bool CheckOverlaps)
{
  //checkup    
  for (int i=0; i<GeoObjects.size(); i++)
    {
      const QString name = GeoObjects.at(i)->Name;
      qDebug() << "Checking"<<name;
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

  //checking Stacks
  for (int iStack=0; iStack<Stacks.size(); iStack++)
  {
      if (Stacks.at(iStack).size()<2)
        {
          clearGeoObjects();
          abort("Stack should contain at least one object!");
          return;
        }

      //stack name should be unique
      const QString name = Stacks.at(iStack).first();
      if (Detector->Sandwich->World->isNameExists(name))
        {
          clearGeoObjects();
          abort("Add geo object: Name already exists in detector geometry: "+name);
          return;
        }
      for (int j=0; j<GeoObjects.size(); j++)
        {
          if (name == GeoObjects.at(j)->Name)
            {
              clearGeoObjects();
              abort("Add geo object: At least two objects have the same name: "+name);
              return;
            }
        }
      for (int j=0; j<Stacks.size(); j++)
        {
          if (j == iStack) continue;
          if (name == Stacks.at(j).first())
            {
              clearGeoObjects();
              abort("Stack with this name already defined: "+name);
              return;
            }
        }

      //names should be defined in World or in list of objects to add
      for (int iObjName=1; iObjName<Stacks.at(iStack).size(); iObjName++)
        {
          QString name = Stacks.at(iStack).at(iObjName);
          if (Detector->Sandwich->World->isNameExists(name)) continue;
          bool fFound = false;
          for (int j=0; j<GeoObjects.size(); j++)
             if (name == GeoObjects.at(j)->Name) fFound = true;
          if (fFound) continue;

          clearGeoObjects();
          abort("Add stack: Object not found: "+name);
          return;
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

  //adding stacks
  for (int iStack=0; iStack<Stacks.size(); iStack++)
  {
     AGeoObject* stackObj = new AGeoObject();

     delete stackObj->ObjectType;
     stackObj->ObjectType = new ATypeStackContainerObject();
     stackObj->Name = Stacks.at(iStack).first();

     QStringList ObjNames = Stacks.at(iStack);
     ObjNames.removeFirst();

     QString FirstName = ObjNames.first(); //stack will be inside the container of the first object
     AGeoObject* firstObj = Detector->Sandwich->World->findObjectByName(FirstName);
     if (!firstObj)
       {
         qCritical() << "Object not found:"<<FirstName;
         exit(666);
       }
     AGeoObject* contObj = firstObj->Container;
     stackObj->Container = contObj;
     qDebug() << "--Stack will be hosted by:"<<stackObj->Container->Name;

     for (int i=0; i<ObjNames.size(); i++)
       {
         QString name = ObjNames.at(i);
         AGeoObject* obj = Detector->Sandwich->World->findObjectByName(name);
         if (!obj) continue;
         if (obj->ObjectType->isSlab()) continue;

         obj->Container->HostedObjects.removeOne(obj);
         obj->Container = stackObj;
         stackObj->HostedObjects.append(obj);
       }
     contObj->HostedObjects.insert(0, stackObj);

     firstObj->updateStack();
  }
  Stacks.clear();

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
    Stacks.clear();
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
