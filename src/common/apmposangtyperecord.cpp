#include "apmposangtyperecord.h"

QJsonArray APmPosAngTypeRecord::writeToJArrayAutoZ()
{
  QJsonArray el;
  el << type << x << y << psi;
  return el;
}

void APmPosAngTypeRecord::readFromJArrayAutoZ(QJsonArray &arr)
{
  type = arr[0].toInt();
  x =    arr[1].toDouble();
  y =    arr[2].toDouble();
  z = 0;
  phi =   0;
  theta = 0;

  if (arr.size()<4) psi = 0;
  else psi = arr[3].toDouble();
}

QJsonArray APmPosAngTypeRecord::writeToJArrayFull()
{
  QJsonArray el;
  el << type << x << y << z << phi << theta << psi;
  return el;
}

void APmPosAngTypeRecord::readFromJArrayFull(QJsonArray &arr)
{
  type =  arr[0].toInt();
  x =     arr[1].toDouble();
  y =     arr[2].toDouble();
  z =     arr[3].toDouble();
  phi =   arr[4].toDouble();
  theta = arr[5].toDouble();
  psi =   arr[6].toDouble();
}
