#include "adrawexplorerwidget.h"
#include "arootmarkerconfigurator.h"
#include "arootlineconfigurator.h"
#include "amessage.h"
#include "graphwindowclass.h"
#include "afiletools.h"
#include "alinemarkerfilldialog.h"

#ifdef USE_EIGEN
    #include "curvefit.h"
#endif

#include <QDebug>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QInputDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>
#include <QColor>

#include "TObject.h"
#include "TNamed.h"
#include "TAttMarker.h"
#include "TAttLine.h"
#include "TAttFill.h"
#include "TH1.h"
#include "TH2.h"
#include "TF2.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TPaveText.h"
#include "TGaxis.h"
#include "TColor.h"
#include "TROOT.h"

ADrawExplorerWidget::ADrawExplorerWidget(GraphWindowClass & GraphWindow, QVector<ADrawObject> & DrawObjects) :
    GraphWindow(GraphWindow), DrawObjects(DrawObjects)
{
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ADrawExplorerWidget::customContextMenuRequested, this, &ADrawExplorerWidget::onContextMenuRequested);
    connect(this, &ADrawExplorerWidget::itemDoubleClicked, this, &ADrawExplorerWidget::onItemDoubleClicked);

    setIndentation(0);
}

void ADrawExplorerWidget::updateGui()
{
    clear();

    for (int i = 0; i < DrawObjects.size(); i++)
    {
        const ADrawObject & drObj = DrawObjects.at(i);
        TObject * tObj = drObj.Pointer;

        QTreeWidgetItem * item = new QTreeWidgetItem();
        QString name = drObj.Name;
        //if (name.isEmpty()) name = "--";

        QString nameShort = name;
        if (nameShort.size() > 15)
        {
            nameShort.truncate(15);
            nameShort += "..";
        }

        QString className = tObj->ClassName();
        QString opt = drObj.Options;

        if (className == "TPaveText") className = "TextBox";
        else if (className == "TLegend") className = "Legend";

        item->setText(0, QString("%1 %2").arg(nameShort).arg(className));
        item->setToolTip(0, QString("Name: %1\nClassName: %2\nDraw options: %3").arg(name).arg(className).arg(opt));
        item->setText(1, QString::number(i));

        if (!drObj.bEnabled) item->setForeground(0, QBrush(Qt::red));

        QIcon icon;
        constructIconForObject(icon, drObj);
        item->setIcon(0, icon);

        addTopLevelItem(item);
    }

    objForCustomProjection = nullptr;
}

void ADrawExplorerWidget::onContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem * item = currentItem();
    if (!item) return;

    int index = item->text(1).toInt();
    if (index < 0 || index >= DrawObjects.size())
    {
        qWarning() << "Corrupted model: invalid index extracted";
        return;
    }

    showObjectContextMenu(mapToGlobal(pos), index);
}

void ADrawExplorerWidget::showObjectContextMenu(const QPoint &pos, int index)
{
    if (index < 0 || index >= DrawObjects.size())
        return;

    ADrawObject & obj = DrawObjects[index];
    const QString Type = obj.Pointer->ClassName();

    QMenu Menu;

    QAction * editTextPave = ( Type == "TPaveText" ? Menu.addAction("Edit") : nullptr );
    if (editTextPave) Menu.addSeparator();

    QAction * renameA =     Menu.addAction("Rename");

    Menu.addSeparator();

    QAction * enableA =     Menu.addAction("Toggle enabled/disabled"); enableA->setChecked(obj.bEnabled); enableA->setEnabled(index != 0);

    Menu.addSeparator();

    QAction * delA =        Menu.addAction("Delete"); delA->setEnabled(index != 0);

    Menu.addSeparator();

    QAction * setAttrib =   Menu.addAction("Draw attributes");

    Menu.addSeparator();

    QAction* axesX =        Menu.addAction("X axis");
    QAction* axesY =        Menu.addAction("Y axis");
    QAction* axesZ =        Menu.addAction("Z axis");

    Menu.addSeparator();

    QAction* addAxisRight = Menu.addAction("Add axis on RHS");
    QAction* addAxisTop   = Menu.addAction("Add axis on Top");

    Menu.addSeparator();

    QMenu * scaleMenu =     Menu.addMenu("Scale / shift"); scaleMenu->setEnabled(Type.startsWith("TH") || Type.startsWith("TGraph") || Type.startsWith("TProfile"));
        QAction * scaleA =      scaleMenu->addAction("Scale");
        QAction * scaleCDRA =   scaleMenu->addAction("Scale: click-drag-release");
        scaleMenu->addSeparator();
        QAction * shiftA =      scaleMenu->addAction("Shift X scale");

    Menu.addSeparator();

    QMenu * manipMenu =     Menu.addMenu("Manipulate histogram"); manipMenu->setEnabled(Type.startsWith("TH1"));
        QAction* interpolateA = manipMenu->addAction("Interpolate");
        manipMenu->addSeparator();
        QAction* medianA =      manipMenu->addAction("Apply median filter");

    Menu.addSeparator();

    QMenu * projectMenu =   Menu.addMenu("Projection"); projectMenu->setEnabled(Type.startsWith("TH2"));
        QAction* projX =        projectMenu->addAction("X projection");
        QAction* projY =        projectMenu->addAction("Y projection");
        projectMenu->addSeparator();
        QAction* projCustom =   projectMenu->addAction("Show projection tool");

    Menu.addSeparator();

    QMenu * calcMenu =     Menu.addMenu("Calculate"); calcMenu->setEnabled(Type.startsWith("TH1") || Type == "TProfile");
        QAction* integralA =    calcMenu->addAction("Draw integral");
        QAction* fractionA =    calcMenu->addAction("Calculate fraction before/after");

    Menu.addSeparator();

    QMenu * fitMenu =       Menu.addMenu("Fit");
        QAction* linFitA    =   fitMenu->addAction("Linear (use click-drag)");     linFitA->setEnabled(Type.startsWith("TH1") || Type == "TProfile" || Type.startsWith("TGraph"));
        QAction* fwhmA      =   fitMenu->addAction("Gauss (use click-frag)");      fwhmA->  setEnabled(Type.startsWith("TH1") || Type == "TProfile" || Type.startsWith("TGraph"));
        QAction* expA       =   fitMenu->addAction("Exp. decay (use click-frag)"); expA->   setEnabled(Type.startsWith("TH1") || Type == "TProfile");
        QAction* splineFitA =   fitMenu->addAction("B-spline"); splineFitA->setEnabled(Type == "TGraph" || Type == "TGraphErrors");   //*** implement for TH1 too!
        fitMenu->addSeparator();
        QAction* showFitPanel = fitMenu->addAction("Show fit panel");

    Menu.addSeparator();

    QMenu * saveMenu =      Menu.addMenu("Save");
        QAction* saveRootA =    saveMenu->addAction("Save ROOT object");
        saveMenu->addSeparator();
        QAction* saveTxtA =     saveMenu->addAction("Save as text");
        QAction* saveEdgeA =    saveMenu->addAction("Save hist as text using bin edges");

    Menu.addSeparator();

    QAction * extractA =    Menu.addAction("Extract object"); extractA->setEnabled(DrawObjects.size() > 1);

    // ------ exec ------

    QAction* si = Menu.exec(pos);
    if (!si) return; //nothing was selected

   if      (si == renameA)      rename(obj);
   else if (si == enableA)      toggleEnable(obj);
   else if (si == delA)         remove(index);
   else if (si == setAttrib)    setAttributes(index);
   else if (si == scaleA)       scale(obj);
   else if (si == scaleCDRA)    scaleCDR(obj);
   else if (si == integralA)    drawIntegral(obj);
   else if (si == fractionA)    fraction(obj);
   else if (si == linFitA)      linFit(index);
   else if (si == fwhmA)        fwhm(index);
   else if (si == expA)         expFit(index);
   else if (si == interpolateA) interpolate(obj);
   else if (si == axesX)        editAxis(obj, 0);
   else if (si == axesY)        editAxis(obj, 1);
   else if (si == axesZ)        editAxis(obj, 2);
   else if (si == addAxisTop)   addAxis(0);
   else if (si == addAxisRight) addAxis(1);
   else if (si == shiftA)       shift(obj);
   else if (si == medianA)      median(obj);
   else if (si == splineFitA)   splineFit(index);
   else if (si == projX)        projection(obj, true);
   else if (si == projY)        projection(obj, false);
   else if (si == saveRootA)    saveRoot(obj);
   else if (si == saveTxtA)     saveAsTxt(obj, true);
   else if (si == saveEdgeA)    saveAsTxt(obj, false);
   else if (si == projCustom)   customProjection(obj);
   else if (si == showFitPanel) fitPanel(obj);
   else if (si == extractA)     extract(obj);
   else if (si == editTextPave) editPave(obj);

}

void ADrawExplorerWidget::manipulateTriggered()
{
    const int size = DrawObjects.size();
    const QPoint pos = mapToGlobal(QPoint(0, 0));

    if      (size == 0) {}
    else if (size == 1) showObjectContextMenu(pos, 0); //showing context menu of the first drawn object
    else
    {
        ADrawObject & obj = DrawObjects[0];
        const QString Type = obj.Pointer->ClassName();

        QMenu Menu;

        QAction* axesX =        Menu.addAction("X axis");
        QAction* axesY =        Menu.addAction("Y axis");
        QAction* axesZ =        Menu.addAction("Z axis");

        Menu.addSeparator();

        QAction* addAxisRight = Menu.addAction("Add axis on RHS");
        QAction* addAxisTop   = Menu.addAction("Add axis on Top");

        Menu.addSeparator();

        //QMenu * scaleMenu =     Menu.addMenu("Scale / shift"); scaleMenu->setEnabled(Type.startsWith("TH") || Type.startsWith("TGraph") || Type.startsWith("TProfile"));
        //    QAction * scaleA =      scaleMenu->addAction("Scale all graphs/histograms to the same max");
        //    QAction * scaleCDRA =   scaleMenu->addAction("Scale: click-drag-release");
        //    scaleMenu->addSeparator();
        //    QAction * shiftA =      scaleMenu->addAction("Shift X scale");

        QAction* scale        = Menu.addAction("Scale all graphs/histograms to the same max"); scale->setEnabled(Type.startsWith("TH") || Type.startsWith("TGraph") || Type.startsWith("TProfile"));


        // ------ exec ------

        QAction* si = Menu.exec(pos);
        if (!si) return; //nothing was selected


        if      (si == axesX)        editAxis(obj, 0);
        else if (si == axesY)        editAxis(obj, 1);
        else if (si == axesZ)        editAxis(obj, 2);
        else if (si == addAxisTop)   addAxis(0);
        else if (si == addAxisRight) addAxis(1);
        else if (si == scale)        scaleAllSameMax();
    }
}

void ADrawExplorerWidget::onItemDoubleClicked(QTreeWidgetItem *item, int)
{
    if (!item) return;
    activateCustomGuiForItem(item->text(1).toInt());
}

void ADrawExplorerWidget::activateCustomGuiForItem(int index)
{
    if (index < 0 || index >= DrawObjects.size())
    {
        qWarning() << "Invalid model index!";
        return;
    }

    ADrawObject & obj = DrawObjects[index];
    const QString Type = obj.Pointer->ClassName();

    if      (Type == "TLegend")   emit requestShowLegendDialog();
    else if (Type == "TPaveText") editPave(obj);
    else if (Type == "TGaxis")    editTGaxis(obj);
    else                          setAttributes(index);
}

void ADrawExplorerWidget::addToDrawObjectsAndRegister(TObject * pointer, const QString & options)
{
    DrawObjects << ADrawObject(pointer, options);
    GraphWindow.RegisterTObject(pointer);
}

void ADrawExplorerWidget::rename(ADrawObject & obj)
{
    bool ok;
    QString text = QInputDialog::getText(&GraphWindow, "Rename item",
                                               "Enter new name:", QLineEdit::Normal,
                                               obj.Name, &ok);
    if (!ok || text.isEmpty()) return;

    obj.Name = text;
    TNamed * tobj = dynamic_cast<TNamed*>(obj.Pointer);
    if (tobj) tobj->SetTitle(text.toLatin1().data());

    GraphWindow.ClearCopyOfDrawObjects();
    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
    updateGui();
}

void ADrawExplorerWidget::toggleEnable(ADrawObject & obj)
{
    obj.bEnabled = !obj.bEnabled;

    GraphWindow.ClearCopyOfDrawObjects();
    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

void ADrawExplorerWidget::remove(int index)
{
    GraphWindow.MakeCopyOfDrawObjects();

    DrawObjects.remove(index); // do not delete - GraphWindow handles garbage collection!

    if (index == 0 && !DrawObjects.isEmpty())
    {
        //remove "same" from the options line for the new leading object
        DrawObjects.first().Options.remove("same", Qt::CaseInsensitive);
    }

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

void ADrawExplorerWidget::setAttributes(int index)
{
    if (index < 0 || index >= DrawObjects.size()) return;

    ADrawObject & obj = DrawObjects[index];
    QString Type = obj.Pointer->ClassName();
    if (Type == "TLegend") return;

    ALineMarkerFillDialog D(obj, (index == 0), this);
    connect(&D, &ALineMarkerFillDialog::requestRedraw, &GraphWindow, &GraphWindowClass::RedrawAll);
    D.exec();

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

void ADrawExplorerWidget::showPanel(ADrawObject &obj)
{
    TH1* h = dynamic_cast<TH1*>(obj.Pointer);
    if (h)
    {
        h->DrawPanel();
        return;
    }
    TGraph* g = dynamic_cast<TGraph*>(obj.Pointer);
    if (g)
    {
        g->DrawPanel();
        return;
    }
}

void ADrawExplorerWidget::fitPanel(ADrawObject &obj)
{
    TH1* h = dynamic_cast<TH1*>(obj.Pointer);
    if (h) h->FitPanel();
    else
    {
        TGraph* g = dynamic_cast<TGraph*>(obj.Pointer);
        if (g) g->FitPanel();
    }
}

double runScaleDialog(QWidget * parent)
{
    QDialog* D = new QDialog(parent);

    QDoubleValidator* vali = new QDoubleValidator(D);
    QVBoxLayout* l = new QVBoxLayout(D);
    QHBoxLayout* l1 = new QHBoxLayout();
    QLabel* lab1 = new QLabel("Multiply by ");
    QLineEdit* leM = new QLineEdit("1.0");
    leM->setValidator(vali);
    l1->addWidget(lab1);
    l1->addWidget(leM);
    QLabel* lab2 = new QLabel(" and divide by ");
    QLineEdit* leD = new QLineEdit("1.0");
    leD->setValidator(vali);
    l1->addWidget(lab2);
    l1->addWidget(leD);
    l->addLayout(l1);
    QPushButton* pb = new QPushButton("Scale");
    QObject::connect(pb, &QPushButton::clicked, D, &QDialog::accept);
    l->addWidget(pb);

    int ret = D->exec();
    double res = 1.0;
    if (ret == QDialog::Accepted)
    {
        double Mult = leM->text().toDouble();
        double Div = leD->text().toDouble();
        if (Div == 0)
            message("Cannot divide by 0!", D);
        else
            res = Mult / Div;
    }

    delete D;
    return res;
}

bool ADrawExplorerWidget::canScale(ADrawObject & obj)
{
    const QString name = obj.Pointer->ClassName();
    QList<QString> impl;
    impl << "TGraph" << "TGraphErrors"  << "TH1I" << "TH1D" << "TH1F" << "TH2I"<< "TH2D"<< "TH2D";
    if (!impl.contains(name))
    {
        message("Not implemented for this object", &GraphWindow);
        return false;
    }
    else return true;
}

void ADrawExplorerWidget::doScale(ADrawObject &obj, double sf)
{
    GraphWindow.MakeCopyOfDrawObjects();
    TObject * tobj = obj.Pointer->Clone();
    GraphWindow.RegisterTObject(tobj);

    const QString name = obj.Pointer->ClassName();
    if (name == "TGraph")
    {
        TGraph* gr = static_cast<TGraph*>(tobj);
        TF2 f("aaa", "[0]*y",0,1);
        f.SetParameter(0, sf);
        gr->Apply(&f);
    }
    else if (name == "TGraphErrors")
    {
        TGraphErrors* gr = static_cast<TGraphErrors*>(tobj);
        TF2 f("aaa", "[0]*y",0,1);
        f.SetParameter(0, sf);
        gr->Apply(&f);
    }
    else if (name.startsWith("TH1"))
    {
        TH1* h = static_cast<TH1*>(tobj);
        //h->Sumw2();
        h->Scale(sf);
        if (obj.Options.isEmpty()) obj.Options = "hist";
    }
    else if (name.startsWith("TH2"))
    {
        TH2* h = static_cast<TH2*>(tobj);
        //h->Sumw2();
        h->Scale(sf);
    }
    obj.Pointer = tobj;

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

void ADrawExplorerWidget::scale(ADrawObject &obj)
{
    if (!canScale(obj)) return;

    double sf = runScaleDialog(&GraphWindow);
    if (sf == 1.0) return;

    doScale(obj, sf);
}

void ADrawExplorerWidget::scaleCDR(ADrawObject &obj)
{
    if (!canScale(obj)) return;

    GraphWindow.TriggerGlobalBusy(true);

    GraphWindow.Extract2DLine();
    if (!GraphWindow.Extraction()) return; //cancel

    double startY = GraphWindow.extracted2DLineYstart();
    double stopY  = GraphWindow.extracted2DLineYstop();

    if (startY == 0)  // can be very small on log canvas
    {
        message("Cannot scale: division by zero", this);
        return;
    }

    double sf = stopY / startY;
    if (sf == 1.0) return;

    doScale(obj, sf);
}

bool getDrawMax(ADrawObject & obj, double & max)
{
    TGraph * g = dynamic_cast<TGraph*>(obj.Pointer);
    if (g)
    {
        max = TMath::MaxElement(g->GetN(), g->GetY());
    }
    else
    {
        TH1 * h = dynamic_cast<TH1*>(obj.Pointer);
        if (h) max = h->GetMaximum();
        else return false;
    }
    return true;
}

void ADrawExplorerWidget::scaleAllSameMax()
{
    const int size = DrawObjects.size();
    if (size == 0) return;

    ADrawObject & mainObj = DrawObjects[0];
    if (!canScale(mainObj)) return;

    double max = 0;
    bool bExtractable = getDrawMax(mainObj, max);
    if (!bExtractable) return;

    for (int i=1; i<size; i++)
    {
        ADrawObject & obj = DrawObjects[i];

        double thisMax = 0;
        getDrawMax(obj, thisMax);
        if (thisMax == 0) continue;

        doScale(obj, max/thisMax);
    }
}

const QPair<double, double> runShiftDialog(QWidget * parent)
{
    QDialog* D = new QDialog(parent);

    QDoubleValidator* vali = new QDoubleValidator(D);
    QVBoxLayout* l = new QVBoxLayout(D);
    QHBoxLayout* l1 = new QHBoxLayout();
      QLabel* lab1 = new QLabel("Multiply by: ");
      QLineEdit* leM = new QLineEdit("1.0");
      leM->setValidator(vali);
      l1->addWidget(lab1);
      l1->addWidget(leM);
      QLabel* lab2 = new QLabel(" Add: ");
      QLineEdit* leA = new QLineEdit("0");
      leA->setValidator(vali);
      l1->addWidget(lab2);
      l1->addWidget(leA);
    l->addLayout(l1);
      QPushButton* pb = new QPushButton("Shift");
      QObject::connect(pb, &QPushButton::clicked, D, &QDialog::accept);
    l->addWidget(pb);

    int ret = D->exec();
    QPair<double, double> res(1.0, 0);
    if (ret == QDialog::Accepted)
      {
        res.first =  leM->text().toDouble();
        res.second = leA->text().toDouble();
      }

    delete D;
    return res;
}

void ADrawExplorerWidget::shift(ADrawObject &obj)
{
    QString name = obj.Pointer->ClassName();
    QList<QString> impl;
    impl << "TGraph" << "TGraphErrors"  << "TH1I" << "TH1D" << "TH1F" << "TProfile";
    if (!impl.contains(name))
    {
        message("Not implemented for this object type", &GraphWindow);
        return;
    }

    const QPair<double, double> val = runShiftDialog(&GraphWindow);
    if (val.first == 1.0 && val.second == 0) return;

    GraphWindow.MakeCopyOfDrawObjects();
    TObject * tobj = obj.Pointer->Clone();
    GraphWindow.RegisterTObject(tobj);

    if (name.startsWith("TGraph"))
    {
        TGraph* g = dynamic_cast<TGraph*>(tobj);
        if (g)
        {
            const int num = g->GetN();
            for (int i=0; i<num; i++)
            {
                double x, y;
                g->GetPoint(i, x, y);
                x = x * val.first + val.second;
                g->SetPoint(i, x, y);
            }
        }
    }
    else
    {
        TH1* h = dynamic_cast<TH1*>(tobj);
        if (h)
        {
            const int nbins = h->GetXaxis()->GetNbins();
            double* new_bins = new double[nbins+1];
            for (int i=0; i <= nbins; i++)
                new_bins[i] = ( h-> GetBinLowEdge(i+1) ) * val.first + val.second;
            h->SetBins(nbins, new_bins);
            delete [] new_bins;
        }
    }
    obj.Pointer = tobj;

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

void ADrawExplorerWidget::drawIntegral(ADrawObject &obj)
{
    TH1* h = dynamic_cast<TH1*>(obj.Pointer);
    if (!h)
    {
        message("Not implemented for this object type", &GraphWindow);
        return;
    }
    int bins = h->GetNbinsX();

    double* edges = new double[bins+1];
    for (int i=0; i<bins+1; i++)
        edges[i] = h->GetBinLowEdge(i+1);

    QString title = "Integral of " + obj.Name;
    TH1D* hi = new TH1D("integral", title.toLocal8Bit().data(), bins, edges);
    delete [] edges;

    QString Xtitle = h->GetXaxis()->GetTitle();
    if (!Xtitle.isEmpty()) hi->GetXaxis()->SetTitle(Xtitle.toLocal8Bit().data());

    double prev = 0;
    for (int i=1; i<bins+1; i++)
      {
        prev += h->GetBinContent(i);
        hi->SetBinContent(i, prev);
      }

    GraphWindow.MakeCopyOfDrawObjects();
    GraphWindow.MakeCopyOfActiveBasketId();
    GraphWindow.ClearBasketActiveId();

    DrawObjects.clear();
    addToDrawObjectsAndRegister(hi, "hist");

    GraphWindow.RedrawAll();
}

void ADrawExplorerWidget::fraction(ADrawObject &obj)
{
    TH1* h = dynamic_cast<TH1*>(obj.Pointer);
    if (!h)
    {
        message("This operation requires TH1 ROOT object", &GraphWindow);
        return;
    }
    TH1* cum = h->GetCumulative(true);

    double integral = h->Integral();

    QDialog D(&GraphWindow);
    D.setWindowTitle("Integral / fraction calculator");

    QVBoxLayout *l = new QVBoxLayout();
        QPushButton * pbD = new QPushButton("Dummy");
    l->addWidget(pbD);
        QHBoxLayout * hh = new QHBoxLayout();
            QLabel * lab = new QLabel("Enter threshold:");
        hh->addWidget(lab);
            QLineEdit* tT = new QLineEdit();
        hh->addWidget(tT);
    l->addLayout(hh);
        QFrame * f = new QFrame();
            f->setFrameShape(QFrame::HLine);
    l->addWidget(f);
        hh = new QHBoxLayout();
            lab = new QLabel("Interpolated sum before:");
        hh->addWidget(lab);
            QLineEdit* tIb = new QLineEdit();
        hh->addWidget(tIb);
            lab = new QLabel("Fraction:");
        hh->addWidget(lab);
            QLineEdit* tIbf = new QLineEdit();
        hh->addWidget(tIbf);
    l->addLayout(hh);
        hh = new QHBoxLayout();
            lab = new QLabel("Interpolated sum after:");
        hh->addWidget(lab);
            QLineEdit* tIa = new QLineEdit();
        hh->addWidget(tIa);
            lab = new QLabel("Fraction:");
        hh->addWidget(lab);
            QLineEdit* tIaf = new QLineEdit();
        hh->addWidget(tIaf);
    l->addLayout(hh);
        hh = new QHBoxLayout();
            lab = new QLabel("Total sum of bins:");
        hh->addWidget(lab);
            QLineEdit* tI = new QLineEdit(QString::number(integral));
            tI->setReadOnly(true);
        hh->addWidget(tI);
    l->addLayout(hh);
        QPushButton * pb = new QPushButton("Close");
    l->addWidget(pb);
    D.setLayout(l);

    pbD->setVisible(false);

    QObject::connect(pb, &QPushButton::clicked, &D, &QDialog::reject);
    QObject::connect(tT, &QLineEdit::editingFinished, [h, cum, integral, tT, tIb, tIbf, tIa, tIaf]()
    {
        bool bOK;
        double val = tT->text().toDouble(&bOK);
        if (bOK)
        {
            int bins = cum->GetNbinsX();
            TAxis * ax = cum->GetXaxis();
            int bin = ax->FindBin(val);

            double result = 0;
            if (bin > bins)
                result = cum->GetBinContent(bins);
            else
            {
                double width = h->GetBinWidth(bin);
                double delta = val - h->GetBinLowEdge(bin);

                double thisBinAdds = h->GetBinContent(bin) * delta / width;

                double prev = (bin == 1 ? 0 : cum->GetBinContent(bin-1));
                result = prev + thisBinAdds;
            }
            tIb->setText(QString::number(result));
            tIbf->setText(QString::number(result/integral));
            if (integral != 0) tIa->setText(QString::number(integral - result));
            else tIa->setText("");
            if (integral != 0) tIaf->setText(QString::number( (integral - result)/integral ));
            else tIa->setText("");
        }
        else
        {
            tIa->clear();
            tIb->clear();
        }
    });

    D.exec();

    delete cum;
}

double GauseWithBase(double * x, double * par)
{
   return par[0] * exp( par[1] * (x[0]+par[2])*(x[0]+par[2]) ) + par[3]*x[0] + par[4];   // [0]*exp([1]*(x+[2])^2) + [3]*x + [4]
}

void ADrawExplorerWidget::fwhm(int index)
{
    ADrawObject & obj = DrawObjects[index];

    TGraph * g = nullptr;
    TH1    * h = nullptr;

    const QString cn = obj.Pointer->ClassName();
    if (cn.startsWith("TGraph"))
    {
        g = dynamic_cast<TGraph*>(obj.Pointer);
        if (!g) return;
    }
    else if (cn.startsWith("TH1") || cn == "TProfile")
    {
        h = dynamic_cast<TH1*>(obj.Pointer);
        if (!h) return;
    }
    else
    {
        message("Can be used only with TGraph and 1D histogram!", &GraphWindow);
        return;
    }

    GraphWindow.TriggerGlobalBusy(true);

    GraphWindow.Extract2DLine();
    if (!GraphWindow.Extraction()) return; //cancel

    double startX = GraphWindow.extracted2DLineXstart();
    double stopX = GraphWindow.extracted2DLineXstop();
    if (startX > stopX) std::swap(startX, stopX);
    //qDebug() << startX << stopX;

    double a = GraphWindow.extracted2DLineA();
    double b = GraphWindow.extracted2DLineB();
    double c = GraphWindow.extracted2DLineC();
    if (fabs(b) < 1.0e-10)
    {
        message("Bad base line, cannot fit", &GraphWindow);
        return;
    }
                                                                   //  S  * exp( -0.5/s2 * (x   -m )^2) +  A *x +  B
    TF1 * f = new TF1("myfunc", GauseWithBase, startX, stopX, 5);  //  [0] * exp(    [1]  * (x + [2])^2) + [3]*x + [4]
    f->SetTitle("Gauss fit");
    GraphWindow.RegisterTObject(f);

    double initMid = startX + 0.5*(stopX - startX);
    //qDebug() << "Initial mid:"<<initMid;
    double initSigma = (stopX - startX)/2.3548; //sigma
    double startPar1 = -0.5 / (initSigma * initSigma );
    //qDebug() << "Initial par1"<<startPar1;
    double midBinNum, valOnMid;
    if (h)
    {
        midBinNum = h->GetXaxis()->FindBin(initMid);
        valOnMid = h->GetBinContent(midBinNum);
    }
    else
    {
        double x = startX;
        double y = 0;
        for (int i=0; i<g->GetN(); i++)
        {
            g->GetPoint(i, x, y);
            if (x >= initMid) break;
        }
        valOnMid = y;
    }
    double baseAtMid = (c - a * initMid) / b;
    double gaussAtMid = valOnMid - baseAtMid;
    //qDebug() << "bin, valMid, baseMid, gaussmid"<<midBinNum<< valOnMid << baseAtMid << gaussAtMid;

    f->SetParameter(0, gaussAtMid);
    f->SetParameter(1, startPar1);
    f->SetParameter(2, -initMid);
    f->FixParameter(3, -a/b);  // fixed!
    f->FixParameter(4, c/b);   // fixed!

    int status = (h ? h->Fit(f, "R0") : g->Fit(f, "R0"));
    if (status != 0)
    {
        message("Fit failed!", &GraphWindow);
        return;
    }

    GraphWindow.MakeCopyOfDrawObjects();
    GraphWindow.MakeCopyOfActiveBasketId();

    double mid = -f->GetParameter(2);
    double sigma = TMath::Sqrt(-0.5/f->GetParameter(1));
    //qDebug() << "sigma:"<<sigma;
    double FWHM = sigma * 2.0*TMath::Sqrt(2.0*TMath::Log(2.0));
    double rel = FWHM/mid;

    //draw fit line
    DrawObjects.insert(index+1, ADrawObject(f, "same"));
    //draw base line
    TF1 *fl = new TF1("line", "pol1", startX, stopX);
    fl->SetTitle("Baseline");
    GraphWindow.RegisterTObject(fl);
    fl->SetLineStyle(2);
    fl->SetParameters(c/b, -a/b);
    DrawObjects.insert(index+2, ADrawObject(fl, "same"));
    //box with results
    QString text = QString("FWHM = %1\nmean = %2\nfwhm/mean = %3").arg(FWHM).arg(mid).arg(rel);
    TPaveText* la = new TPaveText(0.15, 0.75, 0.5, 0.85, "NDC");
    la->SetFillColor(0);
    la->SetBorderSize(1);
    la->SetLineColor(1);
    la->SetTextAlign( (0 + 1) * 10 + 2);
    QStringList sl = text.split("\n");
    for (QString s : sl) la->AddText(s.toLatin1());
    GraphWindow.RegisterTObject(la);
    DrawObjects.insert(index+3, ADrawObject(la, "same"));

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

void ADrawExplorerWidget::linFit(int index)
{
    ADrawObject & obj = DrawObjects[index];

    TGraph * g = nullptr;
    TH1    * h = nullptr;

    const QString cn = obj.Pointer->ClassName();
    if (cn.startsWith("TGraph"))
    {
        g = dynamic_cast<TGraph*>(obj.Pointer);
        if (!g) return;
    }
    else if (cn.startsWith("TH1") || cn == "TProfile")
    {
        h = dynamic_cast<TH1*>(obj.Pointer);
        if (!h) return;
    }
    else
    {
        message("Can be used only with TGraphs and 1D histogram!", &GraphWindow);
        return;
    }

    GraphWindow.TriggerGlobalBusy(true);

    GraphWindow.Extract2DLine();
    if (!GraphWindow.Extraction()) return; //cancel

    double startX = GraphWindow.extracted2DLineXstart();
    double stopX = GraphWindow.extracted2DLineXstop();
    if (startX > stopX) std::swap(startX, stopX);

    double a = GraphWindow.extracted2DLineA();
    double b = GraphWindow.extracted2DLineB();
    double c = GraphWindow.extracted2DLineC();
    if (fabs(b) < 1.0e-10)
    {
        message("Bad line, cannot fit", &GraphWindow);
        return;
    }

    TF1 * f = new TF1("line", "pol1", startX, stopX);
    f->SetTitle("Linear fit");
    GraphWindow.RegisterTObject(f);

    f->SetParameter(0, c/b);
    f->SetParameter(1, -a/b);

    int status = (h ? h->Fit(f, "R0") : g->Fit(f, "R0"));
    if (status != 0)
    {
        message("Fit failed!", &GraphWindow);
        return;
    }

    GraphWindow.MakeCopyOfDrawObjects();
    GraphWindow.MakeCopyOfActiveBasketId();

    double B = f->GetParameter(0);
    double A = f->GetParameter(1);

    DrawObjects.insert(index+1, ADrawObject(f, "same"));

    QString text = QString("y = Ax + B\nA = %1, B = %2\nx range: %3 -> %4").arg(A).arg(B).arg(startX).arg(stopX);
    if (A != 0) text += QString("\ny=0 at %1").arg(-B/A);
    TPaveText* la = new TPaveText(0.15, 0.75, 0.5, 0.85, "NDC");
    la->SetFillColor(0);
    la->SetBorderSize(1);
    la->SetLineColor(1);
    la->SetTextAlign( (0 + 1) * 10 + 2);
    QStringList sl = text.split("\n");
    for (QString s : sl) la->AddText(s.toLatin1());
    GraphWindow.RegisterTObject(la);
    DrawObjects.insert(index+2, ADrawObject(la, "same"));

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

void ADrawExplorerWidget::expFit(int index)
{
    ADrawObject & obj = DrawObjects[index];

    const QString cn = obj.Pointer->ClassName();
    if ( !cn.startsWith("TH1") && cn != "TProfile")
    {
        message("Can be used only with 1D histograms!", &GraphWindow);
        return;
    }

    TH1* h = dynamic_cast<TH1*>(obj.Pointer);
    if (!h) return;

    GraphWindow.TriggerGlobalBusy(true);

    GraphWindow.Extract2DLine();
    if (!GraphWindow.Extraction()) return; //cancel

    double startX = GraphWindow.extracted2DLineXstart();
    double startY = GraphWindow.extracted2DLineYstart();
    double stopX = GraphWindow.extracted2DLineXstop();
    double stopY = GraphWindow.extracted2DLineYstop();
    if (startX > stopX)
    {
        std::swap(startX, stopX);
        std::swap(startY, stopY);
    }

    TF1 * f = new TF1("expDecay", "[0]*exp(-(x-[2])/[1])", startX, stopX);
    f->SetTitle("Exp decay fit");
    GraphWindow.RegisterTObject(f);

    f->SetParameter(0, startY);
    f->SetParameter(1, 1);
    f->SetParameter(2, startX);
    f->SetParLimits(2, startX, startX);

    int status = h->Fit(f, "R0");
    if (status != 0)
    {
        message("Fit failed!", &GraphWindow);
        return;
    }

    GraphWindow.MakeCopyOfDrawObjects();
    GraphWindow.MakeCopyOfActiveBasketId();

    double A = f->GetParameter(0);
    double T = f->GetParameter(1);

    DrawObjects.insert(index+1, ADrawObject(f, "same"));

    QString text = QString("y = A*exp{-(t-t0)/T)}\nT = %1, A = %2, t0 = %3\nx range: %3 -> %4").arg(T).arg(A).arg(startX).arg(stopX);
    TPaveText* la = new TPaveText(0.15, 0.75, 0.5, 0.85, "NDC");
    la->SetFillColor(0);
    la->SetBorderSize(1);
    la->SetLineColor(1);
    la->SetTextAlign( (0 + 1) * 10 + 2);
    QStringList sl = text.split("\n");
    for (QString s : sl) la->AddText(s.toLatin1());
    GraphWindow.RegisterTObject(la);
    DrawObjects.insert(index+2, ADrawObject(la, "same"));

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

void ADrawExplorerWidget::interpolate(ADrawObject &obj)
{
    TH1* hist = dynamic_cast<TH1*>(obj.Pointer);
    if (!hist)
    {
        message("This operation requires TH1 ROOT object", &GraphWindow);
        return;
    }

    QDialog d;
    QVBoxLayout * lMain = new QVBoxLayout(&d);

    QHBoxLayout* l = new QHBoxLayout();
      QVBoxLayout * v = new QVBoxLayout();
          QLabel* lab = new QLabel("From:");
          v->addWidget(lab);
          lab = new QLabel("Step:");
          v->addWidget(lab);
          lab = new QLabel("Steps:");
          v->addWidget(lab);
     l->addLayout(v);
     v = new QVBoxLayout();
          QLineEdit * leFrom = new QLineEdit("0");
          v->addWidget(leFrom);
          QLineEdit * leStep = new QLineEdit("10");
          v->addWidget(leStep);
          QLineEdit* leSteps = new QLineEdit("100");
          v->addWidget(leSteps);
     l->addLayout(v);
    lMain->addLayout(l);

    QPushButton * pb = new QPushButton("Interpolate");
    lMain->addWidget(pb);

    QObject::connect(pb, &QPushButton::clicked,
                     [&d, hist, leFrom, leStep, leSteps, this]()
    {
        int steps = leSteps->text().toDouble();
        int step  = leStep->text().toDouble();
        int from  = leFrom->text().toDouble();

        TH1D* hi = new TH1D("", "", steps, from, from + step*steps);
        for (int i=0; i<steps; i++)
        {
            double x = from + step * i;
            double val = hist->Interpolate(x);
            hi->SetBinContent(i+1, val);
        }
        QString Xtitle = hist->GetXaxis()->GetTitle();
        if (!Xtitle.isEmpty()) hi->GetXaxis()->SetTitle(Xtitle.toLocal8Bit().data());

        GraphWindow.MakeCopyOfDrawObjects();
        GraphWindow.MakeCopyOfActiveBasketId();
        GraphWindow.ClearBasketActiveId();

        DrawObjects.clear();
        addToDrawObjectsAndRegister(hi, "hist");

        GraphWindow.RedrawAll();
        GraphWindow.HighlightUpdateBasketButton(true);

        d.accept();
    }
                     );
    d.exec();
}

void ADrawExplorerWidget::median(ADrawObject &obj)
{
    TH1* hist = dynamic_cast<TH1*>(obj.Pointer);
    if (!hist)
    {
        message("This operation requires TH1 ROOT object", &GraphWindow);
        return;
    }

    QDialog d;
    QVBoxLayout * lMain = new QVBoxLayout(&d);

    QHBoxLayout* l = new QHBoxLayout();
          QLabel* lab = new QLabel("Span in bins:");
          l->addWidget(lab);
          QSpinBox * sbSpan = new QSpinBox();
          sbSpan->setMinimum(2);
          sbSpan->setValue(6);
          l->addWidget(sbSpan);
    lMain->addLayout(l);

    QPushButton * pb = new QPushButton("Apply median filter");
    lMain->addWidget(pb);

    QObject::connect(pb, &QPushButton::clicked,
                     [&d, hist, sbSpan, this]()
    {
        int span = sbSpan->value();

        int deltaLeft  = ( span % 2 == 0 ? span / 2 - 1 : span / 2 );
        int deltaRight = span / 2;

        QVector<double> Filtered;
        int num = hist->GetNbinsX();
        for (int iThisBin = 1; iThisBin <= num; iThisBin++)  // 0-> underflow; num+1 -> overflow
        {
            QVector<double> content;
            for (int i = iThisBin - deltaLeft; i <= iThisBin + deltaRight; i++)
            {
                if (i < 1 || i > num) continue;
                content << hist->GetBinContent(i);
            }

            std::sort(content.begin(), content.end());
            int size = content.size();
            double val;
            if (size == 0) val = 0;
            else val = ( size % 2 == 0 ? (content[size / 2 - 1] + content[size / 2]) / 2 : content[size / 2] );

            Filtered.append(val);
        }

        TH1 * hc = dynamic_cast<TH1*>(hist->Clone());
        if (!hc)
        {
            qWarning() << "Failed to clone the histogram!";
            return;
        }
        for (int iThisBin = 1; iThisBin <= num; iThisBin++)
            hc->SetBinContent(iThisBin, Filtered.at(iThisBin-1));

        GraphWindow.MakeCopyOfDrawObjects();
        GraphWindow.MakeCopyOfActiveBasketId();
        GraphWindow.ClearBasketActiveId();

        DrawObjects.clear();
        addToDrawObjectsAndRegister(hc, "hist");

        GraphWindow.RedrawAll();

        d.accept();
    }
                     );

    d.exec();
}

void ADrawExplorerWidget::projection(ADrawObject &obj, bool bX)
{
    TH2* h = dynamic_cast<TH2*>(obj.Pointer);
    if (!h)
    {
        message("This operation requires TH2 ROOT object", &GraphWindow);
        return;
    }

    TH1D* proj;
    if (bX)
        proj = h->ProjectionX();
    else
        proj = h->ProjectionY();

    if (proj)
    {
        GraphWindow.MakeCopyOfDrawObjects();
        GraphWindow.MakeCopyOfActiveBasketId();
        GraphWindow.ClearBasketActiveId();

        DrawObjects.clear();
        addToDrawObjectsAndRegister(proj, "hist");

        GraphWindow.RedrawAll();
    }
}

void ADrawExplorerWidget::customProjection(ADrawObject & obj)
{
    TH2* hist = dynamic_cast<TH2*>(obj.Pointer);
    if (!hist)
    {
        message("This operation requires TH2 ROOT object selected", &GraphWindow);
        return;
    }

    objForCustomProjection = hist;  // tried safer approach  with clone, but got random error in ROOT on clone attempt
    GraphWindow.ShowProjectionTool();
}

void ADrawExplorerWidget::splineFit(int index)
{
#ifdef USE_EIGEN
    ADrawObject & obj = DrawObjects[index];
    TGraph* g = dynamic_cast<TGraph*>(obj.Pointer);
    if (!g)
    {
        message("Suppoted only for TGraph-based ROOT objects", &GraphWindow);
        return;
    }

    bool ok;
    int numNodes = QInputDialog::getInt(&GraphWindow, "", "Enter number of nodes:", 6, 2, 1000, 1, &ok);
    if (ok)
    {
        int numPoints = g->GetN();
        if (numPoints < numNodes)
        {
            message("Not enough points in the graph for the selected number of nodes", &GraphWindow);
            return;
        }

        QVector<double> x(numPoints), y(numPoints), f(numPoints);
        for (int i=0; i<numPoints; i++)
            g->GetPoint(i, x[i], y[i]);

        CurveFit cf(x.first(), x.last(), numNodes, x, y);

        TGraph* fg = new TGraph();
        fg->SetTitle("SplineFit");
        for (int i=0; i<numPoints; i++)
        {
            const double& xx = x.at(i);
            fg->SetPoint(i, xx, cf.eval(xx));
        }

        GraphWindow.MakeCopyOfDrawObjects();
        GraphWindow.MakeCopyOfActiveBasketId();

        DrawObjects.insert(index+1, ADrawObject(fg, "Csame"));
        GraphWindow.RegisterTObject(fg);

        GraphWindow.RedrawAll();
    }
#else
    message("This option is supported only when ANTS2 is compliled with Eigen library enabled", &GraphWindow);
#endif
}

#include "aaxesdialog.h"
void ADrawExplorerWidget::editAxis(ADrawObject &obj, int axisIndex)
{
    QVector<TAxis*> axes;

    if (dynamic_cast<TGraph*>(obj.Pointer))
    {
        TGraph * g = dynamic_cast<TGraph*>(obj.Pointer);
        axes << g->GetXaxis() << g->GetYaxis() << nullptr;
    }
    else if (dynamic_cast<TH1*>(obj.Pointer))
    {
        TH1* h = dynamic_cast<TH1*>(obj.Pointer);
        axes << h->GetXaxis() << h->GetYaxis() << h->GetZaxis();
    }
    else
    {
        message("Not supported for this object type", &GraphWindow);
        return;
    }

    AAxesDialog D(axes, axisIndex, this);
    connect(&D, &AAxesDialog::requestRedraw, &GraphWindow, &GraphWindowClass::RedrawAll);
    D.exec();

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

const QString ADrawExplorerWidget::generateOptionForSecondaryAxis(int axisIndex, double u1, double u2)
{
    double cx1 = GraphWindow.getCanvasMinX();
    double cx2 = GraphWindow.getCanvasMaxX();
    double cy1 = GraphWindow.getCanvasMinY();
    double cy2 = GraphWindow.getCanvasMaxY();

    double A, B;
    QString axisOpt;

    if (axisIndex == 1)
    {
        //Right axis
        A = (u1 - u2) / (cy1 - cy2);
        B = u1 - A * cy1;
        axisOpt = "+L";
    }
    else
    {
        //Top axis
        A = (u1 - u2) / (cx1 - cx2);
        B = u1 - A * cx1;
        axisOpt = "-L";
    }

    return QString("same;%1;%2;%3").arg(axisIndex == 0 ? "X" : "Y").arg(A).arg(B);
}

void ADrawExplorerWidget::constructIconForObject(QIcon & icon, const ADrawObject & drObj)
{
    const TObject * tObj = drObj.Pointer;
    const QString   Type = tObj->ClassName();
    const QString & Opt  = drObj.Options;

    const TAttLine   * line = nullptr;
    const TAttMarker * mark = nullptr;
    const TAttFill   * fill = nullptr;

    if (Opt.contains("colz", Qt::CaseInsensitive))
    {
        construct2DIcon(icon);
        return;
    }

    if (Type.startsWith("TH2") || Type.startsWith("TF2") || Type.startsWith("TGraph2D"))
    {
        //invent something 3D-ish
        construct2DIcon(icon);
        return;
    }

    if (Type.startsWith("TH") || Type.startsWith("TGraph") || Type.startsWith("TF") || Type.startsWith("TProfile"))
    {
        line = dynamic_cast<const TAttLine*>(tObj);
        mark = dynamic_cast<const TAttMarker*>(tObj);
        fill = dynamic_cast<const TAttFill*>(tObj);
    }

    if ( (Type.startsWith("TH1") || Type.startsWith("TProfile")) && !Opt.contains('P') && !Opt.contains('*')) mark = nullptr;
    if (Type.startsWith("TGraph") && !Opt.contains('P') && !Opt.contains('*')) mark = nullptr;
    if (Type.startsWith("TGraph") && !Opt.contains('C') && !Opt.contains('L')) line = nullptr;

    construct1DIcon(icon, line, mark, fill);
}

void ADrawExplorerWidget::convertRootColoToQtColor(int rootColor, QColor & qtColor)
{
    TColor * tc = gROOT->GetColor(rootColor);

    if (!tc) qtColor = Qt::white;
    else
    {
        int red   = 255 * tc->GetRed();
        int green = 255 * tc->GetGreen();
        int blue  = 255 * tc->GetBlue();

        qtColor = QColor(red, green, blue);
    }
}

void ADrawExplorerWidget::construct1DIcon(QIcon & icon, const TAttLine * line, const TAttMarker * marker, const TAttFill * fill)
{
    QPixmap pm(IconWidth, IconHeight);
    QPainter Painter(&pm);
    Painter.setRenderHint(QPainter::Antialiasing, false);
    Painter.setRenderHint(QPainter::TextAntialiasing, false);
    Painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    Painter.setRenderHint(QPainter::HighQualityAntialiasing, false);
    QColor Color;

    // Background of FillColor
    int RootColor = ( fill ? fill->GetFillColor() : -1 );
    convertRootColoToQtColor(RootColor, Color);
    pm.fill(Color);

    // Line
    if (line)
    {
        RootColor     = line->GetLineColor();
        int LineWidth = 2*line->GetLineWidth();
        if (LineWidth > 10) LineWidth = 10;
        convertRootColoToQtColor(RootColor, Color);
        Painter.setBrush(QBrush(Color));
        //Painter.setPen(QPen(Qt::NoPen));
        Painter.setPen(Color);
        Painter.drawRect( 0, 0.5*IconHeight - ceil(0.5*LineWidth), IconWidth, LineWidth );
    }

    // Marker
    if (marker)
    {
        RootColor = marker->GetMarkerColor();
        convertRootColoToQtColor(RootColor, Color);
        Painter.setBrush(QBrush(Color));
        //Painter.setPen(QPen(Qt::NoPen));
        Painter.setPen(Color);
        int Diameter = 20;
        Painter.drawEllipse( 0.5*IconWidth - 0.5*Diameter, 0.5*IconHeight - 0.5*Diameter, Diameter, Diameter );
    }

    icon = std::move(QIcon(pm));
}

void ADrawExplorerWidget::construct2DIcon(QIcon &icon)
{
    QPixmap pm(IconWidth, IconHeight);
    QPainter Painter(&pm);
    Painter.setRenderHint(QPainter::Antialiasing, false);
    Painter.setRenderHint(QPainter::TextAntialiasing, false);
    Painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    Painter.setRenderHint(QPainter::HighQualityAntialiasing, false);
    QColor Color;

    static const TArrayI & Palette = TColor::GetPalette();
    int Size = Palette.GetSize();
    //qDebug() << "Palette size:" << Size;
    if (Size == 0) return;

    int RootColor = Palette.At(0);
    convertRootColoToQtColor(RootColor, Color);
    Painter.setBrush(QBrush(Color));
    Painter.setPen(QPen(Qt::NoPen));
    Painter.drawRect(0, 0, IconWidth, IconHeight);

    int Divisions = 4;
    int Delta = Size / Divisions;
    for (int i = 1; i <= Divisions; i++)
    {
        int iColor = Delta * i + 1;
        if (iColor >= Size) iColor = Size-1;

        RootColor = Palette.At(iColor);
        convertRootColoToQtColor(RootColor, Color);
        Painter.setBrush(QBrush(Color));
        //Painter.setPen(Color);

        double fraction = (1.0 - 1.0* i / (Divisions+1));
        int w = fraction * IconWidth;
        int h = fraction * IconHeight;
        //qDebug() << "Step:" << i << "iinA/Size"<< iColor << "color:" << RootColor << "w:"<<w << "h:" << h;
        Painter.drawEllipse( 0.5*IconWidth - 0.5*w, 0.5*IconHeight - 0.5*h, w, h );
    }

    icon = std::move(QIcon(pm));
}

void ADrawExplorerWidget::addAxis(int axisIndex)
{
    TGaxis *axis = new TGaxis(0,0,1,1,  0,1, 510, (axisIndex == 1 ? "+L" : "-L") );
    axis->SetLabelFont(42);
    axis->SetTextFont(42);
    axis->SetLabelSize(0.035);
    const QString opt = generateOptionForSecondaryAxis(axisIndex, 0, 1.0);
    DrawObjects << ADrawObject(axis, opt);

    GraphWindow.RedrawAll();
    GraphWindow.HighlightUpdateBasketButton(true);
}

#include <TFile.h>
void ADrawExplorerWidget::saveRoot(ADrawObject &obj)
{
    QString fileName = QFileDialog::getSaveFileName(&GraphWindow, "Save ROOT object", GraphWindow.getLastOpendDir(), "Root files(*.root)");
    if (fileName.isEmpty()) return;
    GraphWindow.getLastOpendDir() = QFileInfo(fileName).absolutePath();
    if (QFileInfo(fileName).suffix().isEmpty()) fileName += ".root";

    TObject * tobj = obj.Pointer;

    //exceptions first
    TF2 * tf2 = dynamic_cast<TF2*>(tobj);
    if (tf2) tobj = tf2->GetHistogram();
    else
    {
        TF1 * tf1 = dynamic_cast<TF1*>(tobj);
        if (tf1) tobj = tf1->GetHistogram();
    }

    tobj->SaveAs(fileName.toLatin1().data());  // sometimes crashes on load!
}

void ADrawExplorerWidget::saveAsTxt(ADrawObject &obj, bool fUseBinCenters)
{
    TObject * tobj = obj.Pointer;
    QString cn = tobj->ClassName();

    if (cn != "TGraph" && cn != "TGraphErrors" && !cn.startsWith("TH1") && !cn.startsWith("TH2") && cn != "TF1" && cn != "TF2")
    {
        message("Not implemented for this object type", &GraphWindow);
        return;
    }

    //exceptions first
    TF2 * tf2 = dynamic_cast<TF2*>(tobj);
    if (tf2) tobj = tf2->GetHistogram();
    else
    {
        TF1 * tf1 = dynamic_cast<TF1*>(tobj);
        if (tf1) tobj = tf1->GetHistogram();
    }

    QString fileName = QFileDialog::getSaveFileName(&GraphWindow, "Save as text", GraphWindow.getLastOpendDir(), "Text files(*.txt);;All files (*.*)");
    if (fileName.isEmpty()) return;
    GraphWindow.getLastOpendDir() = QFileInfo(fileName).absolutePath();
    if (QFileInfo(fileName).suffix().isEmpty()) fileName += ".txt";

    TH2 * h2 = dynamic_cast<TH2*>(tobj);
    if (h2)
    {
        QVector<double> x, y, f;
        for (int iX=1; iX<h2->GetNbinsX()+1; iX++)
            for (int iY=1; iY<h2->GetNbinsX()+1; iY++)
            {
                const double X = (fUseBinCenters ? h2->GetXaxis()->GetBinCenter(iX) : h2->GetXaxis()->GetBinLowEdge(iX));
                const double Y = (fUseBinCenters ? h2->GetYaxis()->GetBinCenter(iY) : h2->GetYaxis()->GetBinLowEdge(iY));
                x.append(X);
                y.append(Y);

                int iBin = h2->GetBin(iX, iY);
                double F = h2->GetBinContent(iBin);
                f.append(F);
            }
        SaveDoubleVectorsToFile(fileName, &x, &y, &f);
        return;
    }

    TH1 * h1 = dynamic_cast<TH1*>(tobj);
    if (h1)
    {
        QVector<double> x,y;
        if (fUseBinCenters)
        {
            for (int i=1; i<h1->GetNbinsX()+1; i++)
            {
                x.append(h1->GetBinCenter(i));
                y.append(h1->GetBinContent(i));
            }
        }
        else
        { //bin starts
            for (int i=1; i<h1->GetNbinsX()+2; i++)  // *** should it be +2?
            {
                x.append(h1->GetBinLowEdge(i));
                y.append(h1->GetBinContent(i));
            }
        }
        SaveDoubleVectorsToFile(fileName, &x, &y);
        return;
    }

    TGraph* g = dynamic_cast<TGraph*>(tobj);
    if (g)
    {
        QVector<double> x,y;
        for (int i = 0; i < g->GetN(); i++)
        {
            double xx, yy;
            int ok = g->GetPoint(i, xx, yy);
            if (ok != -1)
            {
                x.append(xx);
                y.append(yy);
            }
        }
        SaveDoubleVectorsToFile(fileName, &x, &y);
        return;
    }

    qWarning() << "Unsupported type:" << cn;
}

void ADrawExplorerWidget::extract(ADrawObject &obj)
{
    if (!obj.Pointer) return;

    QString cType = obj.Pointer->ClassName();
    if (cType == "TLegend")
    {
        message("Unsupported object type", &GraphWindow);
        return;
    }

    GraphWindow.MakeCopyOfDrawObjects();
    GraphWindow.MakeCopyOfActiveBasketId();
    GraphWindow.ClearBasketActiveId();

    ADrawObject thisObj = obj;

    //update options
    thisObj.Options.remove("same", Qt::CaseInsensitive);
    if (cType.startsWith("TGraph") && !thisObj.Options.contains('a', Qt::CaseInsensitive))
        thisObj.Options += "A";

    thisObj.Pointer = thisObj.Pointer->Clone();
    GraphWindow.RegisterTObject(thisObj.Pointer);

    DrawObjects.clear();
    DrawObjects << thisObj;

    GraphWindow.RedrawAll();
}

#include "TPaveText.h"
#include "atextpavedialog.h"
void ADrawExplorerWidget::editPave(ADrawObject &obj)
{
    TPaveText * Pave = dynamic_cast<TPaveText*>(obj.Pointer);
    if (Pave)
    {
        GraphWindow.MakeCopyOfDrawObjects();

        TPaveText * PaveCopy = new TPaveText(*Pave);
        GraphWindow.RegisterTObject(PaveCopy);
        obj.Pointer = PaveCopy;

        ATextPaveDialog D(*PaveCopy);
        connect(&D, &ATextPaveDialog::requestRedraw, &GraphWindow, &GraphWindowClass::RedrawAll);

        D.exec();
        GraphWindow.RedrawAll();
        GraphWindow.HighlightUpdateBasketButton(true);
    }
}

void ADrawExplorerWidget::copyAxisProperties(TGaxis & grAxis, TAxis & axis)
{
    axis.SetNdivisions(grAxis.GetNdiv());
    axis.SetDrawOption(grAxis.GetDrawOption());
    axis.SetTickLength(grAxis.GetTickSize());

    axis.SetTitle(grAxis.GetTitle());
    axis.SetTitleOffset(grAxis.GetTitleOffset());

    axis.SetTitleFont(grAxis.GetTextFont());
    axis.SetTitleColor(grAxis.GetTextColor());
    axis.SetTitleSize(grAxis.GetTextSize());

    axis.SetLabelColor(grAxis.GetLabelColor());
    axis.SetLabelFont(grAxis.GetLabelFont());
    axis.SetLabelOffset(grAxis.GetLabelOffset());
    axis.SetLabelSize(grAxis.GetLabelSize());
}

void ADrawExplorerWidget::copyAxisProperties(TAxis & axis, TGaxis & grAxis)
{
    grAxis.SetNdivisions(abs(axis.GetNdivisions()));
    grAxis.SetDrawOption(axis.GetDrawOption());
    grAxis.SetTickLength(axis.GetTickLength());

    grAxis.SetTitle(axis.GetTitle());
    grAxis.SetTitleOffset(axis.GetTitleOffset());
    grAxis.SetTitleFont(axis.GetTitleFont());
    grAxis.SetTitleColor(axis.GetTitleColor());
    grAxis.SetTitleSize(axis.GetTitleSize());

    grAxis.SetLabelColor(axis.GetLabelColor());
    grAxis.SetLabelFont(axis.GetLabelFont());
    grAxis.SetLabelOffset(axis.GetLabelOffset());
    grAxis.SetLabelSize(axis.GetLabelSize());
}

void ADrawExplorerWidget::editTGaxis(ADrawObject &obj)
{
    int axisIndex = 1;
    if (obj.Options.contains("X")) axisIndex = 0;

    TGaxis * grAxis = dynamic_cast<TGaxis*>(obj.Pointer);
    if (grAxis)
    {
        TAxis axis;
        copyAxisProperties(*grAxis, axis);

        QVector<TAxis *> vec;
        vec << &axis;
        AAxesDialog D(vec, 0, this);

        QHBoxLayout * lay = new QHBoxLayout();
        lay->addStretch();
            QLabel * lab = new QLabel("Min:");
        lay->addWidget(lab);
            QLineEdit * leMin = new QLineEdit();
            leMin->setText( QString::number(grAxis->GetWmin()) );
        lay->addWidget(leMin);
            lab = new QLabel("Max:");
        lay->addWidget(lab);
            QLineEdit * leMax = new QLineEdit();
            leMax->setText( QString::number(grAxis->GetWmax()) );
        lay->addWidget(leMax);
        lay->addStretch();
        D.addLayout(lay);

        auto UpdateTAxis = [this, leMin, leMax, grAxis, &obj, axisIndex, &axis]()
        {
            double u1 = leMin->text().toDouble();
            double u2 = leMax->text().toDouble();
            grAxis->SetWmin(u1);
            grAxis->SetWmax(u2);
            obj.Options = generateOptionForSecondaryAxis(axisIndex, u1, u2);

            copyAxisProperties(axis, *grAxis);

            GraphWindow.RedrawAll();
        };
        connect(leMin, &QLineEdit::editingFinished, UpdateTAxis);
        connect(leMax, &QLineEdit::editingFinished, UpdateTAxis);

        connect(&D, &AAxesDialog::requestRedraw, UpdateTAxis);
        D.exec();

        UpdateTAxis();

        GraphWindow.HighlightUpdateBasketButton(true);
    }
}

