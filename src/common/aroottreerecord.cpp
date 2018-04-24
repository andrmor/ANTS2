#include "aroottreerecord.h"

#include "TObject.h"
#include "TTree.h"
#include "TFile.h"
#include "TKey.h"

#include <QVariant>
#include <QDebug>

#include "TMathBase.h"

ABranchBuffer::ABranchBuffer(const QString &branchName, const QString &branchType, TTree *tree) :
    name(branchName), type(branchType), treePtr(tree)
{
    tName = name.toLatin1().data();
    TString f = tName + "/" + type.toLatin1().data();

    if (type.length() == 1)
    {
        bVector = false;
        cType   = type.at(0).toLatin1();

        switch (cType)
        {
            case 'C' : branchPtr = tree->Branch(tName, &C, f); break;
            case 'I' : branchPtr = tree->Branch(tName, &I, f); break;
            case 'F' : branchPtr = tree->Branch(tName, &F, f); break;
            case 'D' : branchPtr = tree->Branch(tName, &D, f); break;
            case 'O' : branchPtr = tree->Branch(tName, &O, f); break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
    else if (type.startsWith("A"))
    {
        bVector = true;
        cType   = type.at(1).toLatin1();

        switch (cType)
        {
            case 'C' : branchPtr = tree->Branch(tName, &AC); break;
            case 'I' : branchPtr = tree->Branch(tName, &AI); break;
            case 'F' : branchPtr = tree->Branch(tName, &AF); break;
            case 'D' : branchPtr = tree->Branch(tName, &AD); break;
            case 'O' : branchPtr = tree->Branch(tName, &AO); break;
            default  : qWarning() << "Unknown tree branch type:" << type;
        }
    }
}

ABranchBuffer::ABranchBuffer(const QString &branchName, const QString &branchType, TBranch *branch)
{
    name  = branchName;
    type  = branchType;
    tName = branchName.toLatin1().data();
    treePtr = branch->GetTree();

    if (type.size() == 1)
    {
        if ( ABranchBuffer::getAllTypes().contains(type) )
        {
            //this is one of the basic types            
            bVector = false;
            cType   = type.at(0).toLatin1();
            branchPtr = branch; // non 0 -> indicates that the branch is valid
            switch (cType)
            {
                case 'C' : treePtr->SetBranchAddress(tName, &C); break;
                case 'I' : treePtr->SetBranchAddress(tName, &I); break;
                case 'F' : treePtr->SetBranchAddress(tName, &F); break;
                case 'D' : treePtr->SetBranchAddress(tName, &D); break;
                case 'O' : treePtr->SetBranchAddress(tName, &O); break;
                default  : branchPtr = 0; qWarning() << "Not implemented tree branch type:" << type; break;
            }
        }
    }
    else
    {
        if (type.contains("vector"))
        {
            type.remove("vector");
            type.remove("<");
            type.remove(">");

            bVector = true;
            branchPtr = branch; // non 0 -> indicates that the branch is valid

            if      (type == "string") { type = "AC"; pAC = &AC; treePtr->SetBranchAddress(tName, &pAC); }
            else if (type == "int")    { type = "AI"; pAI = &AI; treePtr->SetBranchAddress(tName, &pAI); }
            else if (type == "float")  { type = "AF"; pAF = &AF; treePtr->SetBranchAddress(tName, &pAF); }
            else if (type == "double") { type = "AD"; pAD = &AD; treePtr->SetBranchAddress(tName, &pAD); }
            else if (type == "bool")   { type = "AO"; pAO = &AO; treePtr->SetBranchAddress(tName, &pAO); }
            else                       { branchPtr = 0; qWarning() << "Not implemented tree branch type:" << branchType; }

            if (type.size() > 1) cType = type.at(1).toLatin1();

        }
        else if (type.startsWith("[") && type.contains("]"))
        {
            type.remove("[");
            QStringList sl = type.split("]", QString::SkipEmptyParts);
            if (sl.size() == 2)
            {
                bool bOK;
                QString sizeStr = sl.at(0);
                QString typeStr = sl.at(1);

                int size = sizeStr.toInt(&bOK);

                cType = typeStr.at(0).toLatin1();
                type = QString("A") + typeStr;

                bCanFill = false; // cannot fill new entries!

                bVector = true;
                if (bOK && ABranchBuffer::getAllTypes().contains(type))
                {
                    qDebug() << type << cType << size;
                    branchPtr = branch; // non 0 -> indicates that the branch is valid
                    switch (cType)
                    {
                        case 'C' : AC.resize(size); treePtr->SetBranchAddress(tName, AC.data()); break;
                        case 'I' : AI.resize(size); treePtr->SetBranchAddress(tName, AI.data()); break;
                        case 'F' : AF.resize(size); treePtr->SetBranchAddress(tName, AF.data()); break;
                        case 'D' : AD.resize(size); treePtr->SetBranchAddress(tName, AD.data()); break;
                        case 'O' : Ao.resize(size); treePtr->SetBranchAddress(tName, Ao.data()); cType = 'o'; break;
                        default  : branchPtr = 0; qWarning() << "Not implemented tree branch type:" << branchType; break;
                    }

                }
            }

        }
    }
}

void ABranchBuffer::write(const QVariant &val)
{
    if (!bVector)
    {
        switch (cType)
        {
            //case 'C' : C = val.toString().toLatin1().data(); break;
            case 'C' :
            {
                TString tmp = val.toString().toLatin1().data();
                for (int i = 0; i < tmp.Length()+1; i++) C[i] = tmp[i]; //including termination
                break;
            }
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
            case 'o' : for (const bool& e : Ao) vl << e; return vl;
            default  : qWarning() << "read - unknown tree branch type:" << type;
        }
    }
    return 0;
}

ARootTreeRecord::ARootTreeRecord(TObject *tree, const QString &name) :
    ARootObjBase(tree, name, "") {}

ARootTreeRecord::~ARootTreeRecord()
{
    if (file)
    {
        TTree* t = dynamic_cast<TTree*>(Object);
        if (t)
        {
            t->AutoSave();
            delete t; Object = 0;
        }
        file->Close();
        delete file; file = 0;
    }

    for (ABranchBuffer* bb : Branches) delete bb;
    Branches.clear();
    MapOfBranches.clear();
}

#include "TFile.h"
bool ARootTreeRecord::createTree(const QString &name, const QVector<QPair<QString, QString> > &branches,
                                 const QString fileName, int autosaveNum)
{
    qDebug() << fileName;
    QMutexLocker locker(&Mutex);

    Branches.resize(branches.size());
    MapOfBranches.clear();
    if (Object) delete Object;

    if (!fileName.isEmpty())
    {
        file = new TFile(fileName.toLatin1().data(), "RECREATE");
    }

    TTree* t = new TTree(name.toLatin1().data(), "");
    Object = t;

    if (file)
    {
        t->SetDirectory(file);
        t->SetMaxVirtualSize(0);
        t->SetAutoSave(autosaveNum);
    }

    for (int ib = 0; ib < branches.size(); ib++)
    {        
        const QString& Bname = branches.at(ib).first;
        const QString& Btype = branches.at(ib).second;

        ABranchBuffer* bb = new ABranchBuffer(Bname, Btype, t);
        if (bb->isValid())
        {
            Branches[ib] = bb;
            MapOfBranches.insert( Bname, bb );
        }
        else
        {
            delete bb;
            return false;
        }
    }

    if (file) t->AutoSave();

    return true;
}

const QString ARootTreeRecord::loadTree(const QString &fileName, const QString treeNameInFile)
{
    TFile *f = TFile::Open(fileName.toLocal8Bit().data(), "READ");
    if (!f) return QString("Cannot open file ") + fileName;

    TTree *t = 0;
    if (treeNameInFile.isEmpty())
    {
        const int numKeys = f->GetListOfKeys()->GetEntries();
        for (int ikey = 0; ikey < numKeys; ikey++)
        {
            TKey *key = (TKey*)f->GetListOfKeys()->At(ikey);
            qDebug() << "Key->  name:" << key->GetName() << " class:" << key->GetClassName() <<" title:"<< key->GetTitle();

            const QString Type = key->GetClassName();
            if (Type != "TTree") continue;
            t = dynamic_cast<TTree*>(key->ReadObj());
            if (t) break;
        }
        if (!t)
        {
            delete f;
            return QString("No trees found in file ") + fileName;
        }
    }
    else
    {
        f->GetObject(treeNameInFile.toLocal8Bit().data(), t);
        if (!t)
        {
            delete f;
            return QString("Tree ") + treeNameInFile + " not found in file " + fileName;
        }
    }

    t->Print();

    Mutex.lock();
        if (Object) delete Object;
        Object = t;
    Mutex.unlock();

    QString error = resetTreeRecords();
    if (!error.isEmpty())
    {
        delete t;
        delete f;
        error += " for tree in file " + fileName;
    }
    return error;

    /*
    Branches.clear();
    MapOfBranches.clear();

    const int numBranches = t->GetNbranches();
    TObjArray* lb = t->GetListOfBranches();

    for (int ibranch = 0; ibranch < numBranches; ibranch++)
    {
        TBranch* branchPtr = (TBranch*)(lb->At(ibranch));
        QString branchName = branchPtr->GetName();
        QString branchType = branchPtr->GetClassName();
        if (branchType.isEmpty())
        {
            QString title = branchPtr->GetTitle();
            title.remove(branchName);
            title.remove("/");
            branchType = title;
        }
        // else    -> vector<T> is here
        qDebug() << branchName << branchType << branchPtr;

        ABranchBuffer* bb = new ABranchBuffer(branchName, branchType, branchPtr);
        if (bb->isValid())
        {
            Branches << bb;
            MapOfBranches.insert( branchName, bb );
            if (!bb->canFill()) canAddEntries = false;
        }
        else
        {
            delete bb;
            delete t;
            delete f;
            branchType.replace("<", "(");
            branchType.replace(">", ")");
            return QString("Unsupported branch type ") + branchType + " for tree in file " + fileName;
        }
    }
    return "";
    */
}

const QString ARootTreeRecord::resetTreeRecords()
{
    QMutexLocker locker(&Mutex);

    Branches.clear();
    MapOfBranches.clear();
    canAddEntries = true;

    TTree* t = static_cast<TTree*>(Object);

    const int numBranches = t->GetNbranches();
    TObjArray* lb = t->GetListOfBranches();

    for (int ibranch = 0; ibranch < numBranches; ibranch++)
    {
        TBranch* branchPtr = (TBranch*)(lb->At(ibranch));
        QString branchName = branchPtr->GetName();
        QString branchType = branchPtr->GetClassName();
        if (branchType.isEmpty())
        {
            QString title = branchPtr->GetTitle();
            title.remove(branchName);
            title.remove("/");
            branchType = title;
        }
        // else    -> vector<T> is here
        qDebug() << branchName << branchType << branchPtr;

        ABranchBuffer* bb = new ABranchBuffer(branchName, branchType, branchPtr);
        if (bb->isValid())
        {
            Branches << bb;
            MapOfBranches.insert( branchName, bb );
            if (!bb->canFill()) canAddEntries = false;
        }
        else
        {
            delete bb;
            branchType.replace("<", "(");
            branchType.replace(">", ")");
            return QString("Unsupported branch type: ") + branchType;
        }
    }
    return "";
}

int ARootTreeRecord::countBranches() const
{
    QMutexLocker locker(&Mutex);
    return Branches.size();
}

const QStringList ARootTreeRecord::getBranchNames() const
{
    QStringList sl;
    for (ABranchBuffer* bb : Branches)
    {
        if (!bb->branchPtr) return QStringList();
        sl << bb->name;
    }
    return sl;
}

const QStringList ARootTreeRecord::getBranchTypes() const
{
    QStringList sl;
    for (ABranchBuffer* bb : Branches)
    {
        if (!bb->branchPtr) return QStringList();
        sl << bb->type;
    }
    return sl;
}

int ARootTreeRecord::countEntries() const
{
    QMutexLocker locker(&Mutex);

    return static_cast<TTree*>(Object)->GetEntries();
}

bool ARootTreeRecord::fillSingle(const QVariantList &vl)
{
    QMutexLocker locker(&Mutex);

    if (!canAddEntries)
    {
        qDebug() << "This tree has branch(es) which cannot be filled from script!";
        return false;
    }

    if (vl.size() != Branches.size()) return false;

    for (int ib = 0; ib < Branches.size(); ib++)
    {
        ABranchBuffer* bb = Branches[ib];
        bb->write(vl.at(ib));
    }
    TTree* t = static_cast<TTree*>(Object);
    t->Fill();

    return true;
}

bool ARootTreeRecord::isBranchExist(const QString &branchName) const
{
    QMutexLocker locker(&Mutex);

    return MapOfBranches.value(branchName, 0);
}

const QVariantList ARootTreeRecord::getBranch(const QString &branchName)
{
    QVariantList res;

    ABranchBuffer* bb = MapOfBranches.value(branchName, 0);
    if (bb)
    {
        if (bb->branchPtr)
        {
            const int numEntries = bb->branchPtr->GetEntries();
            for (int i = 0; i < numEntries; i++)
            {
                Long64_t tentry = bb->treePtr->LoadTree(i);
                bb->branchPtr->GetEntry(tentry);
                res.push_back(bb->read());
            }
        }
    }
    return res;
}

const QVariant ARootTreeRecord::getBranch(const QString &branchName, int entry)
{
    ABranchBuffer* bb = MapOfBranches.value(branchName, 0);
    if (bb)
    {
        if (bb->branchPtr)
        {
            const int numEntries = bb->branchPtr->GetEntries();
            if (entry >= 0 && entry < numEntries)
            {
                const Long64_t tentry = bb->treePtr->LoadTree(entry);
                bb->branchPtr->GetEntry(tentry);
                return bb->read();
            }
        }
    }

    return QVariant();
}

const QVariantList ARootTreeRecord::getEntry(int entry)
{
    QVariantList res;

    for (int ibranch = 0; ibranch < Branches.size(); ibranch++)
    {
         ABranchBuffer* bb = Branches[ibranch];
         if (bb->branchPtr)
         {
             const int numEntries = bb->branchPtr->GetEntries();
             if (entry >= 0 && entry < numEntries)
             {
                 const Long64_t tentry = bb->treePtr->LoadTree(entry);
                 bb->branchPtr->GetEntry(tentry);
                 res.push_back( bb->read() );
             }
             else res.push_back( QVariant() );
         }
         else return QVariantList();
    }

    return res;
}

void ARootTreeRecord::save(const QString &FileName)
{
    TTree* t = static_cast<TTree*>(Object);
    t->SaveAs(FileName.toLatin1().data());
}

void ARootTreeRecord::flush()
{
    if (file)
    {
        TTree* t = static_cast<TTree*>(Object);
        //t->Write();
        t->AutoSave();
    }
}

void ARootTreeRecord::setAutoSave(int autosaveAfterEntriesWritten)
{
    TTree* t = static_cast<TTree*>(Object);
    t->SetAutoSave(autosaveAfterEntriesWritten);
}
