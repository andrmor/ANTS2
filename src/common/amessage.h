#ifndef AMESSAGE_H
#define AMESSAGE_H

#include <QString>
#include <iostream>

#ifdef GUI
class QWidget;

void message(QString text, QWidget* parent = 0);
void message1(const QString & text, const QString &title, QWidget * parent = 0);
bool confirm(const QString &text, QWidget* parent = 0);

void inputInteger(const QString & text, int & input, int min, int max, QWidget * parent = nullptr);
QString inputString(const QString & label, QWidget * parent = nullptr);

#else
void message(QString text);
#endif

#endif // AMESSAGE_H
