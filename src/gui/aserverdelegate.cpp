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
        l->addWidget(cbEnabled);

        leName = new QLineEdit("_name_");
            leName->setMaximumWidth(100);
            leName->setMinimumWidth(100);
            QObject::connect(leName, &QLineEdit::editingFinished, this, &AServerDelegate::updateModel);
            QObject::connect(leName, &QLineEdit::editingFinished, this, &AServerDelegate::nameWasChanged);
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
        l->addWidget(leIP);

        lab = new QLabel(":");
            lab->setAlignment(Qt::AlignCenter);
            lab->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        l->addWidget(lab);

        sbPort = new QSpinBox();
            sbPort->setMaximum(99999);
            sbPort->setValue(modelRecord->Port);
            QObject::connect(sbPort, &QSpinBox::editingFinished, this, &AServerDelegate::updateModel);
        l->addWidget(sbPort);

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

        pbProgress = new QProgressBar();
            pbProgress->setAlignment(Qt::AlignHCenter);
            pbProgress->setMaximumHeight(15);
            pbProgress->setMaximumWidth(125);
        l->addWidget(pbProgress);

        //l->addStretch();
        this->setLayout(l);

        // done

        updateGui();
        //qDebug() << "STYLEEEEEEEEEEEEEE"<<styleSheet();
}

void AServerDelegate::updateGui()
{
    switch (modelRecord->Status)
    {
    case ARemoteServerRecord::Unknown:
        setBackgroundGray(true);
        setIcon(0);
        pbProgress->setEnabled(false);
        pbProgress->setValue(0);
        break;
    case ARemoteServerRecord::Connecting:
        setBackgroundGray(true);
        setIcon(0);
        pbProgress->setEnabled(true);
        break;
    case ARemoteServerRecord::Progressing:
        setBackgroundGray(false);
        setIcon(1);
        pbProgress->setEnabled(true);
        break;
    case ARemoteServerRecord::Alive:
        setBackgroundGray(false);
        setIcon(1);
        pbProgress->setEnabled(false);
        break;
    case ARemoteServerRecord::Dead:
        setBackgroundGray(true);
        setIcon(2);
        pbProgress->setEnabled(false);
        pbProgress->setValue(0);
        break;
    }

    cbEnabled->setEnabled(modelRecord->bEnabled);
    leName->setText(modelRecord->Name);
    leIP->setText(modelRecord->IP);
    sbPort->setValue(modelRecord->Port);
    leiThreads->setText( QString::number(modelRecord->NumThreads) );
    pbProgress->setValue(modelRecord->Progress);

    emit updateSizeHint(this);
}

void AServerDelegate::updateModel()
{
    modelRecord->bEnabled = cbEnabled->isChecked();
    modelRecord->Name = leName->text();
    modelRecord->IP = leIP->text();
    modelRecord->Port = sbPort->value();
    modelRecord->NumThreads = leiThreads->text().toInt();
}

void AServerDelegate::setBackgroundGray(bool flag)
{
    return;


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
    if (option == 0) color = Qt::gray;
    else if (option == 1) color = Qt::green;
    else color = Qt::red;
    b.setBrush(QBrush(color));
    b.drawEllipse(0, 2, 10, 10);
    labStatus->setPixmap(pm);
}

void AServerDelegate::setThreads(int threads)
{
    leiThreads->setText( QString::number(threads) );
}

void AServerDelegate::setProgress(int progress)
{
    pbProgress->setValue(progress);
}

void AServerDelegate::setProgressVisible(bool flag)
{
    pbProgress->setVisible(flag);
}
