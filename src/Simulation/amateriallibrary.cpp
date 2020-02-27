#include "amateriallibrary.h"
//#include "afiletools.h"
#include "ajsontools.h"
#include "amessage.h"
#include "amaterialparticlecolection.h"

#include <QFileDialog>
#include <QVector>

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QCheckBox>
#include <QPushButton>

AMaterialLibrary::AMaterialLibrary(AMaterialParticleCollection & MpCollection) :
    MpCollection(MpCollection)
{

}

QString AMaterialLibrary::LoadFile(const QString & fileName, QWidget * parentWidget)
{
    QJsonObject json, js;
    bool bOK = LoadJsonFromFile(json, fileName);
    if (!bOK)
        return "Cannot open file: "+fileName;
    if (!json.contains("Material"))
        return "File format error: Json with material settings not found";
    js = json["Material"].toObject();
    QVector<QString> newParticles = MpCollection.getUndefinedParticles(js);

    QVector<QString> suppressParticles;
    if (!newParticles.isEmpty())
    {
        QDialog D(parentWidget);
        D.setWindowTitle("Add particles to current configuration");
        QVBoxLayout * l = new QVBoxLayout(&D);
        l->addWidget(new QLabel("This material has data for particles not defined in the current configuration"));
        l->addWidget(new QLabel(""));
        l->addWidget(new QLabel("Select particles to add to the configuration"));
        QCheckBox * cbAll = nullptr;
        QVector<QCheckBox*> cbVec;
        if (newParticles.size() > 1)
        {
            cbAll = new QCheckBox("All particles below:");
            cbAll->setChecked(true);
            l->addWidget(cbAll);
            QObject::connect(cbAll, &QCheckBox::toggled, [&cbVec](bool flag)
            {
               for (QCheckBox * cb : cbVec) cb->setChecked(flag);
            });
        }

        QScrollArea * SA = new QScrollArea();
            SA->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            SA->setMaximumHeight(500);
            SA->setWidgetResizable(true);

            QWidget * www = new QWidget();
            QVBoxLayout * l2 = new QVBoxLayout(www);
            for (const QString & name : newParticles)
            {
                QCheckBox * cb = new QCheckBox(name);
                cb->setChecked(true);
                cbVec << cb;
                l2->addWidget(cb);
                if (cbAll)
                    QObject::connect(cb, &QCheckBox::clicked, [&cbAll]()
                    {
                        cbAll->blockSignals(true);
                        cbAll->setChecked(false);
                        cbAll->blockSignals(false);
                    });
            }

            SA->setWidget(www);
        l->addWidget(SA);

        QHBoxLayout * h = new QHBoxLayout();
            QPushButton * pbAccept = new QPushButton("Import");
            QObject::connect(pbAccept, &QPushButton::clicked, &D, &QDialog::accept);
            h->addWidget(pbAccept);
            QPushButton * pbCancel = new QPushButton("Cancel");
            QObject::connect(pbCancel, &QPushButton::clicked, &D, &QDialog::reject);
            h->addWidget(pbCancel);
        l->addLayout(h);

        int res = D.exec();
        if (res == QDialog::Rejected) return "rejected";

        for (int i = 0; i < cbVec.size(); i++)
            if (!cbVec.at(i)->isChecked())
                suppressParticles << newParticles.at(i);
    }

    MpCollection.tmpMaterial.readFromJson(js, &MpCollection, suppressParticles);

    return "";
}
