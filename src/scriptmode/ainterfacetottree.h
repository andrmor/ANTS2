#ifndef AINTERFACETOTTREE_H
#define AINTERFACETOTTREE_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVector>
#include <QString>
#include <QVariant>

#include "TString.h"

#include <vector>

class TmpObjHubClass;
class TTree;

class AInterfaceToTTree : public AScriptInterface
{
  Q_OBJECT

public:
   AInterfaceToTTree(TmpObjHubClass *TmpHub);
   ~AInterfaceToTTree() {}

public slots:
   void     SetAbortIfAlreadyExists(bool flag) {bAbortIfExists = flag;}

   void     LoadTTree(const QString &TreeName, const QString &FileName, const QString &TreeNameInFile);
   void     CreateTree(const QString &TreeName, const QVariant HeadersOfBranches);
   void     FillTree_SingleEntry(const QString &TreeName, const QVariant Array);

   const QString  PrintBranches(const QString &TreeName);
   const QVariant GetBranch(const QString &TreeName, const QString &BranchName);

   bool     DeleteTree(const QString &TreeName);
   void     DeleteAllTrees();

private:
   TmpObjHubClass *TmpHub;

   bool           bAbortIfExists = false;
};

#endif // AINTERFACETOTTREE_H
