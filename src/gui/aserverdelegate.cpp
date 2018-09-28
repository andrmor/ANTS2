#include "aserverdelegate.h"
#include "aremoteserverrecord.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QCheckBox>

#include <QPixmap>
#include <QPainter>

#include <QDebug>

AServerDelegate::AServerDelegate(ARemoteServerRecord* modelRecord) : QFrame(), modelRecord(modelRecord)
{
    setFrameShape(QFrame::NoFrame);

    //making gui
    QHBoxLayout* l = new QHBoxLayout(this);
    l->setContentsMargins(8, 4, 8, 4);
    l->setSpacing(2);
        labStatus = new QLabel("--");
        l->addWidget(labStatus);

        cbEnabled = new QCheckBox("");
            cbEnabled->setChecked(true);
            QObject::connect(cbEnabled, &QCheckBox::clicked, this, &AServerDelegate::updateModel);
            cbEnabled->setToolTip("Enable / disable this dispatcher. Disabled ones are not used in all operations including status checks.");
        l->addWidget(cbEnabled);

        leName = new QLineEdit("_name_");
            leName->setMaximumWidth(100);
            leName->setMinimumWidth(100);
            QObject::connect(leName, &QLineEdit::editingFinished, this, &AServerDelegate::updateModel);
            QObject::connect(leName, &QLineEdit::editingFinished, this, &AServerDelegate::nameWasChanged);
            leName->setToolTip("Name of the dispatcher. It has only cosmetic function.");
        l->addWidget(leName);

        QLabel* lab = new QLabel("ws://");
            lab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->addWidget(lab);

        leIP = new QLineEdit(modelRecord->IP);
            leIP->setMaximumWidth(100);
            leIP->setMinimumWidth(100);
            QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
            // You may want to use QRegularExpression for new code with Qt 5 (not mandatory).
            QRegExp ipRegex ("^" + ipRange
                         + "\\." + ipRange
                         + "\\." + ipRange
                         + "\\." + ipRange + "$");
            QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
            leIP->setValidator(ipValidator);
            QObject::connect(leIP, &QLineEdit::editingFinished, this, &AServerDelegate::updateModel);
            leIP->setToolTip("IP address of the server dispatcher.");
        l->addWidget(leIP);

        lab = new QLabel(":");
            lab->setAlignment(Qt::AlignCenter);
            lab->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        l->addWidget(lab);

        sbPort = new QSpinBox();
            sbPort->setMaximum(99999);
            sbPort->setValue(modelRecord->Port);
            QObject::connect(sbPort, &QSpinBox::editingFinished, this, &AServerDelegate::updateModel);
            sbPort->setToolTip("Port of the server dispatcher.");
        l->addWidget(sbPort);

        /*
        lab = new QLabel("#thr:");
            lab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->addWidget(lab);

        leiThreads = new QLineEdit("0");
            leiThreads->setMaximumWidth(30);
            leiThreads->setMinimumWidth(30);
            leiThreads->setEnabled(false);
            leiThreads->setReadOnly(true);
            QIntValidator* intVal = new QIntValidator(this);
            leiThreads->setValidator(intVal);
            QObject::connect(leiThreads, &QLineEdit::editingFinished, this, &AServerDelegate::updateModel);
        l->addWidget(leiThreads);

        lab = new QLabel(" ");
            lab->setAlignment(Qt::AlignHCenter);
        l->addWidget(lab);
        */

        labThreads = new QLabel(" #Th: 0");
            labThreads->setAlignment(Qt::AlignCenter);
            labThreads->setToolTip("Number of threads to be used with this dispatcher.");
        l->addWidget(labThreads);

//        lab = new QLabel(" ");
//            lab->setAlignment(Qt::AlignHCenter);
//        l->addWidget(lab);

        labSpeedFactor = new QLabel(" SF: 1.00 ");
            labSpeedFactor->setAlignment(Qt::AlignCenter);
            labSpeedFactor->setToolTip("Speed factor of this dispatcher (the larger is the factor, the more load will be given to this server).");
        l->addWidget(labSpeedFactor);

        pbProgress = new QProgressBar();
            pbProgress->setAlignment(Qt::AlignHCenter);
            pbProgress->setMaximumHeight(15);
            pbProgress->setMaximumWidth(125);
        l->addWidget(pbProgress);

        //l->addStretch();
        this->setLayout(l);

        // done
        updateGui();
}

void AServerDelegate::updateGui()
{
    switch (modelRecord->Status)
    {
    case ARemoteServerRecord::Unknown:
        setIcon(0);
        pbProgress->setEnabled(false);
        pbProgress->setValue(0);
        break;
    case ARemoteServerRecord::Connecting:
        setIcon(0);
        pbProgress->setEnabled(true);
        break;
    case ARemoteServerRecord::Alive:
        setIcon(1);
        pbProgress->setEnabled(true);
        break;
    case ARemoteServerRecord::Dead:
        setIcon(2);
        pbProgress->setEnabled(false);
        pbProgress->setValue(0);
        break;
    case ARemoteServerRecord::Busy:
        setIcon(3);
        pbProgress->setEnabled(false);
        pbProgress->setValue(0);
        break;
    }

    cbEnabled->setChecked(modelRecord->bEnabled);
    leName->setText(modelRecord->Name);
    leIP->setText(modelRecord->IP);
    sbPort->setValue(modelRecord->Port);

    //leiThreads->setText( QString::number(modelRecord->NumThreads) );
    QString st;
    if (modelRecord->NumThreads_Allocated < modelRecord->NumThreads_Possible)
        st =QString(" #Thr: %1/%2").arg(modelRecord->NumThreads_Allocated).arg(modelRecord->NumThreads_Possible);
    else
        st = QString(" #Thr: %1").arg(modelRecord->NumThreads_Allocated);
    labThreads->setText( st );

    labSpeedFactor->setText( QString(" SF: %1 ").arg(modelRecord->SpeedFactor, 3, 'f', 2) );
    pbProgress->setValue(modelRecord->Progress);

    bool bEnabled = cbEnabled->isChecked();
    {
        leName->setEnabled(bEnabled);
        leIP->setEnabled(bEnabled);
        sbPort->setEnabled(bEnabled);
    }

    if (cbEnabled->isChecked())
    {
        if ( modelRecord->Error.isEmpty() )
            pbProgress->setFormat("%p%");
        else
        {
            pbProgress->setFormat("Error");
            pbProgress->setValue(0);
        }
    }
    else
    {
        pbProgress->setFormat("");
        pbProgress->setValue(0);
    }



    emit updateSizeHint(this);
}

void AServerDelegate::updateModel()
{
    modelRecord->bEnabled = cbEnabled->isChecked();
    modelRecord->Name = leName->text();
    modelRecord->IP = leIP->text();
    modelRecord->Port = sbPort->value();
    //modelRecord->NumThreads = leiThreads->text().toInt();

    updateGui();
}

void AServerDelegate::setBackgroundGray(bool flag)
{
    QString ns = ( flag ? "background-color: lightgray" : "");

    QString ss = styleSheet();
    if (ss.contains("background-color: lightgray")) ss.replace("background-color: lightgray", ns);
    else
    {
        if (!ss.isEmpty() && !ss.endsWith(";")) ss.append(';');
        ss.append(ns);
    }
    setStyleSheet(ss);
}

void AServerDelegate::setIcon(int option)
{
    QPixmap pm(QSize(16,16));
    pm.fill(Qt::transparent);
    QPainter b(&pm);

    QColor color;
    if (option == 0 || !cbEnabled->isChecked()) color = Qt::gray;
    else if (option == 1) color = Qt::green;
    else if (option == 2) color = Qt::red;
    else color = Qt::yellow;
    b.setBrush(QBrush(color));
    b.drawEllipse(0, 2, 10, 10);
    labStatus->setPixmap(pm);
}
