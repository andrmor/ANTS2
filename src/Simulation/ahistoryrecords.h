#ifndef AHISTORYRECORDS
#define AHISTORYRECORDS

struct GeneratedPhotonsHistoryStructure
{
    int event;
    int index; // to distinguish between particles with the same ParticleId (e.g. during the same event)
    int ParticleId;
    int MaterialId;
    double Energy;
    int PrimaryPhotons;
    int SecondaryPhotons;
};

#endif // AHISTORYRECORDS

