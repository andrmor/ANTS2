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

  //added after XCOm site changed to HTTPS
  QSslConfiguration config = QSslConfiguration::defaultConfiguration();
  config.setProtocol(QSsl::AnyProtocol);
  request.setSslConfiguration(config);
  //

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  QObject::connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinished(QNetworkReply *)));

  fWait = true;
  ReplyMessage.clear();
  QElapsedTimer t;
  t.start();
  //    qDebug() << "Posting...";
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
  if (replyReceived.isEmpty())
  {
      error = "Check that your system has OpenSSL drivers!\ne.g. for Win32: ssleay32.dll, libssl32.dll and libeay32.dll";
      return false;
  }
  return true;
}

void AInternetBrowser::replyFinished(QNetworkReply *reply)
{
  //    qDebug() << "Reply received!";
  replyReceived = reply->readAll();
  //    qDebug() << replyReceived;
  //    qDebug() << "Errors:"<<reply->errorString();
  fWait = false;
}

