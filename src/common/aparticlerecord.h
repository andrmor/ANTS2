#ifndef APARTICLERECORD_H
#define APARTICLERECORD_H

class TRandom2;
class AParticleTrackingRecord;

class AParticleRecord
{
public:
    AParticleRecord(int Id,
                    double x, double y, double z,
                    double vx, double vy, double vz,
                    double time, double energy,
                    int secondaryOf = -1);
    AParticleRecord(){}

    int     Id;          //type of particle in MatPart collection
    double  r[3];        //starting point
    double  v[3];        //starting vector
    double  time = 0;        //time on start
    double  energy;      //staring energy
    int     secondaryOf = -1; //use in primary tracker to designate secondary particles and link to their primary ***!!! to change to bool

    AParticleRecord * clone(); // TODO no need?
    void ensureUnitaryLength();
    void randomDir(TRandom2 * RandGen);

    // runtime properties
    AParticleTrackingRecord * ParticleRecord = nullptr; // used only of log is on!  it is != nullptr if secondary
};

#endif // APARTICLERECORD_H
