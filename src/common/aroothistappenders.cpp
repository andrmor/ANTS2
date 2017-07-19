#include "aroothistappenders.h"

#include "TH1D.h"
#include "TH2D.h"

#include <QDebug>

void appendTH1D(TH1D *toHist, TH1D *fromHist)
{
    if (!toHist || !fromHist) return;

    qDebug() << toHist->GetEntries() << fromHist->GetEntries();

    double numEntries = toHist->GetEntries();
    if (numEntries < 1)
    {
        *toHist = *fromHist;
    }
    else
    {
        int bins = fromHist->GetNbinsX();
        qDebug() << "biiiins"<<bins+2;

        for (int i = 0; i < bins+2; i++)
            toHist->Fill(fromHist->GetBinCenter(i), fromHist->GetBinContent(i));

        qDebug() << "waaait"<<toHist->GetEntries();
        toHist->TH1::SetEntries(numEntries + fromHist->GetEntries());
    }
     qDebug() << toHist->GetEntries();
}

void appendTH2D(TH2D *toHist, TH2D *fromHist)
{
    if (!toHist || !fromHist) return;

    //qDebug() << toHist->GetEntries() << fromHist->GetEntries();

    double numEntries = toHist->GetEntries();
    if (numEntries < 1)
    {
        *toHist = *fromHist;
    }
    else
    {
        int binsX = fromHist->GetNbinsX();
        int binsY = fromHist->GetNbinsY();

        for (int ix = 0; ix < binsX+2; ix++)
            for (int iy = 0; iy < binsY+1; iy++)
            {
                double x = fromHist->GetXaxis()->GetBinCenter( ix );
                double y = fromHist->GetYaxis()->GetBinCenter( iy );
                toHist->Fill(x, y, fromHist->GetBinContent(ix, iy));
            }

        toHist->SetEntries(numEntries + fromHist->GetEntries());
    }
    //qDebug() << toHist->GetEntries();
}
