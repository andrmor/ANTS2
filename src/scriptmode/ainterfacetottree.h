#ifndef AINTERFACETOTTREE_H
#define AINTERFACETOTTREE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVector>
#include <QString>
#include <QVariant>

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

   void     Load(const QString& TreeName, const QString& FileName, const QString& TreeNameInFile); //one leaf per branch, all branches have same length
   void     Save(const QString& TreeName, const QString& FileName);

   void     CreateTree(const QString &TreeName, const QVariant HeadersOfBranches);
   void     AddEntry(const QString &TreeName, const QVariant Array);

   int      GetNumEntries(const QString &TreeName);
   const QVariant GetBranchNames(const QString &TreeName);
   const QVariant GetAllTreeNames();
   const QString  GetTreeStructure(const QString &TreeName);

   const QVariant GetBranch(const QString &TreeName, const QString &BranchName);
   const QVariant GetBranch(const QString &TreeName, const QString &BranchName, int entry);
   const QVariant GetEntry(const QString &TreeName, int entry);

   const QString Draw(const QString& TreeName, const QString& what, const QString& cuts, const QString& options, const QVariant binsAndRanges = QVariantList());

   bool     DeleteTree(const QString &TreeName);
   void     DeleteAllTrees();

signals:
   void     RequestTreeDraw(TTree* tree, const QString& what, const QString& cond, const QString& how, const QVariantList& binsAndRanges, QString* result = 0);

private:
   TmpObjHubClass* TmpHub;

   bool            bAbortIfExists = false;
};

#endif // AINTERFACETOTTREE_H
