#ifndef ALICE_H
#define ALICE_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTime>
#include <QSettings>
#include <QFile>
#include <QCloseEvent>
#include <QListWidget>
#include "packet.h"
#include <QTest>

namespace Ui {
class Bob;
}

class Bob : public QMainWindow
{
    Q_OBJECT

public:
    explicit Bob(QWidget *parent = 0);
    QTcpSocket* m_pTcpSocket;
    quint64     m_nNextBlockSize;
    qint32      transaction_no;
    qint32     id;
    qint32      port;
    ~Bob();

public slots:
    void slotReadyRead   (                            );
    void slotError       (QAbstractSocket::SocketError);
    void slotSendToServer(Packet& str                 );
    void slotConnected   (                            );

private slots:
    void on_startStopAction_triggered();

    void on_horizontalNormal_clicked();

    void on_horizontalInverted_clicked();

    void on_verticalNormal_clicked();

    void on_verticalInverted_clicked();


private:
    Ui::Bob *ui;
    void loadSettings();
    void saveSettings();
    void startClient(int port);
    void stopClient();
    QString m_settingsFile;
    void closeEvent(QCloseEvent *event);
};

#endif // ALICE_H
