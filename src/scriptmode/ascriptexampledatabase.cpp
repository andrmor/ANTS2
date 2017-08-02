#include "ascriptexampledatabase.h"

#include <QDebug>

AScriptExampleDatabase::AScriptExampleDatabase(QString ConfigText)
{
  QStringList list = ConfigText.split("#end");

  for (int i=0; i<list.size(); i++)
  {
      AScriptExample example;
      if (example.ReadFromRecord(list.at(i)))
      {
          Examples.append(example);
          for (int j=0; j<example.Tags.size(); j++)
              if (!Tags.contains(example.Tags.at(j))) Tags.append(example.Tags.at(j));
      }
  }

  //qDebug() << "Extracted"<<Examples.size()<<"examples";
  Tags.sort();
}

void AScriptExampleDatabase::Select(QStringList tags)
{
    if (tags.isEmpty())
    {
        UnselectAll();
        return;
    }

    for (int i=0; i<Examples.size(); i++)
    {
        Examples[i].fSelected = false;
        for (int j=0; j<tags.size(); j++)
        {
            if (Examples.at(i).Tags.contains(tags.at(j)))
            {
                Examples[i].fSelected = true;
                continue;
            }
        }
    }
}

void AScriptExampleDatabase::UnselectAll()
{
    for (int i=0; i<Examples.size(); i++)
        Examples[i].fSelected = false;
}

int AScriptExampleDatabase::Find(QString fileName)
{
    for (int i=0; i<Examples.size(); i++)
    {
        if (Examples.at(i).FileName == fileName)
            return i;
    }
    return -1;
}
