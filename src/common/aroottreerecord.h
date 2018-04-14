#ifndef AROOTTREERECORD_H
#define AROOTTREERECORD_H

#include "arootobjbase.h"

#include <QObject>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QPair>
#include <QString>
#include <QMutex>

#include "TString.h"

#include <vector>
#include <string>

class TTree;
class TBranch;

class ABranchBuffer
{
public:
    ABranchBuffer(const QString& branchName, const QString& branchType, TTree* tree);     // creates a new branch!
    ABranchBuffer(const QString& branchName, const QString& branchType, TBranch* branch); // uses an existent branch
    ABranchBuffer(){}

    bool           isValid() const {return branchPtr;}

    void           write(const QVariant& val);
    const QVariant read();

    QString        name;
    TString        tName;
    QString        type;
    TTree*         treePtr;
    char           cType = '-';
    bool           bVector = false;
    TBranch*       branchPtr = 0; // if it remains 0 -> branch is invalid

    //TString C;
    char    C[300];
    int     I;
    float   F;
    double  D;
    bool    O;

    std::vector<std::string> AC, *pAC; //need pointers in TTree::setBranchAddress()
    std::vector<int>     AI, *pAI;
    std::vector<float>   AF, *pAF;
    std::vector<double>  AD, *pAD;
    std::vector<bool>    AO, *pAO;

    QVector<bool>        Ao;

    float *pF;

public:
    static QVector<QString> getAllTypes() {QVector<QString> s; s<<"C"<<"I"<<"F"<<"D"<<"O"<<"AC"<<"AI"<<"AF"<<"AD"<<"AO";return s;}
};

class ARootTreeRecord : public ARootObjBase
{
public:
    ARootTreeRecord(TObject* tree, const QString& name);
    ~ARootTreeRecord();

    // Protected by Mutex
    bool  createTree(const QString& name, const QVector<QPair<QString, QString>>& branches);
    const QString loadTree(const QString& fileName, const QString treeNameInFile = ""); //report error ("" if fine)
    int   countBranches() const;
    int   countEntries() const;
    bool  fillSingle(const QVariantList& vl);
    bool  isBranchExist(const QString& branchName) const;
    const QVariantList getBranch(const QString& branchName);
    const QVariant     getBranch(const QString& branchName, int entry);
    void  save(const QString &FileName);    
    const QVariantList getEntry(int entry);

private:
    QVector<ABranchBuffer*> Branches;
    QMap<QString, ABranchBuffer*> MapOfBranches;

};

#endif // AROOTTREERECORD_H
