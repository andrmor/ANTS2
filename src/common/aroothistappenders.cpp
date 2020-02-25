#include "aroothistappenders.h"

#include "TH1D.h"
#include "TH2D.h"

#include <QDebug>

void appendTH1D(TH1D *toHist, TH1D *fromHist)
{
    if (!toHist || !fromHist) return;

    double numEntries = toHist->GetEntries();
    if (numEntries < 1)
    {
        *toHist = *fromHist;
    }
    else
    {
        int bins = fromHist->GetNbinsX();

        for (int i = 0; i < bins+2; i++)
            toHist->Fill(fromHist->GetBinCenter(i), fromHist->GetBinContent(i));

        toHist->BufferEmpty(1); //otherwise set entries will not have effect for histograms with small number of entries (i.e. when buffer is not full)
        toHist->TH1::SetEntries(numEntries + fromHist->GetEntries());
    }
}

#include "ahistogram.h"
void appendTH1DwithStat(TH1D * & toHist, TH1D *fromHist)
{
    if (!toHist || !fromHist) return;

    double numEntries = toHist->GetEntries();
    if (numEntries < 1)
    {
        *toHist = *fromHist;
    }
    else
    {
        double statsFrom[4];
        fromHist->GetStats(statsFrom);
        double statsTo[4];
        toHist->GetStats(statsTo);
        for (int i=0; i<4; i++)
            statsTo[i] += statsFrom[i];

        int bins = fromHist->GetNbinsX();

        for (int i = 0; i < bins+2; i++)
            toHist->Fill(fromHist->GetBinCenter(i), fromHist->GetBinContent(i));

        toHist->BufferEmpty(1); //otherwise set entries will not have effect for histograms with small number of entries (i.e. when buffer is not full)
        //toHist->TH1::SetEntries(numEntries + fromHist->GetEntries());

        ATH1D * toHistMy = new ATH1D(*toHist);
        toHistMy->setStats(statsTo);

        delete toHist; toHist = toHistMy;
    }
}

void appendTH2D(TH2D *toHist, TH2D *fromHist)
{
    if (!toHist || !fromHist) return;

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

        toHist->BufferEmpty(1);
        toHist->SetEntries(numEntries + fromHist->GetEntries());
    }
}
