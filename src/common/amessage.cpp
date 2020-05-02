#include "amessage.h"

#ifdef GUI
#include <QMessageBox>
#include <QInputDialog>

void message(QString text, QWidget* parent)
{
  QMessageBox mb(parent);
  mb.setWindowFlags(mb.windowFlags() | Qt::WindowStaysOnTopHint);
  mb.setText(text);
  if (!parent) mb.move(200,200);
  mb.exec();
}

bool confirm(const QString & text, QWidget * parent)
{
    QMessageBox::StandardButton reply = QMessageBox::question(parent, "Confirmation request", text,
                                    QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    if (reply == QMessageBox::Yes) return true;
    return false;
}

void inputInteger(const QString &text, int &input, int min, int max, QWidget *parent)
{
    bool ok;
    int res = QInputDialog::getInt(parent, "", text, input, min, max, 1, &ok);
    if (ok) input = res;
}

#else

void message(QString text)
{
    std::cout << text.toStdString() << std::endl;
}

#endif

#include <QDialog>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
void message1(const QString & text, const QString & title, QWidget *parent)
{
    QDialog d(parent);
    QVBoxLayout * l = new QVBoxLayout(&d);
    QPlainTextEdit * e = new QPlainTextEdit;
        e->appendPlainText(text);
    l->addWidget(e);
    QPushButton * pb = new QPushButton("Close");
        QObject::connect(pb, &QPushButton::clicked, &d, &QDialog::accept);
    l->addWidget(pb);

    QTextCursor curs = e->textCursor();
    curs.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    e->setTextCursor(curs);

    d.resize(800, 400);
    d.setWindowTitle(title);
    d.exec();
}
