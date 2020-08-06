#include "ageoshape.h"
#include "ajsontools.h"

#include <QDebug>

#include "TGeoBBox.h"
#include "TGeoShape.h"
#include "TGeoPara.h"
#include "TGeoSphere.h"
#include "TGeoTube.h"
#include "TGeoTrd1.h"
#include "TGeoTrd2.h"
#include "TGeoPgon.h"
#include "TGeoCone.h"
#include "TGeoParaboloid.h"
#include "TGeoEltu.h"
#include "TGeoArb8.h"
#include "TGeoCompositeShape.h"
#include "TGeoScaledShape.h"
#include "TGeoMatrix.h"
#include "TGeoTorus.h"
#include "ageoconsts.h"

AGeoShape * AGeoShape::clone() const
{
    AGeoShape * sh = AGeoShape::GeoShapeFactory(getShapeType());
    if (!sh)
    {
        qWarning() << "Failed to clone AGeoShape";
        return nullptr;
    }
    QJsonObject json;
    writeToJson(json);
    sh->readFromJson(json);
    return sh;
}

bool AGeoShape::extractParametersFromString(QString GenerationString, QStringList &parameters, int numParameters)
{
    GenerationString = GenerationString.simplified();
    if (!GenerationString.startsWith(getShapeType()))
    {
        qWarning() << "Attempt to generate shape using wrong type!";
        return false;
    }
    GenerationString.remove(getShapeType());
    GenerationString = GenerationString.simplified();
    if (GenerationString.endsWith("\\)"))
    {
        qWarning() << "Format error in shape read from string";
        return false;
    }
    GenerationString.chop(1);
    if (GenerationString.startsWith("\\("))
    {
        qWarning() << "Format error in shape read from string";
        return false;
    }
    GenerationString.remove(0, 1);
    GenerationString.remove(QString(" "));

    parameters = GenerationString.split(",", QString::SkipEmptyParts);

    if (numParameters == -1) return true;

    if (parameters.size() != numParameters)
    {
        qWarning() << "Wrong number of parameters!";
        return false;
    }
    return true;
}

// ====== BOX ======
bool AGeoBox::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 3);
    if (!ok) return false;

    double tmp[3];
    for (int i=0; i<3; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoBox";
            return false;
        }
    }

    dx = tmp[0];
    dy = tmp[1];
    dz = tmp[2];
    return true;
}

const QString AGeoBox::getHelp()
{
    return "A box has 3 parameters: dx, dy, dz representing the half-lengths on X, Y and Z axes.\n"
           "The box will range from: -dx to dx on X-axis, from -dy to dy on Y and from -dz to dz on Z.";
}

QString AGeoBox::updateShape()
{
    QString errorStr = updateParameter(str2dx, dx);
    if (!errorStr.isEmpty()) return errorStr;

    errorStr = updateParameter(str2dy, dy);
    if (!errorStr.isEmpty()) return errorStr;

    errorStr = updateParameter(str2dz, dz);
    return errorStr;
}

const QString AGeoBox::getGenerationString() const
{
    QString str = "TGeoBBox( " +
            QString::number(dx)+", "+
            QString::number(dy)+", "+
            QString::number(dz)+" )";
    return str;
}

double AGeoBox::maxSize()
{
    double m = std::max(dx, dy);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

double AGeoBox::minSize()
{
    return std::min(dx, dy);
}

TGeoShape *AGeoBox::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoBBox(dx, dy, dz) : new TGeoBBox(shapeName.toLatin1().data(), dx, dy, dz);
}

void AGeoBox::writeToJson(QJsonObject &json) const
{
    json["dx"] = dx;
    json["dy"] = dy;
    json["dz"] = dz;

    if (!str2dx.isEmpty()) json["str2dx"] = str2dx;
    if (!str2dy.isEmpty()) json["str2dy"] = str2dy;
    if (!str2dz.isEmpty()) json["str2dz"] = str2dz;
}

void AGeoBox::readFromJson(QJsonObject &json)
{
    dx = json["dx"].toDouble();
    dy = json["dy"].toDouble();
    dz = json["dz"].toDouble();

    if (!parseJson(json, "str2dx", str2dx)) str2dx.clear();
    if (!parseJson(json, "str2dy", str2dy)) str2dy.clear();
    if (!parseJson(json, "str2dz", str2dz)) str2dz.clear();
}

bool AGeoBox::readFromTShape(TGeoShape *Tshape)
{
    TGeoBBox* Tbox = dynamic_cast<TGeoBBox*>(Tshape);
    if (!Tbox) return false;

    dx = Tbox->GetDX();
    dy = Tbox->GetDY();
    dz = Tbox->GetDZ();

    return true;
}

/*
AGeoShape * AGeoBox::clone() const
{
    return new AGeoBox(dx, dy, dz);
}
*/

// ====== PARA ======
const QString AGeoPara::getHelp()
{
    return "A parallelepiped is a shape having 3 pairs of parallel faces out of which one is parallel with the XY plane (Z"
           " faces). All faces are parallelograms in the general case. The Z faces have 2 edges parallel with the X-axis.\n"
           "The shape has the center in the origin and it is defined by:\n"
           " • dX, dY, dZ: half-lengths of the projections of the edges on X, Y and Z. The lower Z face is "
           "positioned at -dZ, while the upper at +dZ.\n"
           " • alpha: angle between the segment defined by the centers of the X-parallel edges and Y axis [-90,90] in degrees\n"
           " • theta: theta angle of the segment defined by the centers of the Z faces;\n"
           " • phi: phi angle of the same segment";
}

bool AGeoPara::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 6);
    if (!ok) return false;

    double tmp[6];
    for (int i=0; i<6; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoPara";
            return false;
        }
    }

    dx = tmp[0];
    dy = tmp[1];
    dz = tmp[2];
    alpha = tmp[3];
    theta = tmp[4];
    phi = tmp[5];
    return true;
}

TGeoShape *AGeoPara::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoPara(dx, dy, dz, alpha, theta, phi) : new TGeoPara(shapeName.toLatin1().data(), dx, dy, dz, alpha, theta, phi);
}

const QString AGeoPara::getGenerationString() const
{
    QString str = "TGeoPara( " +
            QString::number(dx)+", "+
            QString::number(dy)+", "+
            QString::number(dz)+", "+
            QString::number(alpha)+", "+
            QString::number(theta)+", "+
            QString::number(phi)+" )";
    return str;
}

double AGeoPara::maxSize()
{
    double m = std::max(dx, dy);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

void AGeoPara::writeToJson(QJsonObject &json) const
{
    json["dx"] = dx;
    json["dy"] = dy;
    json["dz"] = dz;
    json["alpha"] = alpha;
    json["theta"] = theta;
    json["phi"] = phi;
}

void AGeoPara::readFromJson(QJsonObject &json)
{
    dx = json["dx"].toDouble();
    dy = json["dy"].toDouble();
    dz = json["dz"].toDouble();
    alpha = json["alpha"].toDouble();
    theta = json["theta"].toDouble();
    phi = json["phi"].toDouble();
}

bool AGeoPara::readFromTShape(TGeoShape *Tshape)
{
    TGeoPara* p = dynamic_cast<TGeoPara*>(Tshape);
    if (!p) return false;

    dx = p->GetDX();
    dy = p->GetDY();
    dz = p->GetDZ();
    alpha = p->GetAlpha();
    theta = p->GetTheta();
    phi = p->GetPhi();

    return true;
}

AGeoComposite::AGeoComposite(const QStringList members, const QString GenerationString) :
    members(members), GenerationString(GenerationString)
{
    //qDebug() << "new composite!";
    //qDebug() << members;
    //qDebug() << GenerationString;
}

const QString AGeoComposite::getHelp()
{
    return "Composite shapes are Boolean combinations of two or more shape components. The supported Boolean "
           "operations are union (+), intersection (*) and subtraction(-).";
}

bool AGeoComposite::readFromString(QString GenerationString)
{
    //qDebug() << "Number of defined members:"<<members.size();
    QString s = GenerationString.simplified();

    s.remove("TGeoCompositeShape");
    s.remove("(");
    s.remove(")");
    QStringList requested = s.split(QRegExp("[+*-]"));
    for (int i=0; i<requested.size(); i++) requested[i] = requested[i].simplified();
    //qDebug() << "Requested members in composite generation:"<<requested;
    QSet<QString> setRequested = QSet<QString>::fromList(requested);
    QSet<QString> setMembers = QSet<QString>::fromList(members);
    //qDebug() << "Members:"<<setMembers;
    //qDebug() << "Requested:"<<setRequested;
    //qDebug() << "Memebers contain requested?"<<setMembers.contains(setRequested);
    if (!setMembers.contains(setRequested)) return false;

    this->GenerationString = GenerationString.simplified();
    return true;
}

TGeoShape *AGeoComposite::createGeoShape(const QString shapeName)
{
    //qDebug() << "---Create TGeoComposite triggered!";
    //qDebug() << "---Members registered:"<<members;
    QString s = GenerationString;
    s.remove("TGeoCompositeShape(");
    s.chop(1);

    //qDebug() << "--->Gen string before:"<<s;
    //qDebug() << "--->Memebers:"<<members;
    for (int i=0; i<members.size(); i++)
    {
        QString mem = members[i];
        QRegExp toReplace("\\b"+mem+"\\b(?!:)");
        QString memMod = mem+":_m"+mem;
        s.replace(toReplace, memMod);
    }
    //qDebug() << "--->Str to be used in composite generation:"<<s;

    return (shapeName.isEmpty()) ? new TGeoCompositeShape(s.toLatin1().data()) : new TGeoCompositeShape(shapeName.toLatin1().data(), s.toLatin1().data());
}

void AGeoComposite::writeToJson(QJsonObject &json) const
{
    json["GenerationString"] = GenerationString;
}

void AGeoComposite::readFromJson(QJsonObject &json)
{
    GenerationString = json["GenerationString"].toString();
}

const QString AGeoSphere::getHelp()
{
    return "TSphere is a spherical sector with the following parameters:\n"
           " • rmin: internal radius\n"
           " • rmax: external radius\n"
           " • theta1: starting theta value [0, 180) in degrees\n"
           " • theta2: ending theta value (0, 180] in degrees (theta1<theta2)\n"
           " • phi1: starting phi value [0, 360) in degrees\n"
           " • phi2: ending phi value (0, 360] in degrees (phi1<phi2)";
}

bool AGeoSphere::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 6);
    if (!ok) return false;

    double tmp[6];
    for (int i=0; i<6; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoSphere";
            return false;
        }
    }

    rmin = tmp[0];
    rmax = tmp[1];
    theta1 = tmp[2];
    theta2 = tmp[3];
    phi1 = tmp[4];
    phi2 = tmp[5];
    return true;
}

TGeoShape *AGeoSphere::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoSphere(rmin, rmax, theta1, theta2, phi1,  phi2) : new TGeoSphere(shapeName.toLatin1().data(), rmin, rmax, theta1, theta2, phi1,  phi2);
}

const QString AGeoSphere::getGenerationString() const
{
    QString str = "TGeoSphere( " +
            QString::number(rmin)+", "+
            QString::number(rmax)+", "+
            QString::number(theta1)+", "+
            QString::number(theta2)+", "+
            QString::number(phi1)+", "+
            QString::number(phi2)+" )";
    return str;
}



void AGeoSphere::writeToJson(QJsonObject &json) const
{
    json["rmin"] = rmin;
    json["rmax"] = rmax;
    json["theta1"] = theta1;
    json["theta2"] = theta2;
    json["phi1"] = phi1;
    json["phi2"] = phi2;
}

void AGeoSphere::readFromJson(QJsonObject &json)
{
    rmin = json["rmin"].toDouble();
    rmax = json["rmax"].toDouble();
    theta1 = json["theta1"].toDouble();
    theta2 = json["theta2"].toDouble();
    phi1 = json["phi1"].toDouble();
    phi2 = json["phi2"].toDouble();
}

bool AGeoSphere::readFromTShape(TGeoShape *Tshape)
{
    TGeoSphere* s = dynamic_cast<TGeoSphere*>(Tshape);
    if (!s) return false;

    rmin = s->GetRmin();
    rmax = s->GetRmax();
    theta1 = s->GetTheta1();
    theta2 = s->GetTheta2();
    phi1 = s->GetPhi1();
    phi2 = s->GetPhi2();

    return true;
}


const QString AGeoTubeSeg::getHelp()
{
    return "A tube segment is a tube having a range in phi. The general phi convention is "
           "that the shape ranges from phi1 to phi2 going counterclockwise. The angles can be defined with either "
           "negative or positive values. They are stored such that phi1 is converted to [0,360] and phi2 > phi1.\n"
           "Tube segments have Z as their symmetry axis. They have a range in Z, a minimum (rmin) and a maximum (rmax) radius.\n"
           "The full Z range is from -dz to +dz.";
}

bool AGeoTubeSeg::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 5);
    if (!ok) return false;

    double tmp[5];
    for (int i=0; i<5; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoTubeSeg";
            return false;
        }
    }

    rmin = tmp[0];
    rmax = tmp[1];
    dz = tmp[2];
    phi1 = tmp[3];
    phi2 = tmp[4];
    return true;
}

TGeoShape *AGeoTubeSeg::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoTubeSeg(rmin, rmax, dz, phi1, phi2) :
                                   new TGeoTubeSeg(shapeName.toLatin1().data(), rmin, rmax, dz, phi1, phi2);
}

const QString AGeoTubeSeg::getGenerationString() const
{
    QString str = "TGeoTubeSeg( " +
            QString::number(rmin)+", "+
            QString::number(rmax)+", "+
            QString::number(dz)+", "+
            QString::number(phi1)+", "+
            QString::number(phi2)+" )";
    return str;
}

double AGeoTubeSeg::maxSize()
{
    double m = std::max(rmax, dz);
    return sqrt(3.0)*m;
}

void AGeoTubeSeg::writeToJson(QJsonObject &json) const
{
    json["rmin"] = rmin;
    json["rmax"] = rmax;
    json["dz"] = dz;
    json["phi1"] = phi1;
    json["phi2"] = phi2;
}

void AGeoTubeSeg::readFromJson(QJsonObject &json)
{
    rmin = json["rmin"].toDouble();
    rmax = json["rmax"].toDouble();
    dz = json["dz"].toDouble();
    phi1 = json["phi1"].toDouble();
    phi2 = json["phi2"].toDouble();
}

bool AGeoTubeSeg::readFromTShape(TGeoShape *Tshape)
{
    TGeoTubeSeg* s = dynamic_cast<TGeoTubeSeg*>(Tshape);
    if (!s) return false;

    rmin = s->GetRmin();
    rmax = s->GetRmax();
    dz = s->GetDz();
    phi1 = s->GetPhi1();
    phi2 = s->GetPhi2();

    return true;
}


const QString AGeoCtub::getHelp()
{
    return "A cut tube is a tube segment cut with two planes. The centers of the 2 sections are positioned at ±dZ. Each cut "
           "plane is therefore defined by a point (0,0,±dZ) and its normal unit vector pointing outside the shape: "
           "Nlow=(Nx,Ny,Nz<0), Nhigh=(Nx’,Ny’,Nz’>0).\n"
           "The general phi convention is that the shape ranges from phi1 to phi2 going counterclockwise. The angles can be defined with either "
           "negative or positive values. They are stored such that phi1 is converted to [0,360] and phi2 > phi1."
           "The shape has a minimum (rmin) and a maximum (rmax) radius.\n";
}

bool AGeoCtub::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 11);
    if (!ok) return false;

    double tmp[11];
    for (int i=0; i<11; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoCtub";
            return false;
        }
    }

    rmin = tmp[0];
    rmax = tmp[1];
    dz =   tmp[2];
    phi1 = tmp[3];
    phi2 = tmp[4];
    nxlow= tmp[5];
    nylow= tmp[6];
    nzlow= tmp[7];
    nxhi = tmp[8];
    nyhi = tmp[9];
    nzhi = tmp[10];
    return true;
}

TGeoShape *AGeoCtub::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoCtub( rmin, rmax, dz, phi1, phi2, nxlow, nylow, nzlow, nxhi, nyhi, nzhi ) :
                                   new TGeoCtub( shapeName.toLatin1().data(), rmin, rmax, dz, phi1, phi2, nxlow, nylow, nzlow, nxhi, nyhi, nzhi );
}

const QString AGeoCtub::getGenerationString() const
{
    QString str = "TGeoCtub( " +
            QString::number(rmin)+", "+
            QString::number(rmax)+", "+
            QString::number(dz)+", "+
            QString::number(phi1)+", "+
            QString::number(phi2)+", "+
            QString::number(nxlow)+", "+
            QString::number(nylow)+", "+
            QString::number(nzlow)+", "+
            QString::number(nxhi)+", "+
            QString::number(nyhi)+", "+
            QString::number(nzhi)+" )";
    return str;
}

double AGeoCtub::maxSize()
{
    double m = std::max(rmax, dz);
    return sqrt(3.0)*m;
}

void AGeoCtub::writeToJson(QJsonObject &json) const
{
    json["rmin"] = rmin;
    json["rmax"] = rmax;
    json["dz"] = dz;
    json["phi1"] = phi1;
    json["phi2"] = phi2;

    json["nxlow"] = nxlow;
    json["nylow"] = nylow;
    json["nzlow"] = nzlow;
    json["nxhi"] = nxhi;
    json["nyhi"] = nyhi;
    json["nzhi"] = nzhi;
}

void AGeoCtub::readFromJson(QJsonObject &json)
{
    rmin = json["rmin"].toDouble();
    rmax = json["rmax"].toDouble();
    dz = json["dz"].toDouble();
    phi1 = json["phi1"].toDouble();
    phi2 = json["phi2"].toDouble();

    nxlow = json["nxlow"].toDouble();
    nylow = json["nylow"].toDouble();
    nzlow = json["nzlow"].toDouble();
    nxhi = json["nxhi"].toDouble();
    nyhi = json["nyhi"].toDouble();
    nzhi = json["nzhi"].toDouble();
}

bool AGeoCtub::readFromTShape(TGeoShape *Tshape)
{
    TGeoCtub* s = dynamic_cast<TGeoCtub*>(Tshape);
    if (!s) return false;

    rmin = s->GetRmin();
    rmax = s->GetRmax();
    dz = s->GetDz();
    phi1 = s->GetPhi1();
    phi2 = s->GetPhi2();
    nxlow = s->GetNlow()[0];
    nylow = s->GetNlow()[1];
    nzlow = s->GetNlow()[2];
    nxhi = s->GetNlow()[0];
    nyhi = s->GetNlow()[1];
    nzhi = s->GetNlow()[2];

    return true;
}

const QString AGeoTube::getHelp()
{
    return "Tubes have Z as their symmetry axis. They have a range in Z, a minimum (rmin) and a maximum (rmax) radius.\n"
           "The full Z range is from -dz to +dz.";
}

QString AGeoTube::updateShape()
{
    QString errorStr = updateParameter(str2rmin, rmin, false);
    if (!errorStr.isEmpty()) return errorStr;

    errorStr = updateParameter(str2rmax, rmax);
    if (!errorStr.isEmpty()) return errorStr;

    if (rmin >= rmax) return "Inside diameter is larger than the outside one!";

    errorStr = updateParameter(str2dz, dz);
    return errorStr;
}

bool AGeoTube::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 3);
    if (!ok) return false;

    double tmp[3];
    for (int i=0; i<3; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoTube";
            return false;
        }
    }

    rmin = tmp[0];
    rmax = tmp[1];
    dz = tmp[2];
    return true;
}

TGeoShape *AGeoTube::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoTube(rmin, rmax, dz) :
                                   new TGeoTube(shapeName.toLatin1().data(), rmin, rmax, dz);
}

const QString AGeoTube::getGenerationString() const
{
    QString str = "TGeoTube( " +
            QString::number(rmin)+", "+
            QString::number(rmax)+", "+
            QString::number(dz)+" )";
    return str;
}

double AGeoTube::maxSize()
{
    double m = std::max(rmax, dz);
    return sqrt(3.0)*m;
}

double AGeoTube::minSize()
{
    return rmax;
}

void AGeoTube::writeToJson(QJsonObject &json) const
{
    json["rmin"] = rmin;
    json["rmax"] = rmax;
    json["dz"]   = dz;

    if (!str2rmin.isEmpty()) json["str2rmin"] = str2rmin;
    if (!str2rmax.isEmpty()) json["str2rmax"] = str2rmax;
    if (!str2dz.isEmpty())   json["str2dz"]   = str2dz;
}

void AGeoTube::readFromJson(QJsonObject &json)
{
    rmin = json["rmin"].toDouble();
    rmax = json["rmax"].toDouble();
    dz   = json["dz"].toDouble();

    if (!parseJson(json, "str2rmin", str2rmin)) str2rmin.clear();
    if (!parseJson(json, "str2rmax", str2rmax)) str2rmax.clear();
    if (!parseJson(json, "str2rdz",  str2dz))   str2dz.clear();
}

bool AGeoTube::readFromTShape(TGeoShape *Tshape)
{
    TGeoTube* s = dynamic_cast<TGeoTube*>(Tshape);
    if (!s) return false;

    rmin = s->GetRmin();
    rmax = s->GetRmax();
    dz = s->GetDz();

    return true;
}

// ---  Trd1  ---
const QString AGeoTrd1::getHelp()
{
    return "Trapezoid with two of the opposite faces parallel to XY plane and positioned at ± dZ\n"
           "Trd1 is a trapezoid with only X varying with Z. It is defined by the half-length in Z, the half-length in X at the "
           "lowest and highest Z planes and the half-length in Y\n"
           " • dx1: half length in X at -dz\n"
           " • dx2: half length in X at +dz\n"
           " • dy: half length in Y\n"
           " • dz: half length in Z\n";
}

bool AGeoTrd1::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 4);
    if (!ok) return false;

    double tmp[4];
    for (int i=0; i<4; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoTrd1";
            return false;
        }
    }

    dx1 = tmp[0];
    dx2 = tmp[1];
    dy = tmp[2];
    dz = tmp[3];
    return true;
}

TGeoShape *AGeoTrd1::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoTrd1(dx1, dx2, dy, dz) :
                                   new TGeoTrd1(shapeName.toLatin1().data(), dx1, dx2, dy, dz);
}

const QString AGeoTrd1::getGenerationString() const
{
    QString str = "TGeoTrd1( " +
            QString::number(dx1)+", "+
            QString::number(dx2)+", "+
            QString::number(dy)+", "+
            QString::number(dz)+" )";
    return str;
}

double AGeoTrd1::maxSize()
{
    double m = std::max(dx1, dx2);
    m = std::max(m, dy);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

void AGeoTrd1::writeToJson(QJsonObject &json) const
{
    json["dx1"] = dx1;
    json["dx2"] = dx2;
    json["dy"] = dy;
    json["dz"] = dz;
}

void AGeoTrd1::readFromJson(QJsonObject &json)
{
    dx1 = json["dx1"].toDouble();
    dx2 = json["dx2"].toDouble();
    dy = json["dy"].toDouble();
    dz = json["dz"].toDouble();
}

bool AGeoTrd1::readFromTShape(TGeoShape *Tshape)
{
    TGeoTrd1* s = dynamic_cast<TGeoTrd1*>(Tshape);
    if (!s) return false;

    dx1 = s->GetDx1();
    dx2 = s->GetDx2();
    dy = s->GetDy();
    dz = s->GetDz();

    return true;
}

// ---  Trd2  ---
const QString AGeoTrd2::getHelp()
{
    return "Trd2 is a trapezoid with both X and Y varying with Z. It is defined by the half-length in Z, the half-length in X at "
           "the lowest and highest Z planes and the half-length in Y at these planes: "
           " • dx1: half length in X at -dz\n"
           " • dx2: half length in X at +dz\n"
           " • dy1: half length in Y at -dz\n"
           " • dy2: half length in Y at +dz\n"
           " • dz: half length in Z\n";
}

bool AGeoTrd2::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 5);
    if (!ok) return false;

    double tmp[5];
    for (int i=0; i<5; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoTrd2";
            return false;
        }
    }
    dx1 = tmp[0];
    dx2 = tmp[1];
    dy1 = tmp[2];
    dy2 = tmp[3];
    dz = tmp[4];
    return true;
}

TGeoShape *AGeoTrd2::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoTrd2(dx1, dx2, dy1, dy2, dz) :
                                   new TGeoTrd2(shapeName.toLatin1().data(), dx1, dx2, dy1, dy2, dz);
}

const QString AGeoTrd2::getGenerationString() const
{
    QString str = "TGeoTrd2( " +
            QString::number(dx1)+", "+
            QString::number(dx2)+", "+
            QString::number(dy1)+", "+
            QString::number(dy2)+", "+
            QString::number(dz)+" )";
    return str;
}

double AGeoTrd2::maxSize()
{
    double m = std::max(dx1, dx2);
    m = std::max(m, dy1);
    m = std::max(m, dy2);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

void AGeoTrd2::writeToJson(QJsonObject &json) const
{
    json["dx1"] = dx1;
    json["dx2"] = dx2;
    json["dy1"] = dy1;
    json["dy2"] = dy2;
    json["dz"] = dz;
}

void AGeoTrd2::readFromJson(QJsonObject &json)
{
    dx1 = json["dx1"].toDouble();
    dx2 = json["dx2"].toDouble();
    dy1 = json["dy1"].toDouble();
    dy2 = json["dy2"].toDouble();
    dz = json["dz"].toDouble();
}

bool AGeoTrd2::readFromTShape(TGeoShape *Tshape)
{
    TGeoTrd2* s = dynamic_cast<TGeoTrd2*>(Tshape);
    if (!s) return false;

    dx1 = s->GetDx1();
    dx2 = s->GetDx2();
    dy1 = s->GetDy1();
    dy2 = s->GetDy2();
    dz = s->GetDz();

    return true;
}

// --- GeoPgon ---
const QString AGeoPgon::getHelp()
{
    return "TGeoPgon:\n"
           "nedges - number of edges\n"
           "phi - start angle [0, 360)\n"
           "dphi - range in angle (0, 360]\n"
           "{z : rmin : rmax} - arbitrary number of sections defined with z position, minimum and maximum radii";
}

bool AGeoPgon::readFromString(QString GenerationString)
{
    Sections.clear();
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, -1);
    if (!ok) return false;

    if (params.size()<5)
    {
        qWarning() << "Not enough parameters to define TGeoPgon";
        return false;
    }

    phi = params[0].toDouble(&ok);
    if (!ok)
    {
        qWarning() << "Syntax error found during extracting parameters of TGeoPgon";
        return false;
    }
    dphi = params[1].toDouble(&ok);
    if (!ok)
    {
        qWarning() << "Syntax error found during extracting parameters of TGeoPgon";
        return false;
    }
    nedges = params[2].toInt(&ok);
    if (!ok)
    {
        qWarning() << "Syntax error found during extracting parameters of TGeoPgon";
        return false;
    }

    for (int i=3; i<params.size(); i++)
    {
        APolyCGsection section;
        if (!section.fromString(params.at(i)))
        {
            qWarning() << "Syntax error found during extracting parameters of TGeoPgon";
            return false;
        }
        Sections << section;
    }

    bool bInc = true;
    for (int i=1; i<Sections.size(); i++)
    {
        if (i == 1)
            if (Sections.first().z > Sections.at(1).z)
                bInc = false;

        if (Sections.at(i).z <= Sections.at(i-1).z && bInc)
        {
            qWarning() << "Non consistent positions of polygon planes";
            return false;
        }

        if (Sections.at(i).z >= Sections.at(i-1).z && !bInc)
        {
            qWarning() << "Non consistent positions of polygon planes";
            return false;
        }
    }

    return true;
}

TGeoShape *AGeoPgon::createGeoShape(const QString shapeName)
{
    TGeoPgon* pg = (shapeName.isEmpty()) ? new TGeoPgon(phi, dphi, nedges, Sections.size()) :
                                           new TGeoPgon(shapeName.toLatin1().data(), phi, dphi, nedges, Sections.size());
    for (int i=0; i<Sections.size(); i++)
    {
        const APolyCGsection& s = Sections.at(i);
        pg->DefineSection(i, s.z, s.rmin, s.rmax);
    }
    return pg;
}

const QString AGeoPgon::getGenerationString() const
{
    QString str = "TGeoPgon( " +
            QString::number(phi)+", "+
            QString::number(dphi) + ", "+
            QString::number(nedges);

    for (const APolyCGsection& s : Sections)  str += ", " + s.toString();

    str +=" )";
    return str;
}

double AGeoPgon::maxSize()
{
    return AGeoPcon::maxSize();
}

void AGeoPgon::writeToJson(QJsonObject &json) const
{
    json["nedges"] = nedges;
    AGeoPcon::writeToJson(json);
}

void AGeoPgon::readFromJson(QJsonObject &json)
{
    nedges = json["nedges"].toInt();
    AGeoPcon::readFromJson(json);
}

bool AGeoPgon::readFromTShape(TGeoShape *Tshape)
{
    TGeoPgon* pg = dynamic_cast<TGeoPgon*>(Tshape);
    if (!pg) return false;

    phi = pg->GetPhi1();
    dphi = pg->GetDphi();
    nedges = pg->GetNedges();

    Sections.clear();
    for (int i=0; i<pg->GetNz(); i++)
        Sections << APolyCGsection(pg->GetZ()[i], pg->GetRmin()[i], pg->GetRmax()[i]);

    //qDebug() << "Pgone loaded from TShape..."<<phi<<dphi<<nedges;
    //for (int i=0; i<Sections.size(); i++) qDebug() << Sections.at(i).toString();

    return true;
}

const QString AGeoConeSeg::getHelp()
{
    return "Cone segment with the following parameters:\n"
           "dz - half size in Z\n"
           "rminL - internal radius at Z-dz\n"
           "rmaxL - external radius at Z-dz\n"
           "rminU - internal radius at Z+dz\n"
           "rmaxU - external radius at Z+dz\n"
           "phi1 - angle [0, 360)\n"
           "phi2 - angle (0, 360]";
}

bool AGeoConeSeg::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 7);
    if (!ok) return false;

    double tmp[7];
    for (int i=0; i<7; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of TGeoConeSeg";
            return false;
        }
    }

    dz =    tmp[0];
    rminL = tmp[1];
    rmaxL = tmp[2];
    rminU = tmp[3];
    rmaxU = tmp[4];
    phi1 =  tmp[5];
    phi2 =  tmp[6];
    return true;
}

TGeoShape *AGeoConeSeg::createGeoShape(const QString shapeName)
{
    TGeoConeSeg* s = (shapeName.isEmpty()) ? new TGeoConeSeg(dz, rminL, rmaxL, rminU, rmaxU, phi1, phi2) :
                                             new TGeoConeSeg(shapeName.toLatin1().data(),
                                                             dz, rminL, rmaxL, rminU, rmaxU, phi1, phi2);
    return s;
}

const QString AGeoConeSeg::getGenerationString() const
{
    QString str = "TGeoConeSeg( " +
            QString::number(dz)+", "+
            QString::number(rminL)+", "+
            QString::number(rmaxL)+", "+
            QString::number(rminU)+", "+
            QString::number(rmaxU)+", "+
            QString::number(phi1)+", "+
            QString::number(phi2)+" )";
    return str;
}

double AGeoConeSeg::maxSize()
{
    double m = std::max(rmaxL, rmaxU);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

void AGeoConeSeg::writeToJson(QJsonObject &json) const
{
    json["dz"] = dz;
    json["rminL"] = rminL;
    json["rmaxL"] = rmaxL;
    json["rminU"] = rminU;
    json["rmaxU"] = rmaxU;
    json["phi1"] = phi1;
    json["phi2"] = phi2;
}

void AGeoConeSeg::readFromJson(QJsonObject &json)
{
    dz = json["dz"].toDouble();
    rminL = json["rminL"].toDouble();
    rmaxL = json["rmaxL"].toDouble();
    rminU = json["rminU"].toDouble();
    rmaxU = json["rmaxU"].toDouble();
    phi1 = json["phi1"].toDouble();
    phi2 = json["phi2"].toDouble();
}

bool AGeoConeSeg::readFromTShape(TGeoShape *Tshape)
{
    TGeoConeSeg* s = dynamic_cast<TGeoConeSeg*>(Tshape);
    if (!s) return false;

    rminL = s->GetRmin1();
    rmaxL = s->GetRmax1();
    rminU = s->GetRmin2();
    rmaxU = s->GetRmax2();
    phi1 = s->GetPhi1();
    phi2 = s->GetPhi2();
    dz = s->GetDz();

    return true;
}

const QString AGeoParaboloid::getHelp()
{
    return  "A paraboloid is defined by the revolution surface generated by a parabola and is bounded by two planes "
            "perpendicular to Z axis. The parabola equation is taken in the form: z = a·r2 + b, where: r2 = x2 + y2. "
            "The coefficients a and b are computed from the input values which are the radii of the circular sections cut by "
            "the planes at +/-dz:\n"
            " • -dz = a·rlo·rlo + b\n"
            " • +dz = a·rhi·rhi + b";
}

bool AGeoParaboloid::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 3);
    if (!ok) return false;

    double tmp[3];
    for (int i=0; i<3; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of TGeoParaboloid";
            return false;
        }
    }

    rlo = tmp[0];
    rhi = tmp[1];
    dz = tmp[2];
    return true;
}

TGeoShape *AGeoParaboloid::createGeoShape(const QString shapeName)
{
    TGeoParaboloid* s = (shapeName.isEmpty()) ? new TGeoParaboloid(rlo, rhi, dz) :
                                                new TGeoParaboloid(shapeName.toLatin1().data(), rlo, rhi, dz);
    return s;
}

const QString AGeoParaboloid::getGenerationString() const
{
    QString str = "TGeoParaboloid( " +
            QString::number(rlo)+", "+
            QString::number(rhi)+", "+
            QString::number(dz)+" )";
    return str;
}

double AGeoParaboloid::maxSize()
{
    double m = std::max(rlo, rhi);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

void AGeoParaboloid::writeToJson(QJsonObject &json) const
{
    json["rlo"] = rlo;
    json["rhi"] = rhi;
    json["dz"] = dz;
}

void AGeoParaboloid::readFromJson(QJsonObject &json)
{
    rlo = json["rlo"].toDouble();
    rhi = json["rhi"].toDouble();
    dz = json["dz"].toDouble();
}

bool AGeoParaboloid::readFromTShape(TGeoShape *Tshape)
{
    TGeoParaboloid* p = dynamic_cast<TGeoParaboloid*>(Tshape);
    if (!p) return false;

    rlo = p->GetRlo();
    rhi = p->GetRhi();
    dz = p->GetDz();

    return true;
}

const QString AGeoCone::getHelp()
{
    return "Cone with the following parameters:\n"
           "dz - half size in Z\n"
           "rminL - internal radius at Z-dz\n"
           "rmaxL - external radius at Z-dz\n"
           "rminU - internal radius at Z+dz\n"
           "rmaxU - external radius at Z+dz";
}

bool AGeoCone::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 5);
    if (!ok) return false;

    double tmp[5];
    for (int i=0; i<5; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of TGeoCone";
            return false;
        }
    }

    dz =    tmp[0];
    rminL = tmp[1];
    rmaxL = tmp[2];
    rminU = tmp[3];
    rmaxU = tmp[4];
    return true;
}

TGeoShape *AGeoCone::createGeoShape(const QString shapeName)
{
    TGeoCone* s = (shapeName.isEmpty()) ? new TGeoCone(dz, rminL, rmaxL, rminU, rmaxU) :
                                          new TGeoCone(shapeName.toLatin1().data(),
                                                       dz, rminL, rmaxL, rminU, rmaxU);
    return s;
}

const QString AGeoCone::getGenerationString() const
{
    QString str = "TGeoCone( " +
            QString::number(dz)+", "+
            QString::number(rminL)+", "+
            QString::number(rmaxL)+", "+
            QString::number(rminU)+", "+
            QString::number(rmaxU)+" )";
    return str;
}

double AGeoCone::maxSize()
{
    double m = std::max(rmaxL, rmaxU);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

void AGeoCone::writeToJson(QJsonObject &json) const
{
    json["dz"] = dz;
    json["rminL"] = rminL;
    json["rmaxL"] = rmaxL;
    json["rminU"] = rminU;
    json["rmaxU"] = rmaxU;
}

void AGeoCone::readFromJson(QJsonObject &json)
{
    dz = json["dz"].toDouble();
    rminL = json["rminL"].toDouble();
    rmaxL = json["rmaxL"].toDouble();
    rminU = json["rminU"].toDouble();
    rmaxU = json["rmaxU"].toDouble();
}

bool AGeoCone::readFromTShape(TGeoShape *Tshape)
{
    TGeoCone* s = dynamic_cast<TGeoCone*>(Tshape);
    if (!s) return false;

    rminL = s->GetRmin1();
    rmaxL = s->GetRmax1();
    rminU = s->GetRmin2();
    rmaxU = s->GetRmax2();
    dz = s->GetDz();

    return true;
}

const QString AGeoEltu::getHelp()
{
    return "An elliptical tube is defined by the two semi-axes a and b. It ranges from –dz to +dz in Z direction.";
}

bool AGeoEltu::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 3);
    if (!ok) return false;

    double tmp[3];
    for (int i=0; i<3; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoEltu";
            return false;
        }
    }

    a  = tmp[0];
    b  = tmp[1];
    dz = tmp[2];
    return true;
}

TGeoShape *AGeoEltu::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoEltu(a, b, dz) :
                                   new TGeoEltu(shapeName.toLatin1().data(), a, b, dz);
}

const QString AGeoEltu::getGenerationString() const
{
    QString str = "TGeoEltu( " +
            QString::number(a)+", "+
            QString::number(b)+", "+
            QString::number(dz)+" )";
    return str;
}

double AGeoEltu::maxSize()
{
    double m = std::max(a, b);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

void AGeoEltu::writeToJson(QJsonObject &json) const
{
    json["a"] = a;
    json["b"] = b;
    json["dz"] = dz;
}

void AGeoEltu::readFromJson(QJsonObject &json)
{
    a  = json["a"].toDouble();
    b  = json["b"].toDouble();
    dz = json["dz"].toDouble();
}

bool AGeoEltu::readFromTShape(TGeoShape *Tshape)
{
    TGeoEltu* s = dynamic_cast<TGeoEltu*>(Tshape);
    if (!s) return false;

    a = s->GetA();
    b = s->GetB();
    dz = s->GetDz();

    return true;
}

AGeoArb8::AGeoArb8(double dz, QList<QPair<double, double> > VertList) : dz(dz)
{
    if (VertList.size() != 8)
    {
        qWarning() << "Wrong size of input list in AGeoArb8!";
        init();
    }
    else  Vertices = VertList;
}

AGeoArb8::AGeoArb8() : dz(10)
{
    init();
}

const QString AGeoArb8::getHelp()
{
    QString s = "An Arb8 is defined by two quadrilaterals sitting on parallel planes, at ±dZ. These are defined each by 4 vertices "
                "having the coordinates (Xi,Yi,+/-dZ), i=0, 3. The lateral surface of the Arb8 is defined by the 4 pairs of "
                "edges corresponding to vertices (i,i+1) on both -dZ and +dZ. If M and M' are the middles of the segments "
                "(i,i+1) at -dZ and +dZ, a lateral surface is obtained by sweeping the edge at -dZ along MM' so that it will "
                "match the corresponding one at +dZ. Since the points defining the edges are arbitrary, the lateral surfaces are "
                "not necessary planes – but twisted planes having a twist angle linear-dependent on Z.\n"
                "Vertices have to be defined clockwise in the XY pane, both at +dz and –dz. The quadrilateral at -dz is defined "
                "by indices [0,3], whereas the one at +dz by vertices [4,7]. Any two or more vertices in each Z plane can "
                "have the same (X,Y) coordinates. It this case, the top and bottom quadrilaterals become triangles, segments or "
                "points. The lateral surfaces are not necessary defined by a pair of segments, but by pair segment-point (making "
                "a triangle) or point-point (making a line). Any choice is valid as long as at one of the end-caps is at least a "
                "triangle.";
    return s;
}

bool AGeoArb8::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 17);
    if (!ok) return false;

    double tmp[17];
    for (int i=0; i<17; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoArb8";
            return false;
        }
    }

    dz = tmp[0];
    for (int i=0; i<8; i++)
    {
        Vertices[i].first =  tmp[1+i*2];
        Vertices[i].second = tmp[2+i*2];
    }
    //  qDebug() << dz << Vertices;

    if (!AGeoShape::CheckPointsForArb8(Vertices))
    {
        qWarning() << "Nodes of AGeoArb8 should be defined clockwise on both planes";
        return false;
    }

    return true;
}

TGeoShape *AGeoArb8::createGeoShape(const QString shapeName)
{
    double ar[8][2];
    for (int i=0; i<8; i++)
    {
        ar[i][0] = Vertices.at(i).first;
        ar[i][1] = Vertices.at(i).second;
    }

    return (shapeName.isEmpty()) ? new TGeoArb8(dz, (double*)ar) : new TGeoArb8(shapeName.toLatin1().data(), dz, (double*)ar);
}

const QString AGeoArb8::getGenerationString() const
{
    QString str = "TGeoArb8( " + QString::number(dz)+",  ";

    QString s;
    for (int i=0; i<4; i++)
        s += ", " + QString::number(Vertices.at(i).first) + "," + QString::number(Vertices.at(i).second);
    s.remove(0, 1);
    str += s + ",   ";

    s = "";
    for (int i=4; i<8; i++)
        s += ", " + QString::number(Vertices.at(i).first) + "," + QString::number(Vertices.at(i).second);
    s.remove(0, 1);
    str += s + " )";

    return str;
}

double AGeoArb8::maxSize()
{
    return dz;
}

void AGeoArb8::writeToJson(QJsonObject &json) const
{
    json["dz"] = dz;
    QJsonArray ar;
    for (int i=0; i<8; i++)
    {
        QJsonArray el;
        el << Vertices.at(i).first;
        el << Vertices.at(i).second;
        ar.append(el);
    }
    json["Vertices"] = ar;
}

void AGeoArb8::readFromJson(QJsonObject &json)
{
    dz = json["dz"].toDouble();
    QJsonArray ar = json["Vertices"].toArray();
    for (int i=0; i<8; i++)
    {
        QJsonArray el = ar[i].toArray();
        Vertices[i].first = el[0].toDouble();
        Vertices[i].second = el[1].toDouble();
    }
}

bool AGeoArb8::readFromTShape(TGeoShape *Tshape)
{
    TGeoArb8* s = dynamic_cast<TGeoArb8*>(Tshape);
    if (!s) return false;

    dz = s->GetDz();
    const double (*ar)[2] = (const double(*)[2])s->GetVertices(); //fXY[8][2]
    Vertices.clear();
    for (int i=0; i<8; i++)
    {
        QPair<double, double> p(ar[i][0], ar[i][1]);
        Vertices << p;
    }

    return true;
}

void AGeoArb8::init()
{
    Vertices << QPair<double,double>(-20,20) << QPair<double,double>(20,20) << QPair<double,double>(20,-20) << QPair<double,double>(-20,-20);
    Vertices << QPair<double,double>(-10,10) << QPair<double,double>(10,10) << QPair<double,double>(10,-10) << QPair<double,double>(10,-10);
}

AGeoPcon::AGeoPcon()
    : phi(0), dphi(360)
{
    Sections << APolyCGsection(-5, 10, 20) <<  APolyCGsection(5, 20, 40);
}

const QString AGeoPcon::getHelp()
{
    return "TGeoPcon:\n"
           "phi - start angle [0, 360)\n"
           "dphi - range in angle (0, 360]\n"
           "{z : rmin : rmax} - arbitrary number of sections defined with z position, minimum and maximum radii";
}

bool AGeoPcon::readFromString(QString GenerationString)
{
    Sections.clear();

    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, -1);
    if (!ok) return false;

    if (params.size()<4)
    {
        qWarning() << "Not enough parameters to define TGeoPcon";
        return false;
    }

    phi = params[0].toDouble(&ok);
    if (!ok)
    {
        qWarning() << "Syntax error found during extracting parameters of TGeoPcon";
        return false;
    }
    dphi = params[1].toDouble(&ok);
    if (!ok)
    {
        qWarning() << "Syntax error found during extracting parameters of TGeoPcon";
        return false;
    }

    for (int i=2; i<params.size(); i++)
    {
        APolyCGsection section;
        if (!section.fromString(params.at(i)))
        {
            qWarning() << "Syntax error found during extracting parameters of TGeoPcon";
            return false;
        }
        Sections << section;
    }
    //qDebug() << phi<<dphi;
    //for (int i=0; i<Sections.size(); i++) qDebug() << Sections.at(i).toString();
    return true;
}

TGeoShape *AGeoPcon::createGeoShape(const QString shapeName)
{
    TGeoPcon* pc = (shapeName.isEmpty()) ? new TGeoPcon(phi, dphi, Sections.size()) :
                                           new TGeoPcon(shapeName.toLatin1().data(), phi, dphi, Sections.size());
    for (int i=0; i<Sections.size(); i++)
    {
        const APolyCGsection& s = Sections.at(i);
        pc->DefineSection(i, s.z, s.rmin, s.rmax);
    }
    return pc;
}

const QString AGeoPcon::getGenerationString() const
{
    QString str = "TGeoPcon( " +
            QString::number(phi)+", "+
            QString::number(dphi);

    for (const APolyCGsection& s : Sections) str += ", " + s.toString();

    str +=" )";
    return str;
}

double AGeoPcon::maxSize()
{
    double m = 0.5*fabs(Sections.at(0).z - Sections.last().z);
    for (int i=0; i<Sections.size(); i++)
        m = std::max(m, Sections.at(i).rmax);
    return sqrt(3.0)*m;
}

void AGeoPcon::writeToJson(QJsonObject &json) const
{
    json["phi"] = phi;
    json["dphi"] = dphi;
    QJsonArray ar;
    for (APolyCGsection s : Sections)
    {
        QJsonObject js;
        s.writeToJson(js);
        ar << js;
    }
    json["Sections"] = ar;
}

void AGeoPcon::readFromJson(QJsonObject &json)
{
    Sections.clear();

    parseJson(json, "phi", phi);
    parseJson(json, "dphi", dphi);

    QJsonArray ar;
    parseJson(json, "Sections", ar);
    if (ar.size()<2)
    {
        qWarning() << "Error in reading AGeoPcone from json";
        Sections.resize(2);
        return;
    }

    for (int i=0; i<ar.size(); i++)
    {
        QJsonObject js = ar[i].toObject();
        APolyCGsection s;
        s.readFromJson(js);
        Sections << s;
    }
    if (Sections.size()<2)
    {
        qWarning() << "Error in reading AGeoPcone from json";
        Sections.resize(2);
    }
}

bool AGeoPcon::readFromTShape(TGeoShape *Tshape)
{
    TGeoPcon* pc = dynamic_cast<TGeoPcon*>(Tshape);
    if (!pc) return false;

    phi = pc->GetPhi1();
    dphi = pc->GetDphi();

    Sections.clear();
    for (int i=0; i<pc->GetNz(); i++)
        Sections << APolyCGsection(pc->GetZ()[i], pc->GetRmin()[i], pc->GetRmax()[i]);

    //qDebug() << "Pcone loaded from TShape..."<<phi<<dphi;
    //for (int i=0; i<Sections.size(); i++) qDebug() << Sections.at(i).toString();

    return true;
}


bool APolyCGsection::fromString(QString s)
{
    s = s.simplified();
    s.remove(" ");
    if (!s.startsWith("{") || !s.endsWith("}")) return false;

    s.remove("{");
    s.remove("}");
    QStringList l = s.split(":");
    if (l.size()!=3) return false;

    bool ok;
    z = l[0].toDouble(&ok);
    if (!ok) return false;
    rmin = l[1].toDouble(&ok);
    if (!ok) return false;
    rmax = l[2].toDouble(&ok);
    if (!ok) return false;

    return true;
}

const QString APolyCGsection::toString() const
{
    return QString("{ ") +
            QString::number(z) + " : " +
            QString::number(rmin) + " : " +
            QString::number(rmax) +
            " }";
}

void APolyCGsection::writeToJson(QJsonObject &json) const
{
    json["z"] = z;
    json["rmin"] = rmin;
    json["rmax"] = rmax;
}

void APolyCGsection::readFromJson(QJsonObject &json)
{
    parseJson(json, "z", z);
    parseJson(json, "rmin", rmin);
    parseJson(json, "rmax", rmax);
}

// --- GeoPolygon ---
const QString AGeoPolygon::getHelp()
{
    return "Simplified TGeoPgon:\n"
           "nedges - number of edges\n"
           "dphi - angle (0, 360]\n"
           "dz - half size in Z\n"
           "rminL - inner size on lower side\n"
           "rmaxL - outer size on lower side\n"
           "rminU - inner size on uppder side\n"
           "rmaxU - outer size on upper side\n";
}

bool AGeoPolygon::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 7);
    if (!ok) return false;

    int n = params[0].toInt(&ok);
    if (!ok)
    {
        qWarning() << "Syntax error found during extracting parameters of TGeoPolygon";
        return false;
    }

    double tmp[7];
    for (int i=1; i<7; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of TGeoPolygon";
            return false;
        }
    }

    nedges = n;
    dphi = tmp[1];
    dz = tmp[2];
    rminL = tmp[3];
    rmaxL = tmp[4];
    rminU = tmp[5];
    rmaxU = tmp[6];
    return true;
}

TGeoShape *AGeoPolygon::createGeoShape(const QString shapeName)
{
    TGeoPgon* s = (shapeName.isEmpty()) ? new TGeoPgon(0, dphi, nedges, 2) :
                                          new TGeoPgon(shapeName.toLatin1().data(), 0, dphi, nedges, 2);
    s->DefineSection(0, -dz, rminL, rmaxL);
    s->DefineSection(1, +dz, rminU, rmaxU);
    return s;
}

const QString AGeoPolygon::getGenerationString() const
{
    QString str = "TGeoPolygon( " +
            QString::number(nedges)+", "+
            QString::number(dphi)+", "+
            QString::number(dz)+", "+
            QString::number(rminL)+", "+
            QString::number(rmaxL)+", "+
            QString::number(rminU)+", "+
            QString::number(rmaxU)+" )";
    return str;
}

double AGeoPolygon::maxSize()
{
    double m = std::max(rmaxL, rmaxU);
    m = std::max(m, dz);
    return sqrt(3.0)*m;
}

void AGeoPolygon::writeToJson(QJsonObject &json) const
{
    json["nedges"] = nedges;
    json["dphi"] = dphi;
    json["dz"] = dz;
    json["rminL"] = rminL;
    json["rmaxL"] = rmaxL;
    json["rminU"] = rminU;
    json["rmaxU"] = rmaxU;
}

void AGeoPolygon::readFromJson(QJsonObject &json)
{
    nedges = json["nedges"].toInt();
    dphi = json["dphi"].toDouble();
    dz = json["dz"].toDouble();
    rminL = json["rminL"].toDouble();
    rmaxL = json["rmaxL"].toDouble();
    rminU = json["rminU"].toDouble();
    rmaxU = json["rmaxU"].toDouble();
}

AGeoScaledShape::AGeoScaledShape(QString ShapeGenerationString, double scaleX, double scaleY, double scaleZ) :
    BaseShapeGenerationString(ShapeGenerationString),
    scaleX(scaleX), scaleY(scaleY), scaleZ(scaleZ) {}

const QString AGeoScaledShape::getHelp()
{
    return "TGeoShape scaled with TGeoScale transformation";
}

bool AGeoScaledShape::readFromString(QString GenerationString)
{
    GenerationString = GenerationString.simplified();
    //qDebug() << GenerationString;
    if (!GenerationString.startsWith(getShapeType()))
    {
        qWarning() << "Attempt to generate AGeoScaledShape using wrong type!";
        return false;
    }
    GenerationString.remove(getShapeType());
    GenerationString = GenerationString.simplified();
    if (GenerationString.endsWith("\\)"))
    {
        qWarning() << "Format error in AGeoScaledShape read from string";
        return false;
    }
    GenerationString.chop(1);
    if (GenerationString.startsWith("\\("))
    {
        qWarning() << "Format error in AGeoScaledShape read from string";
        return false;
    }
    GenerationString.remove(0, 1);
    GenerationString.remove(QString(" "));

    QStringList l1 = GenerationString.split(')');
    if (l1.size() < 2)
    {
        qWarning() << "Format error in AGeoScaledShape read from string";
        return false;
    }

    QString generator = l1.first() + ")";
    TGeoShape* sh = AGeoScaledShape::generateBaseTGeoShape(generator);
    if (!sh)
    {
        qWarning() << "Not valid generation string:"<<GenerationString;
        return false;
    }
    BaseShapeGenerationString = generator;

    QString params = l1.last(); // should be ",scaleX,scaleY,ScaleZ"
    params.remove(0, 1);
    QStringList l2 = params.split(',');
    if (l2.count() != 3)
    {
        qWarning() << "Number of scaling parameters in AGeoScaledShape should be three!";
        return false;
    }
    bool ok = false;
    scaleX = l2.at(0).toDouble(&ok);
    if (!ok)
    {
        qWarning() << "Scaling parameter error in AGeoScaledShape.";
        return false;
    }
    scaleY = l2.at(1).toDouble(&ok);
    if (!ok)
    {
        qWarning() << "Scaling parameter error in AGeoScaledShape.";
        return false;
    }
    scaleZ = l2.at(2).toDouble(&ok);
    if (!ok)
    {
        qWarning() << "Scaling parameter error in AGeoScaledShape.";
        return false;
    }

    delete sh;
    return true;
}

TGeoShape* AGeoScaledShape::generateBaseTGeoShape(const QString & BaseShapeGenerationString) const
{
    //qDebug() << "SCALED->: Generating base shape from "<< BaseShapeGenerationString;
    QString shapeType = BaseShapeGenerationString.left(BaseShapeGenerationString.indexOf('('));
    //qDebug() << "SCALED->: base type:"<<shapeType;
    TGeoShape* Tshape = 0;
    AGeoShape* Ashape = AGeoShape::GeoShapeFactory(shapeType);
    if (Ashape)
    {
        //qDebug() << "SCALED->" << "Created AGeoShape of type" << Ashape->getShapeType();
        bool fOK = Ashape->readFromString(BaseShapeGenerationString);
        if (fOK)
        {
            Tshape = Ashape->createGeoShape();
            if (!Tshape) qWarning() << "TGeoScaledShape processing: Base shape generation fail!";
        }
        else qWarning() << "TGeoScaledShape processing: failed to construct AGeoShape";
        delete Ashape;
    }
    else qWarning() << "TGeoScaledShape processing: unknown base shape type "<< shapeType;

    return Tshape;
}

TGeoShape *AGeoScaledShape::createGeoShape(const QString shapeName)
{
    /*
   TGeoShape* Tshape = generateBaseTGeoShape(BaseShapeGenerationString);
   if (!Tshape)
     {
       qWarning() << "->failed to generate shape using string:"<<BaseShapeGenerationString<<"\nreplacing by default TGeoBBox";
       Tshape = new TGeoBBox(5,5,5);
     }
   QString name = shapeName + "_base";
   Tshape->SetName(name.toLatin1());
   TGeoScale* scale = new TGeoScale(scaleX, scaleY, scaleZ);
   scale->RegisterYourself();

   return (shapeName.isEmpty()) ? new TGeoScaledShape(Tshape, scale) : new TGeoScaledShape(shapeName.toLatin1().data(), Tshape, scale);
   */

    TGeoShape * Tshape = BaseShape->createGeoShape();
    if (!Tshape)
    {
        qWarning() << "->failed to generate shape\nreplacing by default TGeoBBox";
        Tshape = new TGeoBBox(5,5,5);
    }

    QString name = shapeName + "_base";
    Tshape->SetName(name.toLatin1().data());

    TGeoScale* scale = new TGeoScale(scaleX, scaleY, scaleZ);
    scale->RegisterYourself();

    return (shapeName.isEmpty()) ? new TGeoScaledShape(Tshape, scale) : new TGeoScaledShape(shapeName.toLatin1().data(), Tshape, scale);
}

const QString AGeoScaledShape::getGenerationString() const
{
    return QString() + "TGeoScaledShape( " +
            BaseShapeGenerationString + ", " +
            QString::number(scaleX) + ", " +
            QString::number(scaleY) + ", " +
            QString::number(scaleZ) +
            " )";
}

const QString AGeoScaledShape::getBaseShapeType() const
{
    /*
    QStringList sl = BaseShapeGenerationString.split('(', QString::SkipEmptyParts);

    if (sl.size() < 1) return "";
    return sl.first().simplified();
    */

    if (BaseShape) return BaseShape->getShapeType();
    else exit (-7777);
}

void AGeoScaledShape::writeToJson(QJsonObject &json) const
{
    json["scaleX"] = scaleX;
    json["scaleY"] = scaleY;
    json["scaleZ"] = scaleZ;

    //json["BaseShapeGenerationString"] = BaseShapeGenerationString;
    if (BaseShape)
    {
        BaseShape->writeToJson(json);
        json["shape"] = BaseShape->getShapeType();
    }
}

void AGeoScaledShape::readFromJson(QJsonObject &json)
{
    parseJson(json, "scaleX", scaleX);
    parseJson(json, "scaleY", scaleY);
    parseJson(json, "scaleZ", scaleZ);

    bool bOldSystem = parseJson(json, "BaseShapeGenerationString", BaseShapeGenerationString);
    if (bOldSystem)
    {
        //compatibility
        delete BaseShape;

        qDebug() << "SCALED->: Generating base shape from "<< BaseShapeGenerationString;
        QString shapeType = BaseShapeGenerationString.left(BaseShapeGenerationString.indexOf('('));
        qDebug() << "SCALED->: base type:"<<shapeType;
        BaseShape = AGeoShape::GeoShapeFactory(shapeType);
        if (BaseShape)
        {
            qDebug() << "SCALED->" << "Created AGeoShape of type" << BaseShape->getShapeType();
            bool fOK = BaseShape->readFromString(BaseShapeGenerationString);
            qDebug() << "reading base shape properties ->" << fOK;
        }
    }
    else
    {
        //new system
        QString type = "TGeoBBox";
        parseJson(json, "shape", type);
        BaseShape = AGeoShape::GeoShapeFactory(type);
        if (BaseShape) BaseShape->readFromJson(json);
    }

    if (!BaseShape)
    {
        qWarning() << "Shape generation failed, replacing with box";
        BaseShape = new AGeoBox();
    }
}

bool AGeoScaledShape::readFromTShape(TGeoShape *Tshape)
{
    TGeoScaledShape* s = dynamic_cast<TGeoScaledShape*>(Tshape);
    if (!s) return false;

    TGeoScale* scale = s->GetScale();
    scaleX = scale->GetScale()[0];
    scaleY = scale->GetScale()[1];
    scaleZ = scale->GetScale()[2];

    TGeoShape* baseTshape = s->GetShape();
    QString stype = baseTshape->ClassName();
    AGeoShape* AShape = AGeoShape::GeoShapeFactory(stype);
    if (!AShape)
    {
        qWarning() << "AGeoScaledShape from TShape: error building AGeoShape";
        return false;
    }
    bool fOK = AShape->readFromTShape(baseTshape);
    if (!fOK)
    {
        qWarning() << "AGeoScaledShape from TShape: error reading base TShape";
        delete AShape;
        return false;
    }
    BaseShapeGenerationString = AShape->getGenerationString();

    delete AShape;
    return true;
}


const QString AGeoTorus::getHelp()
{
    return QString()+ "Torus segment:\n"
                      "• R - axial radius\n"
                      "• Rmin - inner radius\n"
                      "• Rmax - outer radius\n"
                      "• Phi1 - starting phi\n"
                      "• Dphi - phi extent";
}

bool AGeoTorus::readFromString(QString GenerationString)
{
    QStringList params;
    bool ok = extractParametersFromString(GenerationString, params, 5);
    if (!ok) return false;

    double tmp[5];
    for (int i=0; i<5; i++)
    {
        tmp[i] = params[i].toDouble(&ok);
        if (!ok)
        {
            qWarning() << "Syntax error found during extracting parameters of AGeoTorus";
            return false;
        }
    }

    R = tmp[0];
    Rmin = tmp[1];
    Rmax = tmp[2];
    Phi1 = tmp[3];
    Dphi = tmp[4];
    return true;
}

TGeoShape *AGeoTorus::createGeoShape(const QString shapeName)
{
    return (shapeName.isEmpty()) ? new TGeoTorus(R, Rmin, Rmax, Phi1, Dphi) : new TGeoTorus(shapeName.toLatin1().data(), R, Rmin, Rmax, Phi1, Dphi);
}

const QString AGeoTorus::getGenerationString() const
{
    QString str = "TGeoTorus( " +
            QString::number(R)+", "+
            QString::number(Rmin)+", "+
            QString::number(Rmax)+", "+
            QString::number(Phi1)+", "+
            QString::number(Dphi)+" )";
    return str;
}

double AGeoTorus::maxSize()
{
    double m = std::max(R, Rmax);
    return sqrt(3.0)*m;
}

void AGeoTorus::writeToJson(QJsonObject &json) const
{
    json["R"] = R;
    json["Rmin"] = Rmin;
    json["Rmax"] = Rmax;
    json["Phi1"] = Phi1;
    json["Dphi"] = Dphi;
}

void AGeoTorus::readFromJson(QJsonObject &json)
{
    parseJson(json, "R", R);
    parseJson(json, "Rmin", Rmin);
    parseJson(json, "Rmax", Rmax);
    parseJson(json, "Phi1", Phi1);
    parseJson(json, "Dphi", Dphi);
}

bool AGeoTorus::readFromTShape(TGeoShape *Tshape)
{
    TGeoTorus* tor = dynamic_cast<TGeoTorus*>(Tshape);
    if (!tor) return false;

    R = tor->GetR();
    Rmin = tor->GetRmin();
    Rmax = tor->GetRmax();
    Phi1 = tor->GetPhi1();
    Dphi = tor->GetDphi();

    return true;
}

// ------------------ STATIC METHODS ---------------------

bool checkPointsArb8(QList<QPair<double, double> > V)
{
    double X=0, Y=0;
    for (int i=0; i<4; i++)
    {
        X += V[i].first;
        Y += V[i].second;
    }
    X /= 4;
    Y /= 4;
    //qDebug() << "Center x,y:"<<X << Y;

    QList<double> angles;
    int firstNotNAN = -1;
    for (int i=0; i<4; i++)
    {
        double dx = V[i].first-X;
        double dy = V[i].second - Y;
        double a = atan( fabs(dy)/fabs(dx) ) * 180.0 / 3.1415926535;
        if (a==a && firstNotNAN==-1) firstNotNAN = i;

        if (dx>0 && dy>0)      angles.append( 360.0 - a );
        else if (dx<0 && dy>0) angles.append( 180.0 + a );
        else if (dx<0 && dy<0) angles.append( 180.0 - a );
        else                   angles.append( a );
    }
    //qDebug() << "Raw angles:" << angles;
    // qDebug() << "First Not NAN:"<< firstNotNAN;
    if (firstNotNAN == -1) return true; //all 4 points are the same

    double delta = angles[firstNotNAN];
    for (int i=0; i<4; i++)
    {
        if (angles[i] == angles[i]) // not NAN
        {
            angles[i] -= delta;
            if (angles[i]<0) angles[i] += 360.0;
        }
    }

    //qDebug() << "Shifted to first"<<angles;
    double A = angles[firstNotNAN];
    for (int i=firstNotNAN+1; i<4; i++)
    {
        //qDebug() <<i<< angles[i];
        if (angles[i] != angles[i])
        {
            //qDebug() << "NAN, continue";
            continue;
        }
        if (angles[i]<A)
            return false;
        A=angles[i];
    }
    return true;
}

bool AGeoShape::CheckPointsForArb8(QList<QPair<double, double> > V)
{
    if (V.size() != 8) return false;

    bool ok = checkPointsArb8(V);
    if (!ok) return false;

    V.removeFirst();
    V.removeFirst();
    V.removeFirst();
    V.removeFirst();
    return checkPointsArb8(V);
}

QString AGeoShape::updateParameter(const QString & str, double & returnValue, bool bForbidZero, bool bForbidNegative, bool bMakeHalf)
{
    if (str.isEmpty()) return "";

    bool ok;
    returnValue = str.simplified().toDouble(&ok);
    if (!ok)
    {
        ok = AGeoConsts::getInstance().evaluateFormula(str, returnValue);
        if (!ok) return QString("Syntax error:\n%1").arg(str);
    }

    if (bForbidZero && returnValue == 0) return "Unacceptable value: zero";
    if (bForbidNegative && returnValue < 0) return "Unacceptable value: negative";

    if (bMakeHalf) returnValue *= 0.5;
    return "";
}

AGeoShape * AGeoShape::GeoShapeFactory(const QString ShapeType)
{
    if (ShapeType == "TGeoBBox")
        return new AGeoBox();
    else if (ShapeType == "TGeoPara")
        return new AGeoPara();
    else if (ShapeType == "TGeoSphere")
        return new AGeoSphere();
    else if (ShapeType == "TGeoTube")
        return new AGeoTube();
    else if (ShapeType == "TGeoTubeSeg")
        return new AGeoTubeSeg();
    else if (ShapeType == "TGeoCtub")
        return new AGeoCtub();
    else if (ShapeType == "TGeoEltu")
        return new AGeoEltu();
    else if (ShapeType == "TGeoTrd1")
        return new AGeoTrd1();
    else if (ShapeType == "TGeoTrd2")
        return new AGeoTrd2();
    else if (ShapeType == "TGeoPgon")
        return new AGeoPgon();
    else if (ShapeType == "TGeoPolygon")
        return new AGeoPolygon();
    else if (ShapeType == "TGeoCone")
        return new AGeoCone();
    else if (ShapeType == "TGeoConeSeg")
        return new AGeoConeSeg();
    else if (ShapeType == "TGeoPcon")
        return new AGeoPcon();
    else if (ShapeType == "TGeoParaboloid")
        return new AGeoParaboloid();
    else if (ShapeType == "TGeoArb8")
        return new AGeoArb8();
    else if (ShapeType == "TGeoTorus")
        return new AGeoTorus();
    else if (ShapeType == "TGeoCompositeShape")
        return new AGeoComposite();
    else if (ShapeType == "TGeoScaledShape")
        return new AGeoScaledShape();
    else return 0;
}

QList<AGeoShape *> AGeoShape::GetAvailableShapes()
{
    QList<AGeoShape *> list;
    list << new AGeoBox << new AGeoPara << new AGeoSphere
         << new AGeoTube << new AGeoTubeSeg << new AGeoCtub << new AGeoEltu
         << new AGeoTrd1 << new AGeoTrd2
         << new AGeoCone << new AGeoConeSeg << new AGeoPcon
         << new AGeoPolygon << new AGeoPgon
         << new AGeoParaboloid << new AGeoTorus
         << new AGeoArb8 << new AGeoComposite
         << new AGeoScaledShape;
    return list;
}
