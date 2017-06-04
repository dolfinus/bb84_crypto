#ifndef EVE_H
#define EVE_H

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

#define ATTACK_IDEAL 1
#define ATTACK_MULTI_SEND 2
#define ATTACK_MULTI_REJECT 3

namespace Ui {
class Eve;
}

class Eve : public QMainWindow
{
    Q_OBJECT

public:
    explicit Eve(QWidget *parent = 0);
    QTcpSocket* m_pTcpSocket;
    quint64     m_nNextBlockSize;
    qint32      transaction_no;
    qint32     id;
    qint32      port;
    ~Eve();

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

 //   void on_logCheckBox_toggled(bool checked);

//    void log(QString item);

    void on_idealButton_toggled(bool checked);

    void on_multiPhotonButton_toggled(bool checked);

    void on_multiSendButton_toggled(bool checked);

    void on_multiRejectButton_toggled(bool checked);

    void on_photonCount_valueChanged(int arg1);

private:
    Ui::Eve *ui;
    void loadSettings();
    void saveSettings();
    void startClient(int port);
    void stopClient();
    QString m_settingsFile;
    void closeEvent(QCloseEvent *event);
};

#endif // ALICE_H
