#include "afiletools.h"
#include "amessage.h"

#include <QStringList>
#include <QFile>
#include <QDebug>

#ifdef GUI
#include <QMessageBox>
#endif

int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x)
{
  if (FileName.isEmpty())
      {
          message("Error: empty name was given to file loader!");
          return 1;
      }

  QFile file(FileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("Could not open: "+FileName);
      return 2;
    }

  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
  x->resize(0);
  while(!in.atEnd())
       {
          QString line = in.readLine();
          QStringList fields = line.split(rx, QString::SkipEmptyParts);

          bool ok1= false;
          double xx;
          if (fields.size()>0) xx = fields[0].toDouble(&ok1);  //*** potential problem with decimal separator!

          if (ok1)
            {
              x->append(xx);
            }
        }
   file.close();

   if (x->isEmpty())
   {
       message("Error: Wrong format - nothing was red: "+FileName);
       return 3;
   }

   return 0;
}

int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x, QVector<double>* y, QString * header, int numLines)
{
  bool bGetHeader = (header && !header->isEmpty());
  QString HeaderId;
  if (bGetHeader)
  {
      HeaderId = *header;
      header->clear();
  }

  if (FileName.isEmpty())
      {
          message("Error: empty name was given to file loader!");
          return 1;
      }

  QFile file(FileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("Could not open: "+FileName);
      return 2;
    }

  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
  x->resize(0);
  y->resize(0);
  while(!in.atEnd())
       {
          QString line = in.readLine();

          if (bGetHeader && line.startsWith(HeaderId) && numLines > 0)
          {
              if ( !header->isEmpty() ) *header += "\n";
              *header += line.remove(0, HeaderId.length());
              numLines--;
              continue;
          }

          QStringList fields = line.split(rx, QString::SkipEmptyParts);

          bool ok1=false, ok2;
          double xx, yy;
          if (fields.size() > 1 )
            {
              xx = fields[0].toDouble(&ok1);  //*** potential problem with decimal separator!
              yy = fields[1].toDouble(&ok2);
            }
          if (ok1 && ok2)
            {
              x->append(xx);
              y->append(yy);
            }
        }
   file.close();

   if (x->isEmpty())
   {
       message("Error: Wrong format - nothing was red: "+FileName);
       return 3;
   }

   return 0;
}

int LoadDoubleVectorsFromFile(QString FileName, QVector<double>* x, QVector<double>* y, QVector<double>* z)
{
  if (FileName.isEmpty())
  {
      message("Error: empty name was given to file loader!");
      return 1;
  }

  QFile file(FileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("Could not open: "+FileName);
      return 2;
    }

  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
  x->resize(0);
  y->resize(0);
  z->resize(0);
  while(!in.atEnd())
       {
          QString line = in.readLine();
          QStringList fields = line.split(rx, QString::SkipEmptyParts);

          bool ok1=false, ok2, ok3;
          double xx, yy, zz;
          if (fields.size() > 2 )
            {
               xx = fields[0].toDouble(&ok1);  //*** potential problem with decimal separator!
               yy = fields[1].toDouble(&ok2);
               zz = fields[2].toDouble(&ok3);
            }
          if (ok1 && ok2 && ok3)
            {
              x->append(xx);
              y->append(yy);
              z->append(zz);
            }
        }
   file.close();

   if (x->isEmpty())
   {
       message("Error: Wrong format - nothing was red: "+FileName);
       return 3;
   }

  return 0;
}

const QString LoadDoubleVectorsFromFile(const QString FileName, QVector<QVector<double> *> V)
{
    if (FileName.isEmpty()) return("Empty file name");

    QFile file(FileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text)) return QString("Could not open: ")+FileName;

    const int Vsize = V.size();
    if (Vsize==0) return "Received no vectors to load";
    for (QVector<double>* v : V) v->clear();

    QTextStream in(&file);
    QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
    while (!in.atEnd())
         {
            QString line = in.readLine();
            QStringList fields = line.split(rx, QString::SkipEmptyParts);

            bool fOK = true;
            QVector<double> tmp;
            if (fields.size() >= Vsize )
              {
                for (int i=0; i<Vsize; i++)
                {
                    double x = fields.at(i).toDouble(&fOK);
                    if (!fOK) break;
                    tmp << x;
                }
              }
            if (fOK && tmp.size()==Vsize)
               for (int i=0; i<Vsize; i++) V[i]->append( tmp.at(i) );
         }
     file.close();

     if (V.at(0)->isEmpty()) return QString("Wrong format - nothing was read from: ")+FileName;

    return "";
}

int SaveDoubleVectorsToFile(QString FileName, const QVector<double>* x, int count)
{
  if (count == -1) count = x->size();
  QFile outFile( FileName );
  outFile.open(QIODevice::WriteOnly);
  if(!outFile.isOpen())
    {
      qDebug() << "- Error, unable to open" << FileName << "for output";
#ifdef GUI
      QMessageBox mb;
      mb.setText("Unable to open file " +FileName+ " for writing!");
      mb.exec();
#endif
      return 1;
    }
  QTextStream outStream(&outFile);

  for (int i=0; i<count; i++)
    outStream << x->at(i) <<"\r\n";
  outFile.close();
  return 0;
}

int SaveDoubleVectorsToFile(QString FileName, const QVector<double> *x, const QVector<double> *y, int count)
{
  if (count == -1) count = x->size();
  QFile outFile( FileName );
  outFile.open(QIODevice::WriteOnly);
  if(!outFile.isOpen())
    {
      qDebug() << "- Error, unable to open" << FileName << "for output";
 #ifdef GUI
      QMessageBox mb;
      mb.setText("Unable to open file " +FileName+ " for writing!");
      mb.exec();
 #endif
      return 1;
    }
  QTextStream outStream(&outFile);

  for (int i=0; i<count; i++)
    outStream << x->at(i) << " " << y->at(i) <<"\r\n";
  outFile.close();
  return 0;
}

int SaveDoubleVectorsToFile(QString FileName, const QVector<double> *x, const QVector<double> *y, const QVector<double> *z, int count)
{
  if (count == -1) count = x->size();
  QFile outFile( FileName );
  outFile.open(QIODevice::WriteOnly);
  if(!outFile.isOpen())
    {
      qDebug() << "- Error, unable to open" << FileName << "for output";
 #ifdef GUI
      QMessageBox mb;
      mb.setText("Unable to open file " +FileName+ " for writing!");
      mb.exec();
 #endif
      return 1;
    }
  QTextStream outStream(&outFile);

  for (int i=0; i<count; i++)
    outStream << x->at(i) << " " << y->at(i) << " " << z->at(i) << "\r\n";
  outFile.close();
  return 0;
}

int LoadIntVectorsFromFile(QString FileName, QVector<int>* x)
{
  if (FileName.isEmpty())
      {
          message("Empty file: "+FileName);
          return 1;
      }

  QFile file(FileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("Could not open: "+FileName);
      return 2;
    }

  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
  x->resize(0);
  while(!in.atEnd())
       {
          QString line = in.readLine();
          QStringList fields = line.split(rx);

          bool ok1;
          int xx = fields[0].toInt(&ok1);
          if (ok1)
            {
              x->append(xx);
            }
        }
   file.close();

   if (x->isEmpty())
   {
       message("Error: Wrong format - nothing was red: "+FileName);
       return 3;
   }

   return 0;
}

int LoadIntVectorsFromFile(QString FileName, QVector<int> *x, QVector<int> *y)
{
  if (FileName.isEmpty())
      {
          message("Empty file: "+FileName);
          return 1;
      }

  QFile file(FileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      message("Could not open: "+FileName);
      return 2;
    }

  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
  x->resize(0);
  y->resize(0);
  while(!in.atEnd())
       {
          QString line = in.readLine();
          QStringList fields = line.split(rx);

          bool ok1, ok2;
          int xx = fields[0].toInt(&ok1);
          int yy = fields[1].toInt(&ok2);
          if (ok1 && ok2)
            {
              x->append(xx);
              y->append(yy);
            }
        }
   file.close();

   if (x->isEmpty())
   {
       message("Error: Wrong format - nothing was red: "+FileName);
       return 3;
   }

   return 0;
}

int SaveIntVectorsToFile(QString FileName, const QVector<int> *x, int count)
{
  if (count == -1) count = x->size();
  QFile outFile( FileName );
  outFile.open(QIODevice::WriteOnly);
  if(!outFile.isOpen())
    {
      qDebug() << "- Error, unable to open" << FileName << "for output";
 #ifdef GUI
      QMessageBox mb;
      mb.setText("Unable to open file " +FileName+ " for writing!");
      mb.exec();
 #endif
      return 1;
    }
  QTextStream outStream(&outFile);

  for (int i=0; i<count; i++)
    outStream << x->at(i) <<"\r\n";
  outFile.close();
  return 0;
}

int SaveIntVectorsToFile(QString FileName, const QVector<int> *x, const QVector<int> *y, int count)
{
  if (count == -1) count = x->size();
  QFile outFile( FileName );
  outFile.open(QIODevice::WriteOnly);
  if(!outFile.isOpen())
    {
      qDebug() << "- Error, unable to open" << FileName << "for output";
 #ifdef GUI
      QMessageBox mb;
      mb.setText("Unable to open file " +FileName+ " for writing!");
      mb.exec();
 #endif
      return 1;
    }
  QTextStream outStream(&outFile);

  for (int i=0; i<count; i++)
    outStream << x->at(i) << " " << y->at(i) <<"\r\n";
  outFile.close();
  return 0;
}

bool LoadTextFromFile(const QString &FileName, QString &string)
{
    QFile file( FileName );
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
        return false;

    QTextStream in(&file);
    string = in.readAll();
    file.close();
    return true;
}

bool SaveTextToFile(const QString &FileName, const QString &text)
{
    QFile file( FileName );
    if(!file.open(QIODevice::WriteOnly | QFile::Text))
        return false;

    QTextStream out(&file);
    out << text;
    file.close();
    return true;
}
