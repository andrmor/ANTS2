#include "localscriptinterfaces.h"

#include <QtGui/QVector3D>
#include <QDebug>

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

InterfaceToNodesScript::InterfaceToNodesScript(QVector<QVector3D*> & nodes) : nodes(nodes)
{
    H["node"] = "Adds a node with (x,y,z) coordinates";
    H["clear"] = "clear all nodes";
}

void InterfaceToNodesScript::clear()
{
    for (int i=0; i<nodes.size(); i++) delete nodes.at(i);
    nodes.clear();
}

void InterfaceToNodesScript::node(double x, double y, double z)
{
    nodes.append( new QVector3D(x,y,z) );
}
