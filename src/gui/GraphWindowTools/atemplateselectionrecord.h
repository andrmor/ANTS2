#ifndef ATEMPLATESELECTIONRECORD_H
#define ATEMPLATESELECTIONRECORD_H

#include <QString>
#include <QVector>

class QTreeWidgetItem;

class ATemplateSelectionRecord
{
public:
    QString Label;
    bool bSelected = true;
    bool bExpanded = false;

    ATemplateSelectionRecord * Parent = nullptr;
    QVector<ATemplateSelectionRecord *> Children;

    QTreeWidgetItem * item = nullptr;

    ATemplateSelectionRecord(const QString & Label, ATemplateSelectionRecord * Parent);
    ATemplateSelectionRecord(){}
    virtual ~ATemplateSelectionRecord();

    void addChild(ATemplateSelectionRecord * rec);
};

#endif // ATEMPLATESELECTIONRECORD_H
