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
class Alice;
}

class Alice : public QMainWindow
{
    Q_OBJECT

public:
    explicit Alice(QWidget *parent = 0);
    QTcpSocket* m_pTcpSocket;
    quint64     m_nNextBlockSize;
    qint32      transaction_no;
    QString     id;
    qint32      port;
    ~Alice();

public slots:
    void slotReadyRead   (                            );
    void slotError       (QAbstractSocket::SocketError);
    void slotSendToServer(Packet& str                 );
    void slotConnected   (                            );

private slots:
    void on_generateButton_clicked();

    void on_exchangeButton_clicked();

    void on_messageButton_clicked(bool retry);

    void on_startStopAction_triggered();

    void on_horizontalNormal_clicked();

    void on_horizontalInverted_clicked();

    void on_verticalNormal_clicked();

    void on_verticalInverted_clicked();


    void on_vectorRadio_toggled(bool checked);

    void on_lengthRadio_toggled(bool checked);

private:
    Ui::Alice *ui;
    void loadSettings();
    void saveSettings();
    void startClient(int port);
    void stopClient();
    QString m_settingsFile;
    void closeEvent(QCloseEvent *event);
};

#endif // ALICE_H
