#ifndef Server_H
#define Server_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTime>
#include <QTimer>
#include <QSettings>
#include <QFile>
#include <QCloseEvent>
#include "packet.h"
#include <QtWidgets/QListWidget>
#include <QtWidgets/QLabel>
#include <QTest>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>

namespace Ui {
class Server;
class Tcp;
class QTcpServer;
class QTcpSocket;
}

class Tcp : public QTcpServer
{
    Q_OBJECT

public:
    explicit     Tcp(Ui::Server *parent = 0, qint32 port=0);
    QString      id;
    qint32       transaction_no;
    quint64      m_nNextBlockSize;
    qint32       port;
    QLabel      *label;
    QTcpSocket  *socket;

    void sendToClient(Packet& packet);
    void stopServer();
public slots:
    virtual void slotNewConnection();
            void slotDelete();
            void slotReadClient   ();
};


class Server : public QMainWindow
{
    Q_OBJECT

public:
    explicit    Server(QWidget *parent = 0);
    Tcp*        alice;
    Tcp*        bob;
    Tcp*        eve;

    Ui::Server *ui;
    ~Server();

private slots:
    void drawing();
    void on_exit_triggered();
    void on_loadFromFIle_triggered();
    void on_saveToFile_triggered();

    void on_startStopAction_triggered();

    void on_eveSpy_toggled(bool checked);

    void on_speedSlider_valueChanged(int value);

    void on_noiseLevel_editingFinished();

    void on_photonCount_valueChanged(int arg1);

    void on_photonProbability_valueChanged(int arg1);

    void on_noiseCheck_toggled(bool checked);

    void on_multiCheck_toggled(bool checked);

    void on_rejectCheck_toggled(bool checked);

    void on_photonProbabilityRadio_toggled(bool checked);

    void on_channelLengthRadio_toggled(bool checked);

    void on_channelLength_valueChanged(int arg1);

private:
    void loadSettings();
    void saveSettings();
    QString m_settingsFile;

    void closeEvent(QCloseEvent *event);
};

#endif // Server_H
