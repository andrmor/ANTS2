#ifndef APOSITIONENERGYRECORDS
#define APOSITIONENERGYRECORDS

#include <QDataStream>
#include <QDebug>

struct APositionEnergyRecord
{
    double r[3];
    double energy;
    double time    = 0;

    APositionEnergyRecord & operator=(const APositionEnergyRecord & other)
    {
        if (this != &other)
        {
            for (int i=0; i<3; i++)
                r[i] = other.r[i];

            energy   = other.energy;
            time     = other.time;
        }
        return *this;
    }

    friend bool operator< (const APositionEnergyRecord & lhs, const APositionEnergyRecord & rhs)
    {
        return (lhs.energy < rhs.energy);
    }

    bool isCloser(double length2, const APositionEnergyRecord & other) const
    {
        double d2 = 0;
        for (int i=0; i<3; i++)
        {
            const double delta = r[i] - other.r[i];
            d2 += delta * delta;
        }
        return d2 < length2;
    }

    void MergeWith(const double * R, double E, double Time)
    {
        if (E > 0)
        {
            const double factor = 1.0 / (energy + E);

            for (int i=0; i<3; i++)
                r[i] = (r[i]*energy + R[i]*E) * factor;
            time     = (time*energy + Time*E) * factor;

            energy   += E;
        }
    }

    void MergeWith(const APositionEnergyRecord & other)
    {
        if (other.energy > 0)
        {
            const double factor = 1.0 / (energy + other.energy);

            for (int i=0; i<3; i++)
                r[i] = (r[i]*energy + other.r[i]*other.energy) * factor;
            time     = (time*energy + other.time*other.energy) * factor;

            energy   += other.energy;
        }
    }
};

class APositionEnergyBuffer
{
 private:
    int numRecords = 1;

    APositionEnergyBuffer(const APositionEnergyBuffer &) { } // not copiable!
    APositionEnergyBuffer &operator=(const APositionEnergyBuffer &) { return *this; } // not copiable!

 public:
    APositionEnergyRecord * rec; //pointer to array of position energy records

    APositionEnergyBuffer() : rec(new APositionEnergyRecord[1]) {}   //by default 1 point!
    ~APositionEnergyBuffer(){ delete [] rec; }

    //public functions
    inline APositionEnergyRecord & operator[](int i) const { return rec[i]; } //get record number i
    inline const APositionEnergyRecord & at(int i) const { return rec[i]; }

    inline int size() const {return numRecords;}          //get number of points
    inline void Reinitialize(int size)              //resize (data is undefined!)
    {
         delete [] rec;
         rec = new APositionEnergyRecord[size];
         numRecords = size;
    }

    void AddPoint(double X, double Y, double Z, double Energy, double time) //add new point to the record
    {
        APositionEnergyRecord * recOld = rec;

        rec = new APositionEnergyRecord[numRecords + 1];
        for (int i = 0; i < numRecords; i++)
        {
            for (int j=0; j<3; j++)
                rec[i].r[j] = recOld[i].r[j];

            rec[i].energy = recOld[i].energy;
            rec[i].time   = recOld[i].time;
        }

        delete [] recOld;

        rec[numRecords].r[0]   = X;
        rec[numRecords].r[1]   = Y;
        rec[numRecords].r[2]   = Z;
        rec[numRecords].energy = Energy;
        rec[numRecords].time   = time;
        numRecords++;
    }

    void AddPoint(double * R, double Energy, double Time) //add new point to the record
    {
        APositionEnergyRecord *recOld = rec;

        rec = new APositionEnergyRecord[numRecords+1];
        for (int i=0; i<numRecords; i++)
        {
            for (int j=0; j<3; j++)
                rec[i].r[j] = recOld[i].r[j];

            rec[i].energy = recOld[i].energy;
            rec[i].time   = recOld[i].time;
        }

        delete [] recOld;

        rec[numRecords].r[0]   = R[0];
        rec[numRecords].r[1]   = R[1];
        rec[numRecords].r[2]   = R[2];
        rec[numRecords].energy = Energy;
        rec[numRecords].time   = Time;
        numRecords++;
    }
};

struct ABaseScanAndReconRecord
{
    APositionEnergyBuffer Points;            // collection of points (elements contain:    position, energy)
    bool                  GoodEvent = true;  // true - good event  (for scan obsolete -> before was used to mark noise events)
                                             // TODO consider moving to AReconRecord
};

struct AScanRecord : public ABaseScanAndReconRecord
{
    //pos+energy+Good  - see Base

    int    ScintType;  // 0 - unknown, 1 - primary, 2 - secondary
    double zStop;      // if secondary, r[2] is start and zStop is stop (cover SecScint z-width)

    //int EventType;   // type of noise event - obsolete
};

struct AReconRecord : public ABaseScanAndReconRecord
{
    //pos+energy+Good  - see Base

    double chi2             = 0;
    bool   fScriptFiltered  = false;
    bool   ReconstructionOK = true;   // result of reconstruiction for this event
    int    EventId;                   // serial number, should be kept if a part of data is purged

    //CoG info
    double xCoG             = 0;
    double yCoG             = 0;
    double zCoG             = 0;
    int    iPMwithMaxSignal = 0;

  void CopyTo(AReconRecord * target) //copy all data to another ReconstructionStructure
  {
      if (target == this) return;
      const int targetSize = target->Points.size();
      const int thisSize   =         Points.size();
      if (targetSize != thisSize) target->Points.Reinitialize(thisSize);

      for (int i = 0; i < thisSize; i++)
      {
          target->Points[i].energy = Points[i].energy;
          target->Points[i].time   = Points[i].time;
          target->Points[i].r[0]   = Points[i].r[0];
          target->Points[i].r[1]   = Points[i].r[1];
          target->Points[i].r[2]   = Points[i].r[2];
      }
      target->xCoG = xCoG;
      target->yCoG = yCoG;
      target->zCoG = zCoG;
      target->iPMwithMaxSignal = iPMwithMaxSignal;
      target->GoodEvent = GoodEvent;
      target->EventId = EventId;
      target->chi2 = chi2;
      target->ReconstructionOK = ReconstructionOK;
      target->fScriptFiltered = fScriptFiltered;
  }

  // future improvements: override (outisde the class!)
  //QDataStream &operator<<(QDataStream &, const AReconRecord &);
  //QDataStream &operator>>(QDataStream &, AReconRecord &);

  void sendToQDataStream(QDataStream & out) const
  {
      out << Points.size();

      for (int i=0; i<Points.size(); i++)
      {
          out << Points.at(i).r[0];
          out << Points.at(i).r[1];
          out << Points.at(i).r[2];
          out << Points.at(i).energy;
          out << Points.at(i).time;
      }

      out << chi2;
      out << ReconstructionOK;
  }

  void unpackFromQDataStream(QDataStream & in)
  {
      int numPoints;
      in >> numPoints;
      if (Points.size() != numPoints)
          Points.Reinitialize(numPoints);

      for (int iPoint=0; iPoint<numPoints; iPoint++)
      {
          in >> Points[iPoint].r[0];
          in >> Points[iPoint].r[1];
          in >> Points[iPoint].r[2];
          in >> Points[iPoint].energy;
          in >> Points[iPoint].time;
      }

      in >> chi2;
      in >> ReconstructionOK;
  }

};

#endif // APOSITIONENERGYRECORDS

