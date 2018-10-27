#ifndef APARTICLERECORD_H
#define APARTICLERECORD_H

class TRandom2;

class AParticleRecord
{
public:
    AParticleRecord(int Id,
                    double x, double y, double z,
                    double vx, double vy, double vz,
                    double time, double energy,
                    int secondaryOf = -1);

    int     Id;          //type of particle in MatPart collection
    double  r[3];        //starting point
    double  v[3];        //starting vector
    double  time;        //time on start
    double  energy;      //staring energy
    int     secondaryOf; //use in primary tracker to designate secondary particles and link to their primary

    AParticleRecord * clone(); // TODO no need?
    void ensureUnitaryLength();
    void randomDir(TRandom2 * RandGen);
};

#endif // APARTICLERECORD_H
