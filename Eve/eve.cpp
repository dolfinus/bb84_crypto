#include "eve.h"
#include "ui_eve.h"

char VERTICAL=1;
char HORIZONTAL=0;


int     attack;
int     photon_count;
QString polarization;
QString alice_polarization;
QString bob_polarization;
QString eve_bob_polarization;
QString buffer;
QString key;
QString seance_key;
QString sum_key;
QString bob_key;
//bool    logging;

Eve::Eve(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Eve)
{
    id="eve";
    transaction_no=0;
    ui->setupUi(this);
    m_nNextBlockSize=0;
    m_pTcpSocket = NULL;
    m_settingsFile = QApplication::applicationDirPath() + "/settings.ini";
    loadSettings();

    VERTICAL=(char)ui->verticalNormal->isChecked();
    HORIZONTAL=(char)ui->horizontalNormal->isChecked();

    if (!m_pTcpSocket) startClient(port);
}

void Eve::loadSettings()
{
    if (QFile(m_settingsFile).exists()) {
        QSettings settings(m_settingsFile, QSettings::IniFormat);
        QStringList groups = settings.childGroups();
        if (groups.contains(id)) {
            settings.beginGroup(id);
                port = settings.value("port", 55553).toInt();
                ui->verticalNormal->setChecked(settings.value("vertical",true).toBool());
                ui->verticalInverted->setChecked(!settings.value("vertical",true).toBool());

                ui->horizontalNormal->setChecked(settings.value("horizontal",true).toBool());
                ui->horizontalInverted->setChecked(!settings.value("horizontal",true).toBool());

                ui->idealButton->setChecked(settings.value("attack_ideal",false).toBool());
                ui->idealGroup->setEnabled( ui->idealButton->isChecked());
                ui->noisyGroup->setEnabled(!ui->idealButton->isChecked());

                photon_count=settings.value("ideal_percent",100).toInt();
                ui->photonCount->setValue(photon_count);
                ui->multiRejectButton->setChecked(settings.value("attack_multi_reject",true).toBool());
                ui->multiSendButton->setChecked(  settings.value("attack_multi_send", false).toBool());

                if (ui->idealButton->isChecked(       )) {
                    attack = ATTACK_IDEAL;
                } else if (ui->multiRejectButton->isChecked( )) {
                    attack = ATTACK_MULTI_REJECT;
                } else if (ui->multiSendButton->isChecked(   )) {
                    attack = ATTACK_MULTI_SEND;
                }

                //logging=settings.value("logging",true).toBool();
               // ui->logCheckBox->setChecked(logging);
            settings.endGroup();

            //log(QString("Настройки загружены из файла %1").arg(m_settingsFile));
        }
    }
    return;
}

void Eve::saveSettings()
{
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.beginGroup(id);
        settings.setValue("port",  port);

        settings.setValue("vertical",   VERTICAL);
        settings.setValue("horizontal", HORIZONTAL);

        settings.setValue("attack_ideal",(attack == ATTACK_IDEAL));
        settings.setValue("attack_multi_reject",(attack == ATTACK_MULTI_REJECT));
        settings.setValue("attack_multi_send",  (attack == ATTACK_MULTI_SEND  ));

        settings.setValue("ideal_percent",photon_count);

        //settings.setValue("logging",logging);
    settings.endGroup();
    //log(QString("Настройки сохранены в файл %1").arg(m_settingsFile));
}


void Eve::startClient(int port) {
    m_pTcpSocket = new QTcpSocket(this);
    m_pTcpSocket->connectToHost(QHostAddress::LocalHost, port);
    connect(m_pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(m_pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this,         SLOT(slotError(QAbstractSocket::SocketError))
    );
}

void Eve::stopClient() {
    if (m_pTcpSocket) {
        disconnect(m_pTcpSocket, SIGNAL(connected()), this,SLOT(slotConnected()));
        disconnect(m_pTcpSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
        disconnect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                   this,         SLOT(slotError(QAbstractSocket::SocketError))
        );
        m_pTcpSocket->abort();
        m_pTcpSocket->close();
        delete m_pTcpSocket;
        m_pTcpSocket = NULL;
        //log("Соединение закрыто!");
    } else {
        //log("Но ведь соединение никто не открывал...");
    }
}

void Eve::slotReadyRead()
{
    QDataStream in(m_pTcpSocket);
    in.setVersion(QDataStream::Qt_5_5);

    for (;;) {
        if (!m_nNextBlockSize) {
            if (m_pTcpSocket->bytesAvailable() < sizeof(quint64)) {
                break;
            }
            in >> m_nNextBlockSize;
        }

        if (m_pTcpSocket->bytesAvailable() < m_nNextBlockSize) {
            break;
        }
        Packet packet;
        in >> packet;

        QString photon;

        if (packet.from == "alice"){

        if (!packet.photon.contains('+') && !packet.photon.contains('X')){

            if (attack == ATTACK_MULTI_SEND) {

                if (packet.photon.length() > 1) {
                    photon = QString(packet.photon[0]);
                    packet.photon[0] = ' ';
                } else {
                    photon = " ";
                }


            } else if (attack == ATTACK_MULTI_REJECT) {

                if (packet.photon.length() > 0) {
                    photon = QString(packet.photon[0]);
                    packet.photon[0] = ' ';
                } else {
                    photon = " ";
                }

            } else if (attack == ATTACK_IDEAL){

                if (rnd() < photon_count) {
                    photon = packet.photon;
                    packet.photon = rand_photon();
                } else {
                    photon = " ";
                }
            }
        } else {
            photon = packet.photon;
        }

        if (photon.length() < 1) photon = " ";

        if ((packet.type & END) != END) {

            if ((packet.type & RETRY) != RETRY) {
                sum_key="";
                ui->sumKeyEdit->setText(sum_key);
            }

            buffer += photon;
            //log("Фотон:"+photon);

            if ((packet.type & ANSWER) != ANSWER) {
                polarization += rand_polarization();

                key  += bytearray_to_key(depolarize(QString(polarization[buffer.length()-1 % polarization.length()]),photon));

                //log("Поляризация:" + polarization[polarization.length()-1]);
               // log("Значение:"    + key[key.length()-1]);

                ui->vectorEdit->setText(polarization);
                ui->photonsEdit->setText(buffer);
                ui->keyEdit->setText(key);

                if ((packet.type & MESSAGE) == MESSAGE) {
                    QString text = bytearray_to_text(depolarize(polarization,buffer));
                    ui->messageEdit->setText(text);
                    //log("Сообщение:"+text);
                }

            } else {
                alice_polarization=buffer;
                QString alice_key = "";
                QString eve_alice_polarization="";
                QString common_polarization="";
                QString common_key="";

                for(int i=0; i<alice_polarization.length(); ++i){

                    if(QString(alice_polarization[i]) == QString(polarization[i])){
                        alice_key += key[i];
                        eve_alice_polarization += polarization[i];
                        if (QString(alice_polarization[i]) == QString(eve_bob_polarization[i])) {
                            common_polarization += polarization[i];
                            common_key += bob_key[i];
                        } else {
                            common_polarization += " ";
                            common_key += " ";
                        }
                    } else {
                        alice_key += " ";
                        eve_alice_polarization += " ";
                        common_polarization += " ";
                        common_key += " ";
                    }
                }

                seance_key = alice_key;

                ui->aliceVectorEdit->setText(alice_polarization);
                ui->eveAliceVectorEdit->setText(eve_alice_polarization);
                ui->aliceKeyEdit->setText(alice_key);
                ui->commonKeyEdit->setText(common_key);

                QString digits = alice_key;
                double percent = alice_key.length();

                digits.replace(" ","");
                percent = (digits.length()+0.0)/(alice_key.length()+0.0)*100.0;
                ui->alicePercentLabel->setText(QString::number(percent,'g',2)+"%");

                digits = common_key+"";
                percent = common_key.length();

                digits.replace(" ","");
                percent = (digits.length()+0.0)/(common_key.length()+0.0)*100.0;
                ui->commonPercentLabel->setText(QString::number(percent,'g',2)+"%");

            }

        } else {

          if ((packet.type & MESSAGE) == MESSAGE && (packet.type & ANSWER) == ANSWER) {

              if(sum_key.length() == 0) {
                  sum_key = seance_key;
              } else {
                  for(int i=0;i<seance_key.length(); ++i){
                      if (sum_key.contains(" ") && QString(sum_key[i]) == QString(" ") && QString(seance_key[i]) != QString(" ") && sum_key.length() >= i && sum_key.length() > 0){
                          sum_key[i]=seance_key[i];
                      } else if (sum_key.length() < i){
                          sum_key.append(seance_key[i]);
                      }
                  }
              }

              ui->sumKeyEdit->setText(sum_key);

              QString text=sum_key;
              text=bytearray_to_text(decode_hamming(key_to_bytearray(text.replace(" ", "0"))));
              ui->messageEdit->setText(text);
             // log("Сообщение:"+text);

              polarization="";
              key="";

          } else if ((packet.type & ANSWER) != ANSWER) {
          } else {
             alice_polarization="";
             polarization="";
             key="";
          }

         buffer="";
        }

        } else {

            if ((packet.type & ANSWER) == ANSWER && (packet.type & END) != END){

                buffer += packet.photon;
                bob_polarization=buffer;

                for(int i=0; i<bob_polarization.length(); ++i){

                    if(QString(bob_polarization[i]) == QString(polarization[i])){
                        bob_key += key[i];
                        eve_bob_polarization += polarization[i];
                    } else {
                        bob_key += " ";
                        eve_bob_polarization += " ";
                    }
                }

                ui->bobVectorEdit->setText(bob_polarization);
               // log("Поляризация Боба:"+alice_polarization);

            } else {
               eve_bob_polarization="";
               bob_polarization="";
               bob_key = "";
            }

            if ((packet.type & END) == END){
               buffer="";
            }

        }



        m_nNextBlockSize = 0;

        slotSendToServer(packet);
    }
}

void Eve::slotError(QAbstractSocket::SocketError err)
{
    QString strError =
        "Ошибка: " + (err == QAbstractSocket::HostNotFoundError ?
                     "Сервер не найден!" :
                     err == QAbstractSocket::RemoteHostClosedError ?
                     "Сервер закрыл соединение!" :
                     err == QAbstractSocket::ConnectionRefusedError ?
                     "Нет доступа к серверу!" :
                     QString(m_pTcpSocket->errorString())
                    );
   // log(strError);
}

void Eve::slotSendToServer(Packet& packet)
{
    QByteArray  arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_5);

    out << quint64(0) << packet;
    out.device()->seek(0);
    out << quint64(arrBlock.size() - sizeof(quint64));

    m_pTcpSocket->write(arrBlock);
}

void Eve::slotConnected()
{
   // log("Подключено к серверу");
}

Eve::~Eve()
{
    delete ui;
}


void Eve::closeEvent(QCloseEvent *event)
{
    stopClient();
    saveSettings();
    event->accept();
}


void Eve::on_startStopAction_triggered()
{
    stopClient();
    startClient(port);
}

void Eve::on_horizontalNormal_clicked()
{
   HORIZONTAL=(char)ui->horizontalNormal->isChecked();
}

void Eve::on_horizontalInverted_clicked()
{
    HORIZONTAL=(char)ui->horizontalNormal->isChecked();
}

void Eve::on_verticalNormal_clicked()
{
    VERTICAL=(char)ui->verticalNormal->isChecked();
}

void Eve::on_verticalInverted_clicked()
{
    VERTICAL=(char)ui->verticalNormal->isChecked();
}


void Eve::on_idealButton_toggled(bool checked)
{
    attack = ATTACK_IDEAL;

    ui->idealGroup->setEnabled(true);
    ui->noisyGroup->setEnabled(false);
}

void Eve::on_multiPhotonButton_toggled(bool checked)
{
    ui->idealGroup->setEnabled(false);
    ui->noisyGroup->setEnabled(true);

    if (ui->multiRejectButton->isChecked()){
        attack = ATTACK_MULTI_REJECT;
    } else {
        attack = ATTACK_MULTI_SEND;
    }
}


void Eve::on_multiSendButton_toggled(bool checked)
{
    attack = ATTACK_MULTI_SEND;
}

void Eve::on_multiRejectButton_toggled(bool checked)
{
    attack = ATTACK_MULTI_REJECT;
}

void Eve::on_photonCount_valueChanged(int arg1)
{
    photon_count = ui->photonCount->value();
}
