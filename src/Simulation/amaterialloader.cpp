#include "amaterialloader.h"
#include "ajsontools.h"
#include "amaterialparticlecolection.h"
#include "amateriallibrarybrowser.h"
#include "amaterialloaderdialog.h"

#include <QString>
#include <QVector>

AMaterialLoader::AMaterialLoader(AMaterialParticleCollection & MpCollection) :
    MpCollection(MpCollection) {}

bool AMaterialLoader::LoadTmpMatFromGui(QWidget *parentWidget)
{
    AMaterialLibraryBrowser B(MpCollection, parentWidget);
    int ret = B.exec();
    if (ret == QDialog::Rejected) return false;

    QString fileName = B.getFileName();
    if (fileName.isEmpty()) return false;

    QJsonObject MaterialJson;
    QVector<QString> SuppressParticles;

    if (B.isAdvancedLoadRequested())
    {
        AMaterialLoaderDialog D(fileName, MpCollection, parentWidget);
        ret = D.exec();
        if (ret == QDialog::Rejected) return false;

        MaterialJson = D.getMaterialJson();
        SuppressParticles = D.getSuppressedParticles();
    }
    else
    {
        QJsonObject json;
        LoadJsonFromFile(json, fileName);
        MaterialJson = json["Material"].toObject();
    }

    MpCollection.tmpMaterial.readFromJson(MaterialJson, &MpCollection, SuppressParticles);
    MpCollection.CopyTmpToMaterialCollection();
    return true;
}
