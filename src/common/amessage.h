#ifndef AMESSAGE_H
#define AMESSAGE_H

#include <QString>
#include <iostream>

#ifdef GUI
class QWidget;
void message(QString text, QWidget* parent = 0);
#else
void message(QString text);
#endif

#endif // AMESSAGE_H
