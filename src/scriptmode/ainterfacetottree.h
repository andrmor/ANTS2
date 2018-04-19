#ifndef AINTERFACETOTTREE_H
#define AINTERFACETOTTREE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVector>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include "TString.h"
#include "TTree.h"

#include <vector>

class TmpObjHubClass;

class AInterfaceToTTree : public AScriptInterface
{
  Q_OBJECT

public:
   AInterfaceToTTree(TmpObjHubClass *TmpHub);
   ~AInterfaceToTTree() {}

public slots:
   void     SetAbortIfAlreadyExists(bool flag) {bAbortIfExists = flag;}

   void     LoadTree(const QString& TreeName, const QString& FileName, const QString& TreeNameInFile); //one leaf per branch!
   void     NewTree(const QString &TreeName, const QVariant HeadersOfBranches);

   void     Fill(const QString &TreeName, const QVariant Array);

   int      GetNumEntries(const QString &TreeName) const;
   const QVariant GetBranchNames(const QString &TreeName) const;
   const QVariant GetBranchTypes(const QString &TreeName) const;

   const QVariant GetBranch(const QString &TreeName, const QString &BranchName);
   const QVariant GetBranch(const QString &TreeName, const QString &BranchName, int EntryIndex);
   const QVariant GetEntry(const QString &TreeName, int EntryIndex);

   const QString  Draw(const QString& TreeName, const QString& what, const QString& cuts, const QString& options,
                       const QVariant binsAndRanges = QVariantList(), const QVariant markerAndLineAttributes = QVariantList());

   void     Save(const QString& TreeName, const QString& FileName);

   const QVariant GetAllTreeNames() const;

   bool     DeleteTree(const QString &TreeName);
   void     DeleteAllTrees();

signals:
   void     RequestTreeDraw(TTree* tree, const QString& what, const QString& cond, const QString& how,
                            const QVariantList binsAndRanges, const QVariantList markersAndLine, QString* result = 0);

private:
   TmpObjHubClass* TmpHub;

   bool            bAbortIfExists = false;
};

#endif // AINTERFACETOTTREE_H
