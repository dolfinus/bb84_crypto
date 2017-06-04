#include "server.h"
#include "ui_server.h"

char VERTICAL=1;
char HORIZONTAL=0;

Server *main_server;
qint32  alice_port;
qint32  bob_port;
qint32  eve_port;

QTimer *timer;
qint32  margin;
qint32  tick;

bool    anim;
bool    no_alice;
bool    no_bob;

QPoint  photon_pos;

QGraphicsPixmapItem *alice_pc;
QGraphicsPixmapItem *bob_pc;
QGraphicsPixmapItem *eve_pc;

QString alice_photon;
QString bob_photon;
QString eve_photon;

QString ideal_photon;
QString real_photon;

QGraphicsTextItem *alice_photon_item;
QGraphicsTextItem *bob_photon_item;
QGraphicsTextItem *eve_photon_item;
QGraphicsTextItem *real_photon_item;

QGraphicsLineItem *open_channel_top_line;
QGraphicsLineItem *open_channel_bottom_line;
QGraphicsLineItem *alice_bob_direct_line;
QGraphicsLineItem *alice_to_center_line;
QGraphicsLineItem *center_to_eve_line;
QGraphicsLineItem *eve_to_center_line;
QGraphicsLineItem *center_to_bob_line;

int     sleep;
bool    spy;
qint32  source;
qint32  destination;

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
    tick=10;


    QGraphicsScene *scene = new QGraphicsScene(0,0,ui->graphicsView->width()-5,ui->graphicsView->height()-5,ui->graphicsView);
    ui->graphicsView->setScene(scene);

    m_settingsFile = QApplication::applicationDirPath() + "/settings.ini";
    loadSettings();

    alice = new Tcp(ui, alice_port);
    bob   = new Tcp(ui, bob_port  );
    eve   = new Tcp(ui, eve_port  );


    timer = new QTimer(this);
    timer->setSingleShot(false);

    connect(timer,SIGNAL(timeout()), this, SLOT(draw_items()));

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
            init_draw_surface();

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
    slotDelete();
    disconnect(this, SIGNAL(  newConnection()),
               this, SLOT(slotNewConnection())
    );
    this->socket=NULL;
    this->close();
    //Сервер успешно остановлен!
}


void Tcp::slotNewConnection()
{
    socket = this->nextPendingConnection();
    if (socket->state() == QTcpSocket::ConnectedState) {
        this->state = true;
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
    this->state = false;

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

        while( ((this->port == alice_port && no_alice) ||\
               (this->port == bob_port   && no_bob  )) &&\
               (packet.type & ANSWER) != ANSWER
             ){
            QTest::qWait(sleep*2);
        }

        if (this->port == alice_port) {
            source = ALICE;
            destination = spy ? EVE : BOB;
        }
        if (this->port == bob_port  ) {
            source = BOB;
            destination = spy ? EVE : ALICE;
        }
        if (this->port == eve_port  ) {
            source = EVE;
            destination = (packet.from == ALICE) ? BOB : ALICE;
        }

        QString raw_photon = packet.photon;
        real_photon="";

        if (((source == ALICE) || (source == BOB)) && !raw_photon.contains('X') && !raw_photon.contains('+') && (packet.type & END) != END){

            if (multi) {
                int count = rnd_poisson(photon_count);

                for(int i=0; i<count; ++i){
                    real_photon += raw_photon;
                }
            }

            if (reject){

                qint32 probability;

                if (photon){
                    probability = 100-photon_probatility;
                } else {
                    probability = round(channel_length*5/300);
                }

                for(int i=0; i<real_photon.length(); ++i){
                    if (rnd() < probability) {
                        real_photon[i] = ' ';
                    }
                }
            }

            if (noise) real_photon=noisy(real_photon,noise_level);
        }

        if(real_photon.length() < 1) real_photon = raw_photon;



        if (source == ALICE) alice_photon = raw_photon;
        if (source == BOB  ) bob_photon   = raw_photon;
        if (source == EVE)   eve_photon   = raw_photon;
        if (source != EVE)   ideal_photon = raw_photon;



        if (destination == ALICE) alice_photon = real_photon;
        if (destination == BOB  ) bob_photon   = real_photon;
        if (destination == EVE  ) eve_photon   = real_photon;


        packet.photon = real_photon;
        if (sleep >= 50  && !raw_photon.contains("+") && !raw_photon.contains("X") && (packet.type & END) != END) {
            photon_pos  = QPoint(0,0);

            anim=true;

            timer->setInterval(sleep/10);
            timer->start();
        }

        while(anim){
            QTest::qWait(25);
        }

        if (destination == ALICE && main_server->alice) main_server->alice->sendToClient( packet );
        if (destination == BOB   && main_server->bob  ) main_server->bob->sendToClient(   packet );
        if (destination == EVE   && main_server->eve  ) {
            main_server->eve->sendToClient(   packet );
            if (source == ALICE) no_alice = true;
            if (source == BOB  ) no_bob   = true;
        }

        if (source == EVE){
            no_alice=false;
            no_bob = false;
        }

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

    alice = new Tcp(ui, alice_port);
    bob   = new Tcp(ui, bob_port  );
    eve   = new Tcp(ui, eve_port  );

    init_draw_surface();
}

void Server::init_draw_surface(){
    QGraphicsScene* scene = ui->graphicsView->scene();

    qint32 width  = scene->width( );
    qint32 height = scene->height();

    qint32 vmiddle = height/2;
    qint32 hmiddle = width/2;

    QFont  font;
    QPen   pen;
    font.setPointSize(16);

    pen.setWidthF(0.5);
    pen.setColor(QApplication::palette().text().color());


    if(open_channel_top_line   ) scene->removeItem(open_channel_top_line   );
    if(open_channel_bottom_line) scene->removeItem(open_channel_bottom_line);
    if(alice_bob_direct_line   ) scene->removeItem(alice_bob_direct_line   );

    if(alice_to_center_line    ) scene->removeItem(alice_to_center_line );
    if(center_to_eve_line      ) scene->removeItem(center_to_eve_line   );
    if(eve_to_center_line      ) scene->removeItem(eve_to_center_line   );
    if(center_to_bob_line      ) scene->removeItem(center_to_bob_line   );

    if(real_photon_item        ) scene->removeItem(real_photon_item     );
    if(alice_photon_item       ) scene->removeItem(alice_photon_item    );
    if(bob_photon_item         ) scene->removeItem(bob_photon_item      );
    if(eve_photon_item         ) scene->removeItem(eve_photon_item);

    scene->clear();

    delete open_channel_top_line;
    delete open_channel_bottom_line;
    delete alice_bob_direct_line;

    delete alice_to_center_line;
    delete center_to_eve_line;
    delete eve_to_center_line;
    delete center_to_bob_line;

    delete real_photon_item;
    delete alice_photon_item;
    delete bob_photon_item;
    delete eve_photon_item;

    open_channel_top_line    = 0;
    open_channel_bottom_line = 0;
    alice_bob_direct_line    = 0;

    alice_to_center_line     = 0;
    center_to_eve_line       = 0;
    eve_to_center_line       = 0;
    center_to_bob_line       = 0;

    real_photon_item         = 0;
    alice_photon_item        = 0;
    bob_photon_item          = 0;
    eve_photon_item          = 0;



    real_photon_item  = new QGraphicsTextItem(QString(""));
    alice_photon_item = new QGraphicsTextItem(QString(""));
    bob_photon_item   = new QGraphicsTextItem(QString(""));
    eve_photon_item   = new QGraphicsTextItem(QString(""));


    real_photon_item->setPos(               0,                0);
    alice_photon_item->setPos(         margin,  vmiddle+margin);
    bob_photon_item->setPos(   width-margin*2,  vmiddle+margin);
    eve_photon_item->setPos( hmiddle-margin/2, height-margin*2);

    real_photon_item->setFont(  font );
    alice_photon_item->setFont( font );
    bob_photon_item->setFont(   font );
    eve_photon_item->setFont(   font );






    alice_bob_direct_line       = new QGraphicsLineItem(0,vmiddle-margin*2,width,vmiddle-margin*2);

    if (spy){
        alice_to_center_line    = new QGraphicsLineItem(0,vmiddle,hmiddle-margin,vmiddle);
    } else {
        alice_to_center_line    = new QGraphicsLineItem(0,vmiddle,width,vmiddle);
    }

    if (spy) center_to_eve_line = new QGraphicsLineItem(hmiddle-margin, vmiddle, hmiddle-margin, height );
    if (spy) eve_to_center_line = new QGraphicsLineItem(hmiddle+margin, height,  hmiddle+margin, vmiddle);
    if (spy) center_to_bob_line = new QGraphicsLineItem(hmiddle+margin, vmiddle, width,          vmiddle);


    if(open_channel_top_line   ) open_channel_top_line->setPen(   pen);
    if(open_channel_bottom_line) open_channel_bottom_line->setPen(pen);
    if(alice_bob_direct_line   ) alice_bob_direct_line->setPen   (pen);
    if(alice_to_center_line    ) alice_to_center_line->setPen(    pen);
    if(center_to_eve_line      ) center_to_eve_line->setPen(      pen);
    if(eve_to_center_line      ) eve_to_center_line->setPen(      pen);
    if(center_to_bob_line      ) center_to_bob_line->setPen(      pen);





    if(open_channel_top_line   ) scene->addItem(open_channel_top_line   );
    if(open_channel_bottom_line) scene->addItem(open_channel_bottom_line);
    if(alice_bob_direct_line   ) scene->addItem(alice_bob_direct_line   );

    if(alice_to_center_line    ) scene->addItem(alice_to_center_line );
    if(center_to_eve_line      ) scene->addItem(center_to_eve_line   );
    if(eve_to_center_line      ) scene->addItem(eve_to_center_line   );
    if(center_to_bob_line      ) scene->addItem(center_to_bob_line   );

    if(real_photon_item        ) scene->addItem(real_photon_item     );
    if(alice_photon_item       ) scene->addItem(alice_photon_item    );
    if(bob_photon_item         ) scene->addItem(bob_photon_item      );
    if(eve_photon_item         ) scene->addItem(eve_photon_item);
}

void Server::draw_items(){
    qint32 width  = ui->graphicsView->scene()->width( );
    qint32 height = ui->graphicsView->scene()->height();

    qint32 vmiddle = height/2;
    qint32 hmiddle = width/2;

    if (real_photon_item){

        if (photon_pos.x() < 0 || photon_pos.x() > width || photon_pos.y() > height) {
            anim = false;
            timer->stop();
            return;
        }

        if(alice_photon_item){
            if (source == ALICE) {
                alice_photon_item->setHtml(format_HTML(alice_photon,ideal_photon));
                if (destination == EVE){
                    if(photon_pos.y() > height-margin*2){
                        eve_photon_item->setHtml(format_HTML(eve_photon,ideal_photon));
                    } else{
                        eve_photon_item->setPlainText("");
                    }
                }
            }

            if (destination == ALICE){
                if(photon_pos.x() > margin){
                    alice_photon_item->setHtml("");
                } else {
                    eve_photon_item->setPlainText("");
                    alice_photon_item->setHtml(format_HTML(alice_photon,ideal_photon));
                }
            }

        }

        if (bob_photon_item){
            if (source == BOB) {
                bob_photon_item->setHtml(format_HTML(bob_photon,ideal_photon));
            }

            if (destination == BOB){
                if(photon_pos.x() < width-margin){
                    bob_photon_item->setPlainText("");
                } else {
                    bob_photon_item->setHtml(format_HTML(bob_photon,ideal_photon));
                }
            }

        }

        if (source == EVE) {
            if (destination == ALICE && eve_photon_item) eve_photon_item->setHtml(format_HTML(eve_photon,ideal_photon));
        }

        real_photon_item->setHtml(format_HTML(real_photon,ideal_photon));




        if (photon_pos == QPoint(0,0)){
            if(source == ALICE) photon_pos = QPoint(        margin/2, vmiddle-margin*2);
            if(source == BOB  ) photon_pos = QPoint(  width-margin/2, vmiddle-margin*2);
            if(source == EVE  ) photon_pos = QPoint(hmiddle-margin/2, height-margin/2);
        }

        if(source == ALICE) {
            if (destination == BOB || ( destination == EVE && photon_pos.x() < hmiddle-margin/2)){
                photon_pos.setX(photon_pos.x()+tick);
            } else {
                photon_pos.setY(photon_pos.y()+tick);
            }
        } else if(source == BOB) {
            if (destination == ALICE || ( destination == EVE && photon_pos.x() > hmiddle+margin/2)){
                photon_pos.setX(photon_pos.x()-tick);
            } else {
                photon_pos.setY(photon_pos.y()+tick);
            }
        } else if(source == EVE){
            if (photon_pos.y() > vmiddle-margin*2){
                photon_pos.setY(photon_pos.y()-tick);
            } else if (destination == ALICE) {
                photon_pos.setX(photon_pos.x()-tick);
            } else if (destination == BOB) {
                photon_pos.setX(photon_pos.x()+tick);
            }
        }

        real_photon_item->setPos(      photon_pos);

    }
}

void Server::on_eveSpy_toggled(bool checked)
{
    spy = ui->eveSpy->isChecked();

    init_draw_surface();
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
