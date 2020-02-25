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

    QTreeWidgetItem * Item = nullptr;

    ATemplateSelectionRecord(const QString & Label, ATemplateSelectionRecord * Parent);
    ATemplateSelectionRecord(){}
    virtual ~ATemplateSelectionRecord();

    void addChild(ATemplateSelectionRecord * rec);

    ATemplateSelectionRecord * findChild(const QString & Label) const;
    ATemplateSelectionRecord * findRecordByItem(const QTreeWidgetItem * item);
};

#endif // ATEMPLATESELECTIONRECORD_H
