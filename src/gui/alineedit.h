#ifndef ALINEEDIT_H
#define ALINEEDIT_H

#include <QLineEdit>

class ALineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ALineEdit(QWidget *parent = 0) : QLineEdit(parent) {}
    explicit ALineEdit(const QString& text, QWidget *parent = 0) : QLineEdit(text, parent) {}

    void setCompleter(QCompleter*);
    QCompleter *completer() const {return c; }

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    QString cursorWord(const QString& sentence) const;

private slots:
    void insertCompletion(QString);

private:
    QCompleter* c = 0;
};


#endif // ALINEEDIT_H
