#ifndef AENERGYDEPOSITIONCELL
#define AENERGYDEPOSITIONCELL

// ***!!! index / eventId - check what is not needed anymore

struct AEnergyDepositionCell  //element of the EnergyVector QVector
    {
        double r[3];  // x,y,z -coordinates of the cell
        double time;
        double dE; //energy deposited in this cell
        int ParticleId;          // index of the particle form the particle list
        int MaterialId;         //  index of the material
        int index;  //particle number - need for visualization if tracking several particles with same id - index continuosly increases throughout the EnergyVector
        int eventId; //for EnergyVector import mode - event number

        AEnergyDepositionCell(double* r, double time, double dE, int ParticleId, int MaterialId, int index, int eventId)
          : time(time), dE(dE),
            ParticleId(ParticleId), MaterialId(MaterialId),
            index(index), eventId(eventId) {this->r[0]=r[0]; this->r[1]=r[1]; this->r[2]=r[2];}
        AEnergyDepositionCell(double x, double y, double z, double time, double dE, int ParticleId, int MaterialId, int index, int eventId)
          : time(time), dE(dE),
            ParticleId(ParticleId), MaterialId(MaterialId),
            index(index), eventId(eventId) {this->r[0] = x; this->r[1] = y; this->r[2] = z;}
        AEnergyDepositionCell(){}

        bool isCloser(double length2, const double* R) const
        {
            double d2 = 0;
            for (int i=0; i<3; i++)
            {
                double delta = r[i] - R[i];
                d2 += delta * delta;
            }
            return d2 < length2;
        }
    };

#endif // AENERGYDEPOSITIONCELL

