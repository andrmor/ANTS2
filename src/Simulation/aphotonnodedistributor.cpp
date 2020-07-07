#include "aphotonnodedistributor.h"
#include "aphotonsimsettings.h"
#include "aphoton.h"

#include "TFormula.h"
#include "Spline123/bspline123d.h"

#include <QDebug>

bool APhotonNodeDistributor::init(const APhotonSim_SpatDistSettings & Settings)
{
    ErrorString.clear();

    switch (Settings.Mode)
    {
    case APhotonSim_SpatDistSettings::DirectMode:
      {
        Matrix = Settings.LoadedMatrix;
        break;
      }
    case APhotonSim_SpatDistSettings::FormulaMode:
      {
        QString FormulaText = Settings.Formula;
        FormulaText.replace(QRegExp("\\bx\\b"), "[0]");
        FormulaText.replace(QRegExp("\\by\\b"), "[1]");
        FormulaText.replace(QRegExp("\\bz\\b"), "[2]");
        //qDebug() << "Formula:" << FormulaText;

        TFormula * Formula = new TFormula("", FormulaText.toLocal8Bit().data());
        if (!Formula || !Formula->IsValid())
        {
            ErrorString = "Cannot create TFormula";
            delete Formula;
            return false;
        }

        const double dX = Settings.RangeX / Settings.BinsX;
        const double dY = Settings.RangeY / Settings.BinsY;
        const double dZ = Settings.RangeZ / Settings.BinsZ;

        double R[3];
        Matrix.reserve(Settings.BinsX * Settings.BinsY * Settings.BinsZ);

        for (int ix = 0; ix < Settings.BinsX; ix++)
            for (int iy = 0; iy < Settings.BinsY; iy++)
                for (int iz = 0; iz < Settings.BinsZ; iz++)
                {
                    R[0] = -0.5*Settings.RangeX + (0.5 + ix)*dX;
                    R[1] = -0.5*Settings.RangeY + (0.5 + iy)*dY;
                    R[2] = -0.5*Settings.RangeZ + (0.5 + iz)*dZ;

                    double prob = Formula->EvalPar(nullptr, R);
                    //qDebug() << R[0]  << R[1] << R[2] << prob;

                    Matrix << A3DPosProb(R[0], R[1], R[2], prob);
                }
        delete Formula;

        break;
      }
    case APhotonSim_SpatDistSettings::SplineMode:
      {
        std::string stdSplineStr(Settings.Spline.toLatin1().data());

        Bspline3d sp(stdSplineStr);
        //qDebug() << sp.isInvalid() << sp.IsReady();

        const double dX = Settings.RangeX / Settings.BinsX;
        const double dY = Settings.RangeY / Settings.BinsY;
        const double dZ = Settings.RangeZ / Settings.BinsZ;

        Matrix.reserve(Settings.BinsX * Settings.BinsY * Settings.BinsZ);

        for (int ix = 0; ix < Settings.BinsX; ix++)
            for (int iy = 0; iy < Settings.BinsY; iy++)
                for (int iz = 0; iz < Settings.BinsZ; iz++)
                {
                    double x = -0.5*Settings.RangeX + (0.5 + ix)*dX;
                    double y = -0.5*Settings.RangeY + (0.5 + iy)*dY;
                    double z = -0.5*Settings.RangeZ + (0.5 + iz)*dZ;

                    double prob = sp.Eval(x, y, z);
                    //qDebug() << x << y << z << prob;

                    Matrix << A3DPosProb(x, y, z, prob);
                }

        break;
      }
    default:
        ErrorString = "Unknown distributed node mode";
        return false;
    }

    if (Matrix.isEmpty())
    {
        ErrorString = "Matrix for distributed node photon generation is empty!";
        return false;
    }

    bool ok = calculateCumulativeProbabilities();
    return ok;
}

void APhotonNodeDistributor::releaseResources()
{
    Matrix.clear();
    CumulativeProb.clear();
}

void APhotonNodeDistributor::apply(APhoton & photon, double *center, double rnd) const
{
    auto it = std::upper_bound(CumulativeProb.begin(), CumulativeProb.end(), rnd * SumProb);
    int iSelectedCell = it - CumulativeProb.begin();

    if (iSelectedCell == Matrix.size()) iSelectedCell = Matrix.size() - 1;

    for (int i = 0; i < 3; i++)
        photon.r[i] = center[i] + Matrix.at(iSelectedCell).R[i];
}

bool APhotonNodeDistributor::calculateCumulativeProbabilities()
{
    const int size = Matrix.size();

    SumProb = 0;
    for (int i = 0; i < size; i++) SumProb += Matrix.at(i).Probability;

    if (SumProb == 0)
    {
        ErrorString = "Sum of probabilities iz zero!";
        return false;
    }

    CumulativeProb.resize(size);
    double acc = 0;
    for (int i = 0; i < size; i++)
    {
        acc += Matrix[i].Probability;
        CumulativeProb[i] = acc;
    }

    return true;
}
