#include "ainternetbrowser.h"

#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QApplication>

AInternetBrowser::AInternetBrowser(int timeout_ms) :
  fWait(false), timeout(timeout_ms) {}

bool AInternetBrowser::Post(QString Url, const QString OutgoingMessage, QString &ReplyMessage)
{
  if (fWait)
    {
      error = "Still waiting for reply";
      return false;
    }

  error.clear();

  QUrl url(Url);
  if (!url.isValid())
    {
      error = "Not valid url";
      return false;
    }

  QNetworkAccessManager *manager = new QNetworkAccessManager(this);
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  QObject::connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinished(QNetworkReply *)));

  fWait = true;
  ReplyMessage.clear();
  QElapsedTimer t;
  t.start();
  manager->post(request, OutgoingMessage.toLatin1());
  do
    {
      qApp->processEvents();
      if (t.elapsed()>timeout)
        {
          error = "Timeout";
          fWait = false;
          return false;
        }
    }
  while (fWait);

  ReplyMessage = replyReceived;
  return true;
}

void AInternetBrowser::replyFinished(QNetworkReply *reply)
{
  qDebug() << "Reply received!";
  replyReceived = reply->readAll();
  qDebug() << replyReceived;
  fWait = false;
}

