#ifndef ASCRIPTMESSENGERDIALOG_H
#define ASCRIPTMESSENGERDIALOG_H

#include <QObject>
#include <QVector>
#include <QString>

class QWidget;
class QDialog;
class QPlainTextEdit;

class AScriptMessengerDialog : public QObject
{
    Q_OBJECT

public:
    AScriptMessengerDialog(QWidget *parent);
    ~AScriptMessengerDialog();

    bool IsShown() const;
    void RestoreWidget();
    void HideWidget();

public slots:
    void Show();
    void Hide();
    void Clear();
    void Append(const QString& text);
    void ShowTemporary(int ms);

    void SetTransparent(bool flag);
    void SetDialogTitle(const QString& title);
    void Move(double x, double y);
    void Resize(double w, double h);
    void SetFontSize(int size);

private:
    QWidget* Parent;

    QDialog* D;
    QPlainTextEdit* e;

    bool bShowStatus = false;
    double X = 50, Y = 50;
    double WW = 300, HH = 500;
    QString Title = "Script messenger";

    void init(bool bTransparent);
};

#endif // ASCRIPTMESSENGERDIALOG_H
