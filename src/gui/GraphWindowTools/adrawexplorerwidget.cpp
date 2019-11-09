#include "adrawexplorerwidget.h"
#include "arootmarkerconfigurator.h"
#include "arootlineconfigurator.h"
#include "amessage.h"
#include "graphwindowclass.h"
#include "afiletools.h"

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

#include "TObject.h"
#include "TNamed.h"
#include "TAttMarker.h"
#include "TAttLine.h"
#include "TH1.h"
#include "TH2.h"
#include "TF2.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TPaveText.h"

ADrawExplorerWidget::ADrawExplorerWidget(GraphWindowClass & GraphWindow, QVector<ADrawObject> & DrawObjects) :
    GraphWindow(GraphWindow), DrawObjects(DrawObjects)
{
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ADrawExplorerWidget::customContextMenuRequested, this, &ADrawExplorerWidget::onContextMenuRequested);
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
        if (name.isEmpty()) name = "--";

        QString nameShort = name;
        if (nameShort.size() > 15)
        {
            nameShort.truncate(15);
            nameShort += "..";
        }

        QString className = tObj->ClassName();
        QString opt = drObj.Options;

        item->setText(0, QString("%1 %2").arg(nameShort).arg(className));
        item->setToolTip(0, QString("Name: %1\nClassName: %2\nDraw options: %3").arg(name).arg(className).arg(opt));
        item->setText(1, QString::number(i));

        if (!drObj.bEnabled) item->setForeground(0, QBrush(Qt::red));

        addTopLevelItem(item);
    }

    objForCustomProjection = nullptr;
}

void ADrawExplorerWidget::onContextMenuRequested(const QPoint &pos)
{
    QTreeWidgetItem * item = currentItem();
    if (!item) return;

    int index = item->text(1).toInt();
    //qDebug() << "index selected: " << index;
    if (index < 0 || index >= DrawObjects.size())
    {
        qWarning() << "Corrupted model: invalid index extracted";
        return;
    }
    ADrawObject & obj = DrawObjects[index];
    const QString Type = obj.Pointer->ClassName();

    QMenu Menu;

    QAction * renameA =     Menu.addAction("Rename");    

    Menu.addSeparator();

    QAction * enableA =     Menu.addAction("Toggle enabled/disabled"); enableA->setChecked(obj.bEnabled); enableA->setEnabled(index != 0);

    Menu.addSeparator();

    QAction * delA =        Menu.addAction("Delete"); delA->setEnabled(index != 0);

    Menu.addSeparator();

    QAction * setLineA =    Menu.addAction("Set line attributes");
    QAction * setMarkerA =  Menu.addAction("Set marker attributes");
    QAction * panelA   =    Menu.addAction("Root line/marker panel");

    Menu.addSeparator();

    QMenu * manipMenu =     Menu.addMenu("Manipulate histogram"); manipMenu->setEnabled(Type.startsWith("TH1"));
        QAction * scaleA =      manipMenu->addAction("Scale");
        QAction * shiftA =      manipMenu->addAction("Shift X scale");
        manipMenu->addSeparator();
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
        QAction* linFitA    =   fitMenu->addAction("Linear (use click-drag)"); linFitA->setEnabled(Type.startsWith("TH1") || Type == "TProfile");
        QAction* fwhmA      =   fitMenu->addAction("Gauss (use click-frag)"); fwhmA->setEnabled(Type.startsWith("TH1") || Type == "TProfile");
        QAction* splineFitA =   fitMenu->addAction("B-spline"); splineFitA->setEnabled(Type == "TGraph" || Type == "TGraphErrors");   //*** implement for TH1 too!
        fitMenu->addSeparator();
        QAction* showFitPanel = fitMenu->addAction("Show fit panel");

    Menu.addSeparator();

    QAction* titleX =       Menu.addAction("Edit X title");
    QAction* titleY =       Menu.addAction("Edit Y title");

    Menu.addSeparator();

    QMenu * saveMenu =      Menu.addMenu("Save");
        QAction* saveRootA =    saveMenu->addAction("Save ROOT object");
        saveMenu->addSeparator();
        QAction* saveTxtA =     saveMenu->addAction("Save as text");
        QAction* saveEdgeA =    saveMenu->addAction("Save hist as text using bin edges");

    // ------ exec ------

    QAction* si = Menu.exec(mapToGlobal(pos));
    if (!si) return; //nothing was selected

   if      (si == renameA)      rename(obj);
   else if (si == enableA)      toggleEnable(obj);
   else if (si == delA)         remove(index);
   else if (si == setLineA)     setLine(obj);
   else if (si == setMarkerA)   setMarker(obj);
   else if (si == panelA)       showPanel(obj);
   else if (si == scaleA)       scale(obj);
   else if (si == integralA)    drawIntegral(obj);
   else if (si == fractionA)    fraction(obj);
   else if (si == linFitA)      linFit(index);
   else if (si == fwhmA)        fwhm(index);
   else if (si == interpolateA) interpolate(obj);
   else if (si == titleX)       editTitle(obj, 0);
   else if (si == titleY)       editTitle(obj, 1);
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
    GraphWindow.UpdateBasketGUI();
    updateGui();
}

void ADrawExplorerWidget::toggleEnable(ADrawObject & obj)
{
    obj.bEnabled = !obj.bEnabled;

    GraphWindow.ClearCopyOfDrawObjects();
    GraphWindow.RedrawAll();
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
}

void ADrawExplorerWidget::setMarker(ADrawObject & obj)
{
    TAttMarker* la = dynamic_cast<TAttMarker*>(obj.Pointer);
    if (la)
    {
       int color = la->GetMarkerColor();
       int siz = la->GetMarkerSize();
       int style = la->GetMarkerStyle();
       ARootMarkerConfigurator* rlc = new ARootMarkerConfigurator(&color, &siz, &style);
       int res = rlc->exec();
       if (res != 0)
       {
           la->SetMarkerColor(color);
           la->SetMarkerSize(siz);
           la->SetMarkerStyle(style);
           la->Modify();
           GraphWindow.ClearCopyOfDrawObjects();
           GraphWindow.RedrawAll();
       }
    }
}

void ADrawExplorerWidget::setLine(ADrawObject & obj)
{
    TAttLine* la = dynamic_cast<TAttLine*>(obj.Pointer);
    if (la)
    {
       int color = la->GetLineColor();
       int wid = la->GetLineWidth();
       int style = la->GetLineStyle();
       ARootLineConfigurator* rlc = new ARootLineConfigurator(&color, &wid, &style);
       int res = rlc->exec();
       if (res != 0)
       {
           la->SetLineColor(color);
           la->SetLineWidth(wid);
           la->SetLineStyle(style);
           la->Modify();
           GraphWindow.ClearCopyOfDrawObjects();
           GraphWindow.RedrawAll();
       }
    }
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

void ADrawExplorerWidget::scale(ADrawObject &obj)
{
    QString name = obj.Pointer->ClassName();
    QList<QString> impl;
    impl << "TGraph" << "TGraphErrors"  << "TH1I" << "TH1D" << "TH1F" << "TH2I"<< "TH2D"<< "TH2D";
    if (!impl.contains(name))
    {
        message("Not implemented for this object", &GraphWindow);
        return;
    }

    double sf = runScaleDialog(&GraphWindow);
    if (sf == 1.0) return;

    //qDebug() << "----On start, this:"<< GraphWindow.DrawObjects.first().Pointer;
    GraphWindow.MakeCopyOfDrawObjects();
    //qDebug() << "----Copy:"<< GraphWindow.PreviousDrawObjects.first().Pointer;
    TObject * tobj = obj.Pointer->Clone();
    //qDebug() << "+++++++clone:" << tobj;
    GraphWindow.RegisterTObject(tobj);

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

    //qDebug() << "This:"<< GraphWindow.DrawObjects.first().Pointer;
    //qDebug() << "Copy:"<< GraphWindow.PreviousDrawObjects.first().Pointer;

    GraphWindow.RedrawAll();
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

    const QString cn = obj.Pointer->ClassName();
    if ( !cn.startsWith("TH1") && cn!="TProfile")
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
    double midBinNum = h->GetXaxis()->FindBin(initMid);
    double valOnMid = h->GetBinContent(midBinNum);
    double baseAtMid = (c - a * initMid) / b;
    double gaussAtMid = valOnMid - baseAtMid;
    //qDebug() << "bin, valMid, baseMid, gaussmid"<<midBinNum<< valOnMid << baseAtMid << gaussAtMid;

    f->SetParameter(0, gaussAtMid);
    f->SetParameter(1, startPar1);
    f->SetParameter(2, -initMid);
    f->FixParameter(3, -a/b);  // fixed!
    f->FixParameter(4, c/b);   // fixed!

    int status = h->Fit(f, "R0");
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
}

void ADrawExplorerWidget::linFit(int index)
{
    ADrawObject & obj = DrawObjects[index];

    const QString cn = obj.Pointer->ClassName();
    if ( !cn.startsWith("TH1") && cn!="TProfile")
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

    int status = h->Fit(f, "R0");
    if (status != 0)
    {
        message("Fit failed!", &GraphWindow);
        return;
    }

    GraphWindow.MakeCopyOfDrawObjects();
    GraphWindow.MakeCopyOfActiveBasketId();

    double A = f->GetParameter(0);
    double B = f->GetParameter(1);

    DrawObjects.insert(index+1, ADrawObject(f, "same"));

    QString text = QString("y = Ax+B\nA = %1, B = %2\nx range: %3 -> %4").arg(A).arg(B).arg(startX).arg(stopX);
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

void ADrawExplorerWidget::editTitle(ADrawObject &obj, int X0Y1)
{
    TAxis * axis = nullptr;

    TGraph * g = dynamic_cast<TGraph*>(obj.Pointer);
    if (g)
        axis = ( X0Y1 == 0 ? g->GetXaxis() : g->GetYaxis() );
    else
    {
        TH1* h = dynamic_cast<TH1*>(obj.Pointer);
        if (h)
            axis = ( X0Y1 == 0 ? h->GetXaxis() : h->GetYaxis() );
        else
        {
            message("Not supported for this object type", &GraphWindow);
            return;
        }
    }

    QString oldTitle;
    oldTitle = axis->GetTitle();
    bool ok;
    QString newTitle = QInputDialog::getText(&GraphWindow, "Change axis title", QString("New %1 axis title:").arg(X0Y1 == 0 ? "X" : "Y"), QLineEdit::Normal, oldTitle, &ok);
    if (ok) axis->SetTitle(newTitle.toLatin1().data());

    GraphWindow.RedrawAll();
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

