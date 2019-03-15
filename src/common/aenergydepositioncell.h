#ifndef AENERGYDEPOSITIONCELL
#define AENERGYDEPOSITIONCELL

struct AEnergyDepositionCell  //element of the EnergyVector QVector
    {
        double r[3];  // x,y,z -coordinates of the cell
        double cellLength; //cell length for charged particles
        double time;
        double dE; //energy deposited in this cell
        int ParticleId;          // index of the particle form the particle list
        int MaterialId;         //  index of the material
        int index;  //particle number - need for visualization if tracking several particles with same id - index continuosly increases throughout the EnergyVector
        int eventId; //for EnergyVector import mode - event number

        AEnergyDepositionCell(double* r, double cellLength, double time, double dE, int ParticleId, int MaterialId, int index, int eventId)
          : cellLength(cellLength), time(time), dE(dE),
            ParticleId(ParticleId), MaterialId(MaterialId),
            index(index), eventId(eventId) {this->r[0]=r[0]; this->r[1]=r[1]; this->r[2]=r[2];}
        AEnergyDepositionCell(double x, double y, double z, double cellLength, double time, double dE, int ParticleId, int MaterialId, int index, int eventId)
          : cellLength(cellLength), time(time), dE(dE),
            ParticleId(ParticleId), MaterialId(MaterialId),
            index(index), eventId(eventId) {this->r[0] = x; this->r[1] = y; this->r[2] = z;}
        AEnergyDepositionCell(){}
    };

#endif // AENERGYDEPOSITIONCELL

