#ifndef SEARCHER_H
#define SEARCHER_H

#include <QApplication>
#include <QMainWindow>
#include <QMap>
#include <QThread>
#include <QCommonStyle>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QDirIterator>

class Searcher :  public QObject {
    Q_OBJECT

public:
    void get_duplicates(QString const &dir);
signals:
    void send_duplicates(QVector<QString> const &);
    void finish();
    void progress(qint8 const &);

private:
    QByteArray get_hash(QString const &filepath);
    QString get_first_k(QString const &filepath, qint64 k);
    bool checkStop();
};

#endif // SEARCHER_H
