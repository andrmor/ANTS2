#include "adrawexplorerwidget.h"
#include "arootmarkerconfigurator.h"
#include "arootlineconfigurator.h"
#include "amessage.h" //need?

#include <QTreeWidgetItem>
#include <QMenu>
#include <QInputDialog>
#include <QDebug>

#include "TObject.h"
#include "TNamed.h"
#include "TAttMarker.h"
#include "TAttLine.h"
#include "TH1.h"
#include "TH2.h"
#include "TF2.h"
#include "TGraph.h"
#include "TGraphErrors.h"

ADrawExplorerWidget::ADrawExplorerWidget(QVector<ADrawObject> & DrawObjects) :
    DrawObjects(DrawObjects)
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

        addTopLevelItem(item);
    }
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

    QMenu Menu;

    QAction * renameA =     Menu.addAction("Rename");    
    Menu.addSeparator();
    QAction * setMarkerA =  Menu.addAction("Set marker attributes");
    QAction * setLineA =    Menu.addAction("Set line attributes");
    QAction * panelA   =    Menu.addAction("Root hist/graph panel");
    Menu.addSeparator();
    QAction * scaleA   =    Menu.addAction("Scale");
    Menu.addSeparator();
    QAction* integralA =    Menu.addAction("Draw integral");
    QAction* fractionA =    Menu.addAction("Calculate fraction before/after");
    QAction* interpolateA = Menu.addAction("Interpolate");
    Menu.addSeparator();
    QAction* titleX =       Menu.addAction("Edit X title");
    QAction* titleY =       Menu.addAction("Edit Y title");
    Menu.addSeparator();
    QAction * delA =        Menu.addAction("Delete");

    QAction* si = Menu.exec(mapToGlobal(pos));
    if (!si) return; //nothing was selected

   if      (si == renameA)      rename(obj);
   else if (si == delA)         remove(index);
   else if (si == setLineA)     setLine(obj);
   else if (si == setMarkerA)   setMarker(obj);
   else if (si == panelA)       showPanel(obj);
   else if (si == scaleA)       scale(obj);
   else if (si == integralA)    drawIntegral(obj);
   else if (si == fractionA)    fraction(obj);
   else if (si == interpolateA) interpolate(obj);
   else if (si == titleX)       editTitle(obj, 0);
   else if (si == titleY)       editTitle(obj, 1);

    /*
    QAction* scale = 0;
    QAction* shift = 0;
    QAction* uniMap = 0;
    QAction* gaussFit = 0;

    QAction* median = 0;

    QAction* splineFit = 0;
    QAction* projX = 0;
    QAction* projY = 0;

    if (temp)
      {
        //menu triggered at a valid item
        index = ui->lwBasket->row(temp);
        const QString Type = Basket->getType(index);
        shift = BasketMenu.addAction("Shift X scale");
        if (!MasterDrawObjects.isEmpty())
            if ( QString(MasterDrawObjects.first().Pointer->ClassName()) == "TH2D")
                if (Type == "TH2D")
                   uniMap = BasketMenu.addAction("Use as unif. correction map");

        if (Type.startsWith("TH1"))
        {
               gaussFit = BasketMenu.addAction("Fit with Gauss");
               median = BasketMenu.addAction("Apply median filter");
               drawIntegral = BasketMenu.addAction("Draw integral");
               interpolate = BasketMenu.addAction("Interpolate");
               fraction = BasketMenu.addAction("Calculate fraction before/after");
        }
        else if (Type.startsWith("TH2"))
        {
               projX = BasketMenu.addAction("X projection");
               projY = BasketMenu.addAction("Y projection");
        }
        else if (Type == "TGraph" || Type == "TProfile")
        {
               splineFit = BasketMenu.addAction("Fit with B-spline");
        }
        BasketMenu.addSeparator();

      }
    else if (selectedItem == shift)
    {
        if (index == -1) return; //protection
        if (DrawObjects.isEmpty()) return; //protection
        //TObject * obj = Basket->getDrawObjects(row)->first().Pointer;
        if (obj)
        {
            QString name = obj->ClassName();
            QList<QString> impl;
            impl << "TGraph" << "TGraphErrors"  << "TH1I" << "TH1D" << "TH1F" << "TProfile";
            if (!impl.contains(name))
             {
               message("Not implemented for this object type", this);
               return;
             }

            const QPair<double, double> val = runShiftDialog();
            if (val.first == 1.0 && val.second == 0) return;

            if (name.startsWith("TGraph"))
            {
                TGraph* g = dynamic_cast<TGraph*>(obj);
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
                TH1* h = dynamic_cast<TH1*>(obj);
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

            RedrawAll();
        }
    }
    else if (selectedItem == projX || selectedItem == projY)
    {
        TH2* h = static_cast<TH2*>(obj);
        TH1D* proj;
        if (selectedItem == projX)
            proj = h->ProjectionX();
        else
            proj = h->ProjectionY();
        if (proj) Draw(proj, "hist");
    }
    else if (selectedItem == gaussFit)
    {
        TH1* h =   static_cast<TH1*>(obj);
        TF1* f1 = new TF1("f1", "gaus");
        int status = h->Fit(f1, "+");
        if (status == 0)
        {
            ui->cbShowFitParameters->setChecked(true);
            ui->cbShowLegend->setChecked(true);
            RedrawAll();
        }
    }
    else if (selectedItem == splineFit)
    {
  #ifdef USE_EIGEN
        TGraph* g =   static_cast<TGraph*>(obj);
        if (!g)
        {
            message("Suppoted only for TGraph-based ROOT objects", this);
            return;
        }

        bool ok;
        int numNodes = QInputDialog::getInt(this, "", "Enter number of nodes:", 6, 2, 1000, 1, &ok);
        if (ok)
        {
            int numPoints = g->GetN();
            if (numPoints < numNodes)
            {
                message("Not enough points in the graph for the selected number of nodes", this);
                return;
            }

            QVector<double> x(numPoints), y(numPoints), f(numPoints);
            for (int i=0; i<numPoints; i++)
                g->GetPoint(i, x[i], y[i]);

            CurveFit cf(x.first(), x.last(), numNodes, x, y);

            TGraph* fg = new TGraph();
            for (int i=0; i<numPoints; i++)
            {
                const double& xx = x.at(i);
                fg->SetPoint(i, xx, cf.eval(xx));
            }

            //Basket[row].DrawObjects.append(ADrawObject(fg, "Csame"));
            RedrawAll();
        }
  #else
      message("CurveFitter is supported only if ANTS2 is compliled with Eigen library enabled", this);
      return;
  #endif
    }
    else if (selectedItem == median)
    {
        TH1* hist = dynamic_cast<TH1*>(obj);
        if (!hist)
        {
            message("This operation requires TH1 ROOT object", this);
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

            this->Draw(hc, "hist");
            d.accept();
        }
                         );

        d.exec();
    }

    */
}

void ADrawExplorerWidget::rename(ADrawObject & obj)
{
    bool ok;
    QString text = QInputDialog::getText(this, "Rename item",
                                               "Enter new name:", QLineEdit::Normal,
                                               obj.Name, &ok);
    if (!ok || text.isEmpty()) return;

    obj.Name = text;
    TNamed * tobj = dynamic_cast<TNamed*>(obj.Pointer);
    if (tobj) tobj->SetTitle(text.toLatin1().data());

    updateGui();
}

void ADrawExplorerWidget::remove(int index)
{
    DrawObjects.remove(index); // do not delete - GraphWindow handles garbage collection!

    if (index == 0 && !DrawObjects.isEmpty())
    {
        //remove "same" from the options line for the new leading object
        DrawObjects[0].Options.remove("same", Qt::CaseInsensitive);
    }

    updateGui();
    emit requestRedraw();
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
           emit requestRedraw();
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
           emit requestRedraw();
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

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleValidator>
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
        message("Not implemented for this object", this);
        return;
    }

    double sf = runScaleDialog(this);
    if (sf == 1.0) return;

    TObject * tobj = obj.Pointer;
    if (name == "TGraph")
    {
        TGraph* gr = dynamic_cast<TGraph*>(tobj);
        TF2 f("aaa", "[0]*y",0,1);
        f.SetParameter(0, sf);
        gr->Apply(&f);
    }
    if (name == "TGraphErrors")
    {
        TGraphErrors* gr = dynamic_cast<TGraphErrors*>(tobj);
        TF2 f("aaa", "[0]*y",0,1);
        f.SetParameter(0, sf);
        gr->Apply(&f);
    }
    if (name.startsWith("TH1"))
    {
        TH1* h = dynamic_cast<TH1*>(tobj);
        //h->Sumw2();
        h->Scale(sf);
    }
    if (name.startsWith("TH2"))
    {
        TH2* h = dynamic_cast<TH2*>(tobj);
        //h->Sumw2();
        h->Scale(sf);
    }
    emit requestRedraw();
}

void ADrawExplorerWidget::drawIntegral(ADrawObject &obj)
{
    TH1* h = dynamic_cast<TH1*>(obj.Pointer);
    if (!h)
    {
        message("Not implemented for this object type", this);
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

    emit requestMakeCopy();
    emit requestRegister(hi);

    DrawObjects.clear();
    DrawObjects << ADrawObject(hi, "hist");

    emit requestRedraw();
}

void ADrawExplorerWidget::fraction(ADrawObject &obj)
{
    TH1* h = dynamic_cast<TH1*>(obj.Pointer);
    if (!h)
    {
        message("This operation requires TH1 ROOT object", this);
        return;
    }
    TH1* cum = h->GetCumulative(true);

    double integral = h->Integral();

    QDialog D(this);
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

void ADrawExplorerWidget::interpolate(ADrawObject &obj)
{
    TH1* hist = dynamic_cast<TH1*>(obj.Pointer);
    if (!hist)
    {
        message("This operation requires TH1 ROOT object", this);
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

        emit requestMakeCopy();
        emit requestRegister(hi);

        DrawObjects.clear();
        DrawObjects << ADrawObject(hi, "hist");

        emit requestRedraw();

        d.accept();
    }
                     );
    d.exec();
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
            message("Not supported for this object type", this);
            return;
        }
    }

    QString oldTitle;
    oldTitle = axis->GetTitle();
    bool ok;
    QString newTitle = QInputDialog::getText(this, "Change axis title", QString("New %1 axis title:").arg(X0Y1 == 0 ? "X" : "Y"), QLineEdit::Normal, oldTitle, &ok);
    if (ok) axis->SetTitle(newTitle.toLatin1().data());

    emit requestRedraw();
}

