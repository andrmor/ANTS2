#include "ainterfacetottree.h"
#include "aroottreerecord.h"
#include "tmpobjhubclass.h"

#include <QDebug>

#include "TFile.h"
//#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"

AInterfaceToTTree::AInterfaceToTTree(TmpObjHubClass *TmpHub) :
    TmpHub(TmpHub)
{
    Description = "Interface to CERN ROOT Trees";

    H["LoadTree"] = "If the third argument is not provided, the first tree found in the file is loaded";
    H["NewTree"] = "Branch types:\n"
            "C - string\n"
            "I - int\n"
            "F - float\n"
            "D - double\n"
            "O - bool\n"
            "AC - vector of strings\n"
            "AI - vector of ints\n"
            "AF - vector of floats\n"
            "AD - vector of doubles\n"
            "AO - vector of bools";
}

void AInterfaceToTTree::LoadTree(const QString& TreeName, const QString& FileName, const QString TreeNameInFile)
{
    if (!bGuiThread)
    {
        abort("Can load TTree only in main thread!");
        return;
    }

    ARootTreeRecord* rec = new ARootTreeRecord(0, TreeName);
    QString ErrorString = rec->loadTree(FileName, TreeNameInFile);
    if (ErrorString.isEmpty())
    {
        bool bOK = TmpHub->Trees.append(TreeName, rec, bAbortIfExists);
        if (!bOK)
        {
            delete rec;
            abort("Tree " + TreeName + " already exists!");
        }
    }
    else abort("Failed to create tree "+ TreeName + ": " + ErrorString);
}

void AInterfaceToTTree::NewTree(const QString &TreeName, const QVariant HeadersOfBranches,
                                const QString StoreInFileName, int AutosaveAfterEntriesAdded)
{
    if (!bGuiThread)
    {
        abort("Can load TTree only in main thread!");
        return;
    }

    if ( IsTreeExists(TreeName) )
    {
        if (bAbortIfExists)
        {
            abort("Tree " + TreeName + " already exists!");
            return;
        }
        DeleteTree(TreeName); // need to delete first -> cannot just replace because of save to file root mechanism
    }

    const QVariantList headersVL = HeadersOfBranches.toList();
    if (headersVL.size() < 1)
    {
        abort("CreateTree() requires array of arrays as the second argument");
        return;
    }

    QVector<QPair<QString, QString>> h;
    for (int ibranch = 0; ibranch < headersVL.size(); ibranch++)
    {
        QVariantList th = headersVL.at(ibranch).toList();
        if (th.size() != 2)
        {
            abort("CreateTree() headers should be array of [Name,Type] values");
            return;
        }

        QString Bname = th.at(0).toString();
        QString Btype = th.at(1).toString();
        if (!ABranchBuffer::getAllTypes().contains(Btype))
        {
            abort("CreateTree() header contain unknown branch type: " + Btype );
            return;
        }

        h << QPair<QString, QString>(Bname, Btype);
    }

    ARootTreeRecord* rec = new ARootTreeRecord(0, TreeName);
    bool bOK = rec->createTree(TreeName, h, StoreInFileName, AutosaveAfterEntriesAdded);
    if (bOK)
    {
        bOK = TmpHub->Trees.append(TreeName, rec, bAbortIfExists);
        if (!bOK) // paranoic - should not happen
        {
            delete rec;
            abort("Tree " + TreeName + " already exists!");
        }
    }
    else abort("Failed to create tree: " + TreeName);
}

void AInterfaceToTTree::Fill(const QString &TreeName, const QVariant Array)
{
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        abort("Tree "+TreeName+" not found!");
    else
    {
        const QVariantList vl = Array.toList();
        const bool bOK = r->fillSingle(vl);
        if (!bOK)
            abort("Fill failed");
    }
}

int AInterfaceToTTree::GetNumEntries(const QString &TreeName) const
{
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        return 0;
    else
        return r->countEntries();
}

const QVariant AInterfaceToTTree::GetBranchNames(const QString &TreeName) const
{
    QVariantList vl;
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (r)
    {
        QStringList sl = r->getBranchNames();
        for (const QString& s : sl) vl << s;
    }
    return vl;
}

const QVariant AInterfaceToTTree::GetBranchTypes(const QString &TreeName) const
{
    QVariantList vl;
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (r)
    {
        QStringList sl = r->getBranchTypes();
        for (const QString& s : sl) vl << s;
    }
    return vl;
}

const QVariant AInterfaceToTTree::GetAllTreeNames() const
{
    QStringList sl = TmpHub->Trees.getAllRecordNames();

    QVariantList vl;
    for (const QString& s : sl) vl << s;
    return vl;
}

bool AInterfaceToTTree::IsTreeExists(const QString &TreeName) const
{
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    return r;
}

void AInterfaceToTTree::ResetTreeAddresses(const QString &TreeName)
{
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        abort("Tree "+TreeName+" not found!");
    else
        r->resetTreeRecords();
}

/*
void AInterfaceToTTree::Scan(const QString& TreeName, const QString& arg1, const QString& arg2, const QString& arg3)
{
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        abort("Tree "+TreeName+" not found!");
    else
        r->scan(arg1, arg2, arg3);
}
*/

/*
const QString AInterfaceToTTree::GetTreeStructure(const QString& TreeName)
{
    ARootObjBase* r = TmpHub->Trees.getRecord(TreeName);
    if (!r)
    {
        abort("Tree " + TreeName + " not found!");
        return "";
    }
    TTree *t = static_cast<TTree*>(r->GetObject());

    QString s = "Thee ";
    s += TreeName;
    s += " has the following branches (-> data_type):<br>";
    for (int i=0; i<t->GetNbranches(); i++)
    {
        TObjArray* lb = t->GetListOfBranches();
        const TBranch* b = (const TBranch*)(lb->At(i));
        QString name = b->GetName();
        s += name;
        s += " -> ";
        QString type = b->GetClassName();
        if (type.isEmpty())
        {
            QString title = b->GetTitle();
            title.remove(name);
            title.remove("/");
            s += title;
        }
        else
        {
            type.replace("<", "(");
            type.replace(">", ")");
            s += type;
        }
        s += "<br>";
    }
    return s;
}
*/

const QVariant AInterfaceToTTree::GetBranch(const QString& TreeName, const QString& BranchName)
{
    QVariantList res;
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        abort("Tree " + TreeName + " not found!");
    else
    {
        if (!r->isBranchExist(BranchName))
            abort("Tree " + TreeName + " does not have branch " + BranchName);
        else
            res = r->getBranch(BranchName);
    }
    return res;

    /*
    TTree *t = static_cast<TTree*>(r->GetObject());

    TBranch* branch = t->GetBranch(BranchName.toLocal8Bit().data());
    if (!branch)
    {
        abort("Tree " + TreeName + " does not have branch " + BranchName);
        return QVariant();
    }

    int numEntries = branch->GetEntries();
    qDebug() << "The branch contains:" << numEntries << "elements";

    QList< QVariant > varList;
    QString type = branch->GetClassName();
    qDebug() << "Element type:" << type;
    if (type == "vector<double>")
    {
        std::vector<double> *v = 0;
        t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);

        for (Int_t i = 0; i < numEntries; i++)
        {
            Long64_t tentry = t->LoadTree(i);
            branch->GetEntry(tentry);
            QList<QVariant> ll;
            for (UInt_t j = 0; j < v->size(); ++j)
                ll.append( (*v)[j] );
            QVariant r = ll;
            varList << r;
        }
    }
    else if (type == "vector<float>")
    {
        std::vector<float> *v = 0;
        t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);

        for (Int_t i = 0; i < numEntries; i++)
        {
            Long64_t tentry = t->LoadTree(i);
            branch->GetEntry(tentry);
            QList<QVariant> ll;
            for (UInt_t j = 0; j < v->size(); ++j)
                ll.append( (*v)[j] );
            QVariant r = ll;
            varList << r;
        }
    }
    else if (type == "vector<int>")
    {
        std::vector<int> *v = 0;
        t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);

        for (Int_t i = 0; i < numEntries; i++)
        {
            Long64_t tentry = t->LoadTree(i);
            branch->GetEntry(tentry);
            QList<QVariant> ll;
            for (UInt_t j = 0; j < v->size(); ++j)
                ll.append( (*v)[j] );
            QVariant r = ll;
            varList << r;
        }
    }
    else if (type == "")
    {
        //have to use another system
        QString title = branch->GetTitle();  //  can be, e.g., "blabla/D" or "signal[19]/F"

        if (title.contains("["))
        {
            qDebug() << "Array of data"<<title;
            QRegExp selector("\\[(.*)\\]");
            selector.indexIn(title);
            QStringList List = selector.capturedTexts();
            if (List.size()!=2)
            {
               abort("Cannot extract the length of the array");
               return QVariant();
            }
            else
            {
               QString s = List.at(1);
               bool fOK = false;
               int numInArray = s.toInt(&fOK);
               if (!fOK)
               {
                  abort("Cannot extract the length of the array");
                  return QVariant();
               }
               qDebug() << "in the array there are"<<numInArray<<"elements";

               //type dependent too!
               if (title.contains("/I"))
               {
                   qDebug() << "It is an array with ints";
                   int *array = new int[numInArray];
                   t->SetBranchAddress(BranchName.toLocal8Bit().data(), array, &branch);

                   for (Int_t i = 0; i < numEntries; i++)
                   {
                       Long64_t tentry = t->LoadTree(i);
                       branch->GetEntry(tentry);
                       QList<QVariant> ll;
                       for (int j = 0; j < numInArray; j++)
                           ll.append( array[j] );
                       QVariant r = ll;
                       varList << r;
                   }
                   delete [] array;
               }
               else if (title.contains("/D"))
               {
                   qDebug() << "It is an array with doubles";
                   double *array = new double[numInArray];
                   t->SetBranchAddress(BranchName.toLocal8Bit().data(), array, &branch);

                   for (Int_t i = 0; i < numEntries; i++)
                   {
                       Long64_t tentry = t->LoadTree(i);
                       branch->GetEntry(tentry);
                       QList<QVariant> ll;
                       for (int j = 0; j < numInArray; j++)
                           ll.append( array[j] );
                       QVariant r = ll;
                       varList << r;
                   }
                   delete [] array;
               }
               else if (title.contains("/F"))
               {
                   qDebug() << "It is an array with floats";
                   float *array = new float[numInArray];
                   t->SetBranchAddress(BranchName.toLocal8Bit().data(), array, &branch);

                   for (Int_t i = 0; i < numEntries; i++)
                   {
                       Long64_t tentry = t->LoadTree(i);
                       branch->GetEntry(tentry);
                       QList<QVariant> ll;
                       for (int j = 0; j < numInArray; j++)
                           ll.append( array[j] );
                       QVariant r = ll;
                       varList << r;
                   }
                   delete [] array;
               }
               else
               {
                   abort("Cannot extract the type of the array");
                   return QVariant();
               }
            }
        }
        else if (title.contains("/I"))
        {
            qDebug() << "Int data - scalar";
            int v = 0;
            t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);
            for (Int_t i = 0; i < numEntries; i++)
            {
                Long64_t tentry = t->LoadTree(i);
                branch->GetEntry(tentry);
                varList.append(v);
            }
        }
        else if (title.contains("/D"))
        {
            qDebug() << "Double data - scalar";
            double v = 0;
            t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);
            for (Int_t i = 0; i < numEntries; i++)
            {
                Long64_t tentry = t->LoadTree(i);
                branch->GetEntry(tentry);
                varList.append(v);
            }
        }
        else if (title.contains("/F"))
        {
            qDebug() << "Float data - scalar";
            float v = 0;
            t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);
            for (Int_t i = 0; i < numEntries; i++)
            {
                Long64_t tentry = t->LoadTree(i);
                branch->GetEntry(tentry);
                varList.append(v);
            }
        }
        else
        {
            abort("Unsupported data type of the branch - title is: "+title);
            return QVariant();
        }

    }
    else
    {
        abort("Tree branch type " + type + " is not supported");
        return QVariant();
    }

    t->ResetBranchAddresses();
    return varList;
    */
}

const QVariant AInterfaceToTTree::GetBranch(const QString &TreeName, const QString &BranchName, int EntryIndex)
{
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        abort("Tree " + TreeName + " not found!");
    else
    {
        if (!r->isBranchExist(BranchName))
            abort("Tree " + TreeName + " does not have branch " + BranchName);
        else
            return r->getBranch(BranchName, EntryIndex);
    }
    return QVariant();
}

const QVariant AInterfaceToTTree::GetEntry(const QString &TreeName, int EntryIndex)
{
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
    {
        abort("Tree " + TreeName + " not found!");
        return 0;
    }
    else
        return r->getEntry(EntryIndex);
}

const QVariantList assertBinsAndRanges(const QVariant& in)
{
    QVariantList out;
    bool bOK;

    QVariantList inVL = in.toList();
    if (inVL.size() == 3)
    {
        int bins = inVL.at(0).toInt(&bOK);
        if (!bOK) bins = 100;
        double from = inVL.at(1).toDouble(&bOK); if (!bOK) from = 0;
        double to   = inVL.at(2).toDouble(&bOK); if (!bOK) to   = 0;
        out << bins << from << to;
    }
    else out << 100 << 0.0 << 0.0;
    return out;
}

const QVariantList assertMarkerLine(const QVariant& in)
{
    QVariantList out;
    bool bOK;

    QVariantList inVL = in.toList();
    if (inVL.size() == 3)
    {
        int color = inVL.at(0).toInt(&bOK);
        if (!bOK) color = 602;
        double style = inVL.at(1).toInt(&bOK);    if (!bOK) style = 1;
        double size  = inVL.at(2).toDouble(&bOK); if (!bOK) size  = 1.0;
        out << color << style << size;
    }
    else out << 602 << 1 << 1.0;
    return out;
}

const QString AInterfaceToTTree::Draw(const QString &TreeName, const QString &what, const QString &cuts, const QString &options,
                                      const QVariant binsAndRanges, const QVariant markerAndLineAttributes, bool AbortIfFailedToDraw)
{
    if (!bGuiThread)
    {
        abort("Threads cannot draw!");
        return "";
    }

    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
    {
        abort("Tree "+TreeName+" not found!");
        return "";
    }
    else
    {
        QVariantList vlInBR = binsAndRanges.toList();
        QVariantList outBR;
        for (int i = 0; i < 3; i++)
        {
            QVariantList el = assertBinsAndRanges( i < vlInBR.size() ? vlInBR.at(i) : 0 );
            outBR.push_back( el );
        }
        QVariantList vlInML = markerAndLineAttributes.toList();
        QVariantList outML;
        for (int i = 0; i < 2; i++)
        {
            QVariantList el = assertMarkerLine( i < vlInML.size() ? vlInML.at(i) : 0 );
            outML.push_back( el );
        }

        QString error;
        r->externalLock();
        TTree* t = static_cast<TTree*>(r->GetObject());
        emit RequestTreeDraw(t, what, cuts, options, outBR, outML, &error);
        r->externalUnlock();

        //r->resetTreeRecords();

        if (AbortIfFailedToDraw && !error.isEmpty())
            abort("Tree Draw error -> " + error);

        return error;
    }
}

void AInterfaceToTTree::Save(const QString &TreeName, const QString &FileName)
{
    if (!bGuiThread)
    {
        abort("Threads cannot save tree!");
        return;
    }

    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        abort("Tree "+TreeName+" not found!");
    else
    {
        r->save(FileName);
        const QString err = r->resetTreeRecords();
        if (!err.isEmpty())
            abort("Could not recover tree after save:\n" + err);
    }
}

void AInterfaceToTTree::FlushToFile(const QString &TreeName)
{
    if (!bGuiThread)
    {
        abort("Threads cannot close trees!");
        return;
    }

    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        abort("Tree "+TreeName+" not found!");
    else
    {
        bool bOK = r->autoSave();
        if (bOK) ;
        else abort("Tree " + TreeName + " does not support autosave to file.\nThis feature requires the tree to be created with non-empty file name argument.");
    }
}

/*
void AInterfaceToTTree::SetAutoSave(const QString &TreeName, int AutoSaveAfterEntriesAdded)
{
    ARootTreeRecord* r = dynamic_cast<ARootTreeRecord*>(TmpHub->Trees.getRecord(TreeName));
    if (!r)
        abort("Tree "+TreeName+" not found!");
    else
        r->setAutoSave(AutoSaveAfterEntriesAdded);
}
*/

bool AInterfaceToTTree::DeleteTree(const QString& TreeName)
{
    return TmpHub->Trees.remove(TreeName);
}

void AInterfaceToTTree::DeleteAllTrees()
{
    TmpHub->Trees.clear();
}
