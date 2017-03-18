#ifndef APARTICLEONSTACK
#define APARTICLEONSTACK

class AParticleOnStack //element of the particle stack
{
  public:
    AParticleOnStack(int Id,
                     double x, double y, double z,
                     double vx, double vy, double vz,
                     double time, double energy,
                     int secondaryOf = -1) :
        Id(Id), time(time), energy(energy), secondaryOf(secondaryOf)
    {
        r[0]=x; r[1]=y; r[2]=z;
        v[0]=vx; v[1]=vy; v[2]=vz;
    }
    AParticleOnStack(){}

    int Id;    
    double r[3];     //starting point
    double v[3];     //starting vector
    double time;     //time on start
    double energy;   //staring energy
    int secondaryOf; //use in primary tracker to designate secondary particles and link to their primary

    AParticleOnStack* clone()
    {
       return new AParticleOnStack(Id, r[0], r[1], r[2], v[0], v[1], v[2], time, energy, secondaryOf);
    }
};

#endif // APARTICLEONSTACK

