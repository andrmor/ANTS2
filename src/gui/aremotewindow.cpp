#include "aremotewindow.h"
#include "ui_aremotewindow.h"
#include "agridrunner.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QCheckBox>
#include <QPlainTextEdit>

ARemoteWindow::ARemoteWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ARemoteWindow)
{
    ui->setupUi(this);
    ui->lwServers->setViewMode(QListView::ListMode);

    ui->twLog->clear();

    AddNewServer();
}

void ARemoteWindow::AddNewServer()
{
    QListWidgetItem* item = new QListWidgetItem();
    ui->lwServers->addItem(item);
    AServerDelegate* d = new AServerDelegate("0.0.0.0", 4321);
    ui->lwServers->setItemWidget(item, d);
    item->setSizeHint(d->sizeHint());
    QObject::connect(d, &AServerDelegate::nameWasChanged, this, &ARemoteWindow::onNameWasChanged);
    ui->lwServers->updateGeometry();
    Delegates << d;

    QPlainTextEdit* pte = new QPlainTextEdit();
    ui->twLog->addTab(pte, "_name_");
}

ARemoteWindow::~ARemoteWindow()
{
    delete ui;

    for (AServerDelegate* d : Delegates) delete d;
    Delegates.clear();
}

void ARemoteWindow::onTextLogReceived(int index, const QString message)
{
    QWidget* w = ui->twLog->widget(index);
    QPlainTextEdit* pte = dynamic_cast<QPlainTextEdit*>(w);
    if (w) pte->appendPlainText(message);
    else qDebug() << "Caanot find pte for index"<<index<<"to display"<<message;

}

void ARemoteWindow::onNameWasChanged()
{
    int index = ui->lwServers->currentRow();
    const QString newName = Delegates.at(index)->getName();
    ui->twLog->setTabText(index, newName);
}

// -------------------- Server delegate ----------------------

AServerDelegate::AServerDelegate(const QString &ip, int port)
{
    setFrameShape(QFrame::NoFrame);

    QHBoxLayout* l = new QHBoxLayout(this);
        QLabel* labStatus = new QLabel("--");
        l->addWidget(labStatus);

        QCheckBox* cbEnabled = new QCheckBox("");
            cbEnabled->setChecked(true);
        l->addWidget(cbEnabled);

        leName = new QLineEdit("_name_");
            leName->setMaximumWidth(75);
            leName->setMinimumWidth(75);
            QObject::connect(leName, &QLineEdit::editingFinished, this, &AServerDelegate::nameWasChanged);
        l->addWidget(leName);

        QLabel* lab = new QLabel("IP:");
        l->addWidget(lab);

        leIP = new QLineEdit(ip);
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
        l->addWidget(leIP);

        lab = new QLabel("Port:");
        l->addWidget(lab);

        sbPort = new QSpinBox();
            sbPort->setMaximum(99999);
            sbPort->setValue(port);
        l->addWidget(sbPort);

        lab = new QLabel("Threads:");
        l->addWidget(lab);

        leThreads = new QLineEdit("n.a.");
            leThreads->setMaximumWidth(40);
            leThreads->setEnabled(false);
            leThreads->setReadOnly(true);
        l->addWidget(leThreads);

        pbProgress = new QProgressBar();
            pbProgress->setAlignment(Qt::AlignHCenter);
        l->addWidget(pbProgress);

        l->addStretch();

        this->setLayout(l);

        pbProgress->setVisible(false);
}

const QString AServerDelegate::getName() const
{
    return leName->text();
}

const QString AServerDelegate::getIP() const
{
    return leIP->text();
}

int AServerDelegate::getPort() const
{
    return sbPort->value();
}

void AServerDelegate::setIP(const QString &ip)
{
    leIP->setText(ip);
}

void AServerDelegate::setPort(int port)
{
    sbPort->setValue(port);
}

void AServerDelegate::setThreads(int threads)
{
    leThreads->setText( QString::number(threads) );
}

void AServerDelegate::setProgress(int progress)
{
    pbProgress->setValue(progress);
}

void AServerDelegate::setProgressVisible(bool flag)
{
    pbProgress->setVisible(flag);
}

void ARemoteWindow::on_pbStatus_clicked()
{
    QVector<ARemoteServerRecord> servers;
    servers << ARemoteServerRecord("127.0.0.1", 1234);

    AGridRunner* GR = new AGridRunner();
    QObject::connect(GR, &AGridRunner::requestTextLog, this, &ARemoteWindow::onTextLogReceived, Qt::QueuedConnection);

    GR->CheckStatus(servers);
    qDebug() << "Status click processing finished!";
    GR->deleteLater();
}
