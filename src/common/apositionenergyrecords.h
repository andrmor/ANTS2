#ifndef APOSITIONENERGYRECORDS
#define APOSITIONENERGYRECORDS

#include <QDataStream>
#include <QDebug>

struct APositionEnergyRecord
{
    double r[3];
    double energy;

    APositionEnergyRecord& operator=(const APositionEnergyRecord& other)
    {
        if (this != &other)
        {
            energy = other.energy;
            for (int i=0; i<3; i++)
                r[i] = other.r[i];
        }
        return *this;
    }

    friend bool operator< (const APositionEnergyRecord& lhs, const APositionEnergyRecord& rhs)
    {
        return (lhs.energy < rhs.energy);
    }

    bool isCloser(double length2, const APositionEnergyRecord & other) const
    {
        double d2 = 0;
        for (int i=0; i<3; i++)
        {
            double delta = r[i] - other.r[i];
            d2 += delta * delta;
        }
        return d2 < length2;
    }

    void MergeWith(const double R[3], double E)
    {
        if (E > 0)
        {
            for (int i=0; i<3; i++)
                r[i] = (r[i]*energy + R[i]*E) / (energy + E);
            energy += E;
        }
    }

    void MergeWith(const APositionEnergyRecord & other)
    {
        if (other.energy > 0)
        {
            for (int i=0; i<3; i++)
                r[i] = (r[i]*energy + other.r[i]*other.energy) / (energy + other.energy);
            energy += other.energy;
        }
    }
};

class APositionEnergyBuffer
{
 private:
    int numRecords;

    APositionEnergyBuffer(const APositionEnergyBuffer &) { } // not copiable!
    APositionEnergyBuffer &operator=(const APositionEnergyBuffer &) { return *this; } // not copiable!

 public:
    APositionEnergyRecord *rec; //pointer to array of position energy records

    APositionEnergyBuffer() { rec = new APositionEnergyRecord[1]; numRecords = 1;}   //by default 1 point!
    ~APositionEnergyBuffer(){ delete [] rec; }

    //public functions
    inline APositionEnergyRecord &operator[](int i) const { return rec[i]; } //get record number i
    inline const APositionEnergyRecord &at(int i) const { return rec[i]; }

    inline int size() const {return numRecords;}          //get number of points
    inline void Reinitialize(int size)              //resize (data is undefined!)
      {
         delete [] rec;
         rec = new APositionEnergyRecord[size];
         numRecords = size;
      }
    void AddPoint(double X, double Y, double Z, double Energy) //add new point to the record
      {
        APositionEnergyRecord *recOld = rec;

        rec = new APositionEnergyRecord[numRecords+1];
        for (int i=0; i<numRecords; i++)
          {
            for (int j=0; j<3; j++) rec[i].r[j] = recOld[i].r[j];
            rec[i].energy = recOld[i].energy;
          }
        delete [] recOld;
        rec[numRecords].r[0] = X; rec[numRecords].r[1] = Y; rec[numRecords].r[2] = Z;
        rec[numRecords].energy = Energy;
        numRecords++;
      }
    void AddPoint(double* R, double Energy) //add new point to the record
      {
        APositionEnergyRecord *recOld = rec;

        rec = new APositionEnergyRecord[numRecords+1];
        for (int i=0; i<numRecords; i++)
          {
            for (int j=0; j<3; j++) rec[i].r[j] = recOld[i].r[j];
            rec[i].energy = recOld[i].energy;
          }
        delete [] recOld;
        rec[numRecords].r[0] = R[0]; rec[numRecords].r[1] = R[1]; rec[numRecords].r[2] = R[2];
        rec[numRecords].energy = Energy;
        numRecords++;
      }
};

struct ABaseScanAndReconRecord
{
   APositionEnergyBuffer Points;  //collection of points (elements contain:    position, energy)
   bool GoodEvent; //true - good event  (for scan - bad event = noise event)
};

struct AScanRecord : public ABaseScanAndReconRecord
{
  //pos+energy+Good  - see Base
  int EventType; //type of noise event

  int ScintType; //0 - unknown, 1 - primary, 2 - secondary
  double zStop;   //if secondary, r[2] is start and zStop is stop (cover SecScint z-width)

  AScanRecord() {GoodEvent = true;}  //by default there is one point //good event by defaut
};

struct AReconRecord : public ABaseScanAndReconRecord
{
  //pos+energy+Good  - see Base
  double chi2;
  bool fScriptFiltered;
  bool ReconstructionOK; // false = reconstruction failed
  int EventId; //serial number, should be kept if a part of data is purged

    //CoG info
  double xCoG, yCoG, zCoG;
  int iPMwithMaxSignal;

  AReconRecord() :
    chi2(0), fScriptFiltered(false),
    xCoG(0), yCoG(0), zCoG(0),
    iPMwithMaxSignal(0) {}  //by default there is one point

  void CopyTo(AReconRecord* target) //copy all data to another ReconstructionStructure
    {
      if (target == this) return;
      int targetSize = target->Points.size();
      int thisSize = Points.size();
      if (targetSize != thisSize) target->Points.Reinitialize(thisSize);
      for (int i=0; i<thisSize; i++)
        {
          target->Points[i].energy = Points[i].energy;
          target->Points[i].r[0] = Points[i].r[0];
          target->Points[i].r[1] = Points[i].r[1];
          target->Points[i].r[2] = Points[i].r[2];
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
      }

      in >> chi2;
      in >> ReconstructionOK;
  }

};

#endif // APOSITIONENERGYRECORDS

