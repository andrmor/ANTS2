#ifndef AROOTHISTAPPENDERS_H
#define AROOTHISTAPPENDERS_H

class TH1D;
class TH2D;

void appendTH1D(TH1D *toHist, TH1D *fromHist);
void appendTH2D(TH2D *toHist, TH2D *fromHist);

#endif // AROOTHISTAPPENDERS_H
