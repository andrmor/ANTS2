#include "aroottreerecord.h"

#include "TObject.h"
#include "TTree.h"

#include <QVariant>
#include <QDebug>

void ABranchBuffer::createBranch(TTree *t)
{
    TString n = name.toLatin1().data();
    TString f = n + "/" + type.toLatin1().data();

    if (type.length() == 1)
    {
        bVector = false;
        cType   = type.at(0).toLatin1();

        switch (cType)
        {
            case 'C' : branchPtr = t->Branch(n, &C, f); break;
            case 'I' : branchPtr = t->Branch(n, &I, f); break;
            case 'F' : branchPtr = t->Branch(n, &F, f); break;
            case 'D' : branchPtr = t->Branch(n, &D, f); break;
            case 'O' : branchPtr = t->Branch(n, &O, f); break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
    else
    {
        bVector = true;
        cType   = type.at(1).toLatin1();

        switch (cType)
        {
            case 'C' : branchPtr = t->Branch(n, &AC); break;
            case 'I' : branchPtr = t->Branch(n, &AI); break;
            case 'F' : branchPtr = t->Branch(n, &AF); break;
            case 'D' : branchPtr = t->Branch(n, &AD); break;
            case 'O' : branchPtr = t->Branch(n, &AO); break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
}

void ABranchBuffer::reconnectAddresses(TTree *t)
{
    qDebug() << "Attempting to reconnect";

    if (type.isEmpty()) return;

    TString n = name.toLatin1().data();

    if (bVector)
    {
        switch (cType)
        {
            case 'C' : t->SetBranchAddress(n, &C); break;
            case 'I' : t->SetBranchAddress(n, &I); break;
            case 'F' : t->SetBranchAddress(n, &F); break;
            case 'D' : t->SetBranchAddress(n, &D); break;
            case 'O' : t->SetBranchAddress(n, &O); break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
    else
    {
        bVector = true;
        cType   = type.at(1).toLatin1();

        switch (cType)
        {
            case 'C' : t->SetBranchAddress(n, &AC); break;
            case 'I' : t->SetBranchAddress(n, &AI); break;
            case 'F' : t->SetBranchAddress(n, &AF); break;
            case 'D' : t->SetBranchAddress(n, &AD); break;
            case 'O' : t->SetBranchAddress(n, &AO); break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
}

void ABranchBuffer::write(const QVariant &val)
{
    if (!bVector)
    {
        switch (cType)
        {
            case 'C' : C = val.toString().toLatin1().data(); break;
            case 'I' : I = val.toInt();    break;
            case 'F' : F = val.toFloat();  break;
            case 'D' : D = val.toDouble(); break;
            case 'O' : O = val.toBool();   break;
            default  : qWarning() << "write - unknown tree branch type:" << type;
        }
    }
    else
    {
        const QVariantList vl = val.toList();
        switch (cType)
        {
            case 'C' : AC.resize(vl.size()); for (int i=0; i<vl.size(); i++) AC[i] = vl.at(i).toString().toLatin1().data(); break;
            case 'I' : AI.resize(vl.size()); for (int i=0; i<vl.size(); i++) AI[i] = vl.at(i).toInt();    break;
            case 'F' : AF.resize(vl.size()); for (int i=0; i<vl.size(); i++) AF[i] = vl.at(i).toFloat();  break;
            case 'D' : AD.resize(vl.size()); for (int i=0; i<vl.size(); i++) AD[i] = vl.at(i).toDouble(); break;
            case 'O' : AO.resize(vl.size()); for (int i=0; i<vl.size(); i++) AO[i] = vl.at(i).toBool();   break;
            default  : qWarning() << "write - unknown tree branch type:" << type;
        }
    }
}

const QVariant ABranchBuffer::read()
{
    if (!bVector)
    {
        switch (cType)
        {
            case 'C' : return QString(C);
            case 'I' : return I;
            case 'F' : return F;
            case 'D' : return D;
            case 'O' : return O;
            default  : qWarning() << "read - unknown tree branch type:" << type;
        }
    }
    else
    {
        QVariantList vl;
        switch (cType)
        {
            case 'C' : for (const TString& e : AC) vl << QString(e); return vl;
            case 'I' : for (const int& e : AI) vl << e; return vl;
            case 'F' : for (const float& e : AF) vl << e; return vl;
            case 'D' : for (const double& e : AD) vl << e; return vl;
            case 'O' : for (const bool& e : AO) vl << e; return vl;
            default  : qWarning() << "read - unknown tree branch type:" << type;
        }
    }
    return 0;
}

ARootTreeRecord::ARootTreeRecord(TObject *tree, const QString &name) :
    ARootObjBase(tree, name, "") {}

bool ARootTreeRecord::createTree(const QString &name, const QVector<QPair<QString, QString> > &branches)
{
    QMutexLocker locker(&Mutex);

    Branches.resize(branches.size());

    for (int ib = 0; ib < branches.size(); ib++)
    {
        ABranchBuffer& b = Branches[ib];
        b.name = branches.at(ib).first;
        b.type = branches.at(ib).second;
        if (!ABranchBuffer::getAllTypes().contains(b.type))
            return false;
    }

    TTree* t = new TTree(name.toLatin1().data(), "");

    if (Object) delete Object;
    Object = t;

    for (int ib = 0; ib < branches.size(); ib++)
    {
        ABranchBuffer& b = Branches[ib];
        b.createBranch(t);
    }

    return true;
}

int ARootTreeRecord::countBranches() const
{
    QMutexLocker locker(&Mutex);
    return Branches.size();
}

bool ARootTreeRecord::fillSingle(const QVariantList &vl)
{
    QMutexLocker locker(&Mutex);

    if (vl.size() != Branches.size()) return false;

    for (int ib = 0; ib < Branches.size(); ib++)
    {
        ABranchBuffer& b = Branches[ib];
        b.write(vl.at(ib));
    }
    static_cast<TTree*>(Object)->Fill();

    return true;
}

bool ARootTreeRecord::isBranchExist(const QString &branchName) const
{
    QMutexLocker locker(&Mutex);

    for (const ABranchBuffer& b : Branches)
        if (b.name == branchName) return true;
    return false;
}

const QVariantList ARootTreeRecord::getBranch(const QString &branchName)
{
    QVariantList res;
    TTree *t = static_cast<TTree*>(Object);
    TBranch* branch = t->GetBranch(branchName.toLocal8Bit().data());
    if (branch)
    {
        const int numEntries = branch->GetEntries();
        ABranchBuffer* bb = getBranchBuffer(branchName);
        if (bb)
        {
            for (int i = 0; i < numEntries; i++)
            {
                Long64_t tentry = t->LoadTree(i);
                branch->GetEntry(tentry);
                res.push_back(bb->read());
            }
        }
    }
    return res;
}

const QVariant ARootTreeRecord::getBranch(const QString &branchName, int entry)
{
    TTree *t = static_cast<TTree*>(Object);
    ABranchBuffer* bb = getBranchBuffer(branchName);

    TBranch* branchPtr = bb->branchPtr;//->GetBranch(branchName.toLocal8Bit().data());

    if (branchPtr)
    {
        const int numEntries = branchPtr->GetEntries();
        if (entry>=0 && entry<numEntries)
        {

            if (bb)
            {
                    Long64_t tentry = t->LoadTree(entry);
                    branchPtr->GetEntry(tentry);
                    return bb->read();
            }
        }
    }
    return QVariant();
}

void ARootTreeRecord::save(const QString &FileName)
{
    TTree* t = static_cast<TTree*>(Object);
    t->SaveAs(FileName.toLatin1().data());
}

void ARootTreeRecord::reconnectBranchAddresses()
{
    TTree* t = static_cast<TTree*>(Object);
    for (ABranchBuffer& b : Branches) b.reconnectAddresses(t);
}

ABranchBuffer* ARootTreeRecord::getBranchBuffer(const QString &name)
{
    for (ABranchBuffer& bb : Branches)
        if (bb.name == name) return &bb;
    return 0;
}


