#ifndef AINTERNETBROWSER_H
#define AINTERNETBROWSER_H

#include <QObject>

class QNetworkReply;

class AInternetBrowser : public QObject
{
  Q_OBJECT

public:
  AInternetBrowser(int timeout_ms = 3000);

  bool Post(QString Url, const QString OutgoingMessage, QString &ReplyMessage);

  void Abort() {fWait = false;}
  bool IsWaiting() const {return fWait;}
  QString GetLastError() const {return error;}

private slots:
  void replyFinished(QNetworkReply *reply);

private:
  bool fWait;
  int timeout;
  QString error;
  QString replyReceived;

};

#endif // AINTERNETBROWSER_H
