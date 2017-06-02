#include "server.h"
#include "ui_server.h"

char VERTICAL=1;
char HORIZONTAL=0;

Server *main_server;
qint32  alice_port;
qint32  bob_port;
qint32  eve_port;

QTimer *timer;
QPoint  text_pos;
qint32  margin;
qint32  tick;
QString text;
QGraphicsTextItem *text_item;
QGraphicsLineItem *first_line;
QGraphicsLineItem *second_line;
QGraphicsLineItem *third_line;
QGraphicsLineItem *forth_line;

int     sleep;
bool    spy;
QString source;
QString destination;

bool    noise;
bool    multi;
bool    reject;
bool    photon;

int     noise_level;
int     photon_count;
int     photon_probatility;
int     channel_length;


Tcp::Tcp(Ui::Server *parent, qint32 _port) :
    QTcpServer()
{
    id="server";
    transaction_no=0;
    m_nNextBlockSize=0;
    port=_port;
    if (port == alice_port) {
        label=parent->aliceLabel;
    } else if (port == bob_port) {
        label=parent->bobLabel;
    } else if (port == eve_port) {
        label=parent->eveLabel;
    } else {
        label=NULL;
    }
    socket=NULL;

    if (!this->listen(QHostAddress::LocalHost, port)) {
        //Невозможно запустить сервер
        this->close();
        return;
    }
    connect(this, SIGNAL(  newConnection()),
            this, SLOT(slotNewConnection())
           );
}

Server::Server(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Server)
{
    main_server=this;
    ui->setupUi(main_server);

    margin=20;

    QGraphicsScene *scene = new QGraphicsScene(0,0,ui->graphicsView->width()-5,ui->graphicsView->height()-5,ui->graphicsView);
    ui->graphicsView->setScene(scene);

    m_settingsFile = QApplication::applicationDirPath() + "/settings.ini";
    loadSettings();

    alice = new Tcp(ui, alice_port);
    bob   = new Tcp(ui, bob_port  );
    eve   = new Tcp(ui, eve_port  );

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()), this, SLOT(drawing()));

    tick=10;

}

void Server::drawing(){
    qint32 width  = ui->graphicsView->scene()->width( );
    qint32 height = ui->graphicsView->scene()->height();

    qint32 vmiddle = height/2;
    qint32 hmiddle = width/2;

    if (text_item){

        if (text_pos.x() < 0 || text_pos.x() > width || text_pos.y() > height) timer->stop();

        if (text_pos == QPoint(0,0)){
            if(source == "alice") text_pos = QPoint(1,      vmiddle+1);
            if(source == "bob"  ) text_pos = QPoint(width-1,vmiddle+1);

            if(source == "eve"  ) {
                if (destination == "alice"){
                    text_pos = QPoint(hmiddle-margin, height);
                } else {
                    text_pos = QPoint(hmiddle+margin, height);
                }
            }
        }

        if(source == "alice") {
            if (destination == "bob" || ( destination == "eve" && text_pos.x() < hmiddle-margin)){
                text_pos.setX(text_pos.x()+tick);
            } else {
                text_pos.setY(text_pos.y()+tick);
            }
        } else if(source == "bob") {
            if (destination == "alice" || ( destination == "eve" && text_pos.x() > hmiddle+margin)){
                text_pos.setX(text_pos.x()-tick);
            } else {
                text_pos.setY(text_pos.y()+tick);
            }
        } else if(source == "eve"){
            if (text_pos.y() > vmiddle){
                text_pos.setY(text_pos.y()-tick);
            } else if (destination == "alice") {
                text_pos.setX(text_pos.x()-tick);
            } else if (destination == "bob") {
                text_pos.setX(text_pos.x()+tick);
            }
        }

        text_item->setPlainText(text);
        text_item->setPos(text_pos);

    }
}

void Server::loadSettings()
{
    if (QFile(m_settingsFile).exists()) {
        QSettings settings(m_settingsFile, QSettings::IniFormat);

        settings.beginGroup("alice");
            alice_port = settings.value("port", 55551).toInt();
        settings.endGroup();

        settings.beginGroup("bob");
            bob_port   = settings.value("port", 55552).toInt();
        settings.endGroup();

        settings.beginGroup("eve");
            eve_port   = settings.value("port", 55553).toInt();
        settings.endGroup();

        settings.beginGroup("server");
            sleep = settings.value("sleep", 200).toInt();
            ui->speedSlider->setValue(ui->speedSlider->maximum()-sleep);

            noise  = settings.value("noise",  false).toBool();
            ui->noiseCheck->setChecked(noise);
            ui->noiseGroup->setEnabled(noise);

            noise_level = settings.value("noise_level", 5).toInt();
            ui->noiseLevel->setValue(noise_level);


            multi  = settings.value("multi",  false).toBool();

            ui->multiCheck->setChecked(multi);
            ui->multiGroup->setEnabled(multi);

            photon_count = settings.value("photon_count", 2).toInt();
            ui->photonCount->setValue(photon_count);

            ui->rejectCheck->setChecked(reject);
            ui->rejectGroup->setEnabled(reject);



            reject = settings.value("reject", false).toBool();

            photon = settings.value("photon", true).toBool();

            ui->photonProbabilityRadio->setChecked(photon);
            ui->photonProbability->setEnabled(photon);

            ui->channelLengthRadio->setChecked(!photon);
            ui->channelLength->setEnabled(!photon);

            photon_probatility = settings.value("photon_probatility", 100).toInt();
            ui->photonProbability->setValue(photon_probatility);

            channel_length = settings.value("channel_length", 300).toInt();
            ui->channelLength->setValue(channel_length);

            spy = settings.value("spy", true).toBool();
            ui->eveSpy->setChecked(spy);
            this->on_eveSpy_toggled(spy);

        settings.endGroup();

        //"Настройки загружены из файла
    }
    return;
}

void Server::saveSettings()
{
    QSettings settings(m_settingsFile, QSettings::IniFormat);

    settings.beginGroup("server");
        settings.setValue("sleep",                           sleep);
        settings.setValue("spy",                               spy);

        settings.setValue("noise",                           noise);
        settings.setValue("multi",                           multi);
        settings.setValue("reject",                         reject);
        settings.setValue("photon",                         photon);

        settings.setValue("noise_level",               noise_level);
        settings.setValue("photon_count",             photon_count);
        settings.setValue("photon_probatility", photon_probatility);
        settings.setValue("channel_length",         channel_length);
    settings.endGroup();

    //Настройки сохранены в файл
}

void Tcp::stopServer() {
    disconnect(this, SIGNAL(  newConnection()),
               this, SLOT(slotNewConnection())
    );
    this->socket=NULL;
    this->close();
    //Сервер успешно остановлен!
}


void Tcp::slotNewConnection()
{
    if (this->label) this->label->setStyleSheet("color: rgb(0, 255, 0)");
    socket = this->nextPendingConnection();
    if (socket->state() == QTcpSocket::ConnectedState) {
        QString name;
        if (this->port == alice_port) name = "alice";
        if (this->port == bob_port  ) name = "bob";
        if (this->port == eve_port  ) name = "eve";
        //Подключен пользователь

        if (this->port == eve_port) {
            if (spy) {
                //Eve прослушивает канал
            } else {
                //Eve НЕ прослушивает канал
            }
        }
    }

    connect(socket, SIGNAL(disconnected()),
            this,   SLOT(    slotDelete())
    );
    connect(socket, SIGNAL(   readyRead()),
            this,   SLOT(slotReadClient())
    );
}

void Tcp::slotDelete()
{
    if (this->label) this->label->setStyleSheet("color: rgb(255, 0, 0)");
    disconnect(socket, SIGNAL(disconnected()),
               this,   SLOT(  slotDelete())
    );
    disconnect(socket, SIGNAL(readyRead()),
               this,   SLOT(slotReadClient())
    );

    socket->deleteLater();
    socket=NULL;
}


void Tcp::sendToClient(Packet& packet)
{
    if(!socket) {return;}

    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_5);

    out << quint64(0) << packet;
    out.device()->seek(0);
    out << quint64(arrBlock.size() - sizeof(quint64));

    socket->write(arrBlock);
}

void Tcp::slotReadClient()
{
    socket = (QTcpSocket*)sender();
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_5_5);
    for (;;) {
        if (!m_nNextBlockSize) {
            if (socket->bytesAvailable() < sizeof(quint64)) {
                break;
            }
            in >> m_nNextBlockSize;
        }

        if (socket->bytesAvailable() < m_nNextBlockSize) {
            break;
        }
        Packet packet;
        in >> packet;

        if (this->port == alice_port) {
            source = "alice";
            destination = spy ? "eve" : "bob";
        }
        if (this->port == bob_port  ) {
            source = "bob";
            destination = spy ? "eve" : "alice";
        }
        if (this->port == eve_port  ) {
            source = "eve";
            destination = (packet.from == "alice") ? "bob" : "alice";
        }

        QString new_photon="";


        if (((source == "alice") || (source == "bob")) && !packet.photon.contains('X') && !packet.photon.contains('+') && (packet.type & END) != END){

            if (multi) {
                int count = rnd_poisson(photon_count);

                for(int i=0; i<count; ++i){
                    new_photon += packet.photon;
                }
            }

            if (reject){

                qint32 probability;

                if (photon){
                    probability = 100-photon_probatility;
                } else {
                    probability = round(channel_length*5/300);
                }

                for(int i=0; i<new_photon.length(); ++i){
                    if (rnd() < probability) {
                        new_photon[i] = ' ';
                    }
                }
            }

            if (noise) new_photon=noisy(new_photon,noise_level);
        }

        if(new_photon.length() < 1) new_photon = packet.photon;

        packet.photon = new_photon;

        if (sleep >= 50  && !packet.photon.contains("+") && !packet.photon.contains("X") && (packet.type & END) != END) {
            text = packet.photon;
            text_pos  = QPoint(0,0);

            timer->setSingleShot(false);
            timer->setInterval(sleep/10);
            timer->start();
        }

        while(timer->isActive()){
            QTest::qWait(25);
        }

        if (destination == "alice" && main_server->alice) main_server->alice->sendToClient(packet);
        if (destination == "bob"   && main_server->bob  ) main_server->bob->sendToClient(  packet);
        if (destination == "eve"   && main_server->eve  ) main_server->eve->sendToClient(  packet);

        m_nNextBlockSize = 0;
    }
}


Server::~Server()
{
    delete ui;
}


void Server::closeEvent(QCloseEvent *event)
{
    alice->stopServer();
    bob->stopServer(  );
    eve->stopServer(  );
    saveSettings();
    event->accept();
}

void Server::on_exit_triggered()
{
    this->close();
}

void Server::on_loadFromFIle_triggered()
{
    loadSettings();
}

void Server::on_saveToFile_triggered()
{
    saveSettings();
}

void Server::on_startStopAction_triggered()
{
    alice->stopServer();
    bob->stopServer(  );
    eve->stopServer(  );

    ui->aliceLabel->setStyleSheet("color:red");
    ui->bobLabel->setStyleSheet("color:red");
    ui->eveLabel->setStyleSheet("color:red");

    alice = new Tcp(ui, alice_port);
    bob   = new Tcp(ui, bob_port  );
    eve   = new Tcp(ui, eve_port  );
}

void Server::on_eveSpy_toggled(bool checked)
{
    spy = ui->eveSpy->isChecked();


    QGraphicsScene* scene = ui->graphicsView->scene();

    qint32 width  = scene->width( );
    qint32 height = scene->height();

    qint32 vmiddle = height/2;
    qint32 hmiddle = width/2;



    if(text_item  ) scene->removeItem(text_item  );
    if(first_line ) scene->removeItem(first_line );
    if(second_line) scene->removeItem(second_line);
    if(third_line ) scene->removeItem(third_line );
    if(forth_line ) scene->removeItem(forth_line );

    scene->clear();

    delete text_item  ;
    delete first_line ;
    delete second_line;
    delete third_line ;
    delete forth_line ;

    text_item   = 0;
    first_line  = 0;
    second_line = 0;
    third_line  = 0;
    forth_line  = 0;



    text_item = new QGraphicsTextItem(QString(""));
    text_item->setPos(3,vmiddle+10);

    if (spy){
        first_line  = new QGraphicsLineItem(0,vmiddle,hmiddle-margin,vmiddle);
    } else {
        first_line  = new QGraphicsLineItem(0,vmiddle,width,vmiddle);
    }

    if (spy) second_line = new QGraphicsLineItem(hmiddle-margin, vmiddle, hmiddle-margin, height );
    if (spy) third_line  = new QGraphicsLineItem(hmiddle+margin, height,  hmiddle+margin, vmiddle);
    if (spy) forth_line  = new QGraphicsLineItem(hmiddle+margin, vmiddle, width,          vmiddle);

    if(text_item  ) scene->addItem(text_item  );
    if(first_line ) scene->addItem(first_line );
    if(second_line) scene->addItem(second_line);
    if(third_line ) scene->addItem(third_line );
    if(forth_line ) scene->addItem(forth_line );
}

void Server::on_speedSlider_valueChanged(int value)
{
    sleep = ui->speedSlider->maximum()-value;
}


void Server::on_photonCount_valueChanged(int arg1)
{
    photon_count = ui->photonCount->value();

}


void Server::on_noiseLevel_editingFinished()
{
    noise = ui->noiseLevel->value();
}

void Server::on_photonProbability_valueChanged(int arg1)
{
    photon_probatility = ui->photonProbability->value();
}



void Server::on_noiseCheck_toggled(bool checked)
{
    noise = checked;
    ui->noiseGroup->setEnabled(noise);
}

void Server::on_multiCheck_toggled(bool checked)
{
    multi = checked;
    ui->multiGroup->setEnabled(multi);
}

void Server::on_rejectCheck_toggled(bool checked)
{
    reject = checked;
    ui->rejectGroup->setEnabled(reject);
}

void Server::on_photonProbabilityRadio_toggled(bool checked)
{
    photon = checked;
    ui->photonProbability->setEnabled(photon);
    ui->channelLength->setEnabled(   !photon);
}

void Server::on_channelLengthRadio_toggled(bool checked)
{
    photon = !checked;
    ui->photonProbability->setEnabled(photon);
    ui->channelLength->setEnabled(   !photon);
}

void Server::on_channelLength_valueChanged(int arg1)
{
    channel_length = ui->channelLength->value();
}
