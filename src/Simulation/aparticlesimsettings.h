#ifndef APARTICLEMODESETTINGS_H
#define APARTICLEMODESETTINGS_H

class QJsonObject;

class ASourceParticleGenerator;
class AFileParticleGenerator;
class AScriptParticleGenerator;

class AParticleSimSettings
{
public:
    AParticleSimSettings(ASourceParticleGenerator * SoG, AFileParticleGenerator * FiG, AScriptParticleGenerator * ScG);

    enum AGenMode   {Sources = 0, File = 1, Script = 2};
    enum AMultiMode {Constant = 0, Poisson = 1};

    AGenMode GenerationMode = Sources;
    int     EventsToDo      = 1;
    bool    bMultiple       = false;        //affects only "Sources", maybe move there?
    int     MeanPerEvent    = 1;
    AMultiMode MultiMode    = Constant;
    bool    bDoS1           = true;
    bool    bDoS2           = false;
    bool    bIgnoreNoHits   = false;
    bool    bIgnoreNoDepo   = false;
    bool    bClusterMerge   = false;
    double  ClusterRadius   = 0.1;
    double  ClusterTime     = 1.0;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
    void clearSettings();

private:
    // !*! next round of conversion - remove dependence on generators by splitting each to two classes: settings + generator
    ASourceParticleGenerator * SourceGen = nullptr;
    AFileParticleGenerator   * FileGen   = nullptr;
    AScriptParticleGenerator * ScriptGen = nullptr;

};

#endif // APARTICLEMODESETTINGS_H
