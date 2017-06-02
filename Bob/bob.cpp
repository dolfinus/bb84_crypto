#include "bob.h"
#include "ui_bob.h"

char VERTICAL=1;
char HORIZONTAL=0;
bool    detected;
QString polarization;
QString alice_polarization;
QString buffer;
QString key;
QString seance_key;
QString sum_key;
//bool    logging;

Bob::Bob(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Bob)
{
    id="bob";
    detected = false;
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

void Bob::loadSettings()
{
    if (QFile(m_settingsFile).exists()) {
        QSettings settings(m_settingsFile, QSettings::IniFormat);
        QStringList groups = settings.childGroups();
        if (groups.contains(id)) {
            settings.beginGroup(id);
                port = settings.value("port", 55552).toInt();
                ui->verticalNormal->setChecked(settings.value("vertical",true).toBool());
                ui->verticalInverted->setChecked(!settings.value("vertical",true).toBool());

                ui->horizontalNormal->setChecked(settings.value("horizontal",true).toBool());
                ui->horizontalInverted->setChecked(!settings.value("horizontal",true).toBool());

                //logging=settings.value("logging",true).toBool();
                //ui->logCheckBox->setChecked(logging);
            settings.endGroup();

           // log(QString("Настройки загружены из файла %1").arg(m_settingsFile));
        }
    }
    return;
}

void Bob::saveSettings()
{
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.beginGroup(id);
        settings.setValue("port",  port);

        settings.setValue("vertical",  ui->verticalNormal->isChecked());
        settings.setValue("horizontal",ui->horizontalNormal->isChecked());

        //settings.setValue("logging",logging);
    settings.endGroup();
    //log(QString("Настройки сохранены в файл %1").arg(m_settingsFile));
}


void Bob::startClient(int port) {
    m_pTcpSocket = new QTcpSocket(this);
    m_pTcpSocket->connectToHost(QHostAddress::LocalHost, port);
    connect(m_pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(m_pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this,         SLOT(slotError(QAbstractSocket::SocketError))
    );
}

void Bob::stopClient() {
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
       // log("Соединение закрыто!");
    } else {
       // log("Но ведь соединение никто не открывал...");
    }
}

void Bob::slotReadyRead()
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

        packet.photon=denoisy(packet.photon);

        if (((packet.type & END) == END)) {
            if (((packet.type & ANSWER) != ANSWER) && ((packet.type & RETRY) != RETRY) ) {
                detected = false;
            }
        }

        if ((packet.type & END) != END) {

            if ((packet.type & RETRY) != RETRY) {
                sum_key="";
                ui->sumKeyEdit->setText(sum_key);
            }

            if ((packet.type & MESSAGE) == MESSAGE) {
               // log(packet.dateTime.toString() + " " + "Получено сообщение от от Алисы");
            } else {
                if ((packet.type & ANSWER) == ANSWER) {
                  //  log(packet.dateTime.toString() + " " + "Получен ответ на обмен ключами от от Алисы");
                } else {
                   // log(packet.dateTime.toString() + " " + "Получен запрос на обмен ключами от от Алисы");
                }
            }

            buffer += packet.photon;
            //log("Фотон:"+packet.photon);

            if ((packet.type & ANSWER) != ANSWER) {
                polarization += rand_polarization();

                key  += bytearray_to_key(depolarize(QString(polarization[buffer.length()-1 % polarization.length()]),packet.photon));

                //log("Поляризация:" + polarization[polarization.length()-1]);
               // log("Значение:"    + key[key.length()-1]);

                ui->vectorEdit->setText(polarization);
                ui->photonsEdit->setText(buffer);
                ui->keyEdit->setText(key);

                if ((packet.type & MESSAGE) == MESSAGE) {
                    QString text = bytearray_to_text(depolarize(polarization,buffer));
                    ui->messageEdit->setText(text);
                  //  log("Сообщение:"+text);
                }

            } else {
                alice_polarization=buffer;
                seance_key = "";
                QString common_polarization="";

                for(int i=0; i<alice_polarization.length(); ++i){

                    if(QString(alice_polarization[i]) == QString(polarization[i])){
                        seance_key += key[i];
                        common_polarization += polarization[i];
                    } else {
                        seance_key += " ";
                        common_polarization += " ";
                    }
                }

                ui->aliceVectorEdit->setText(alice_polarization);
               // log("Поляризация Алисы:"+alice_polarization);


                ui->commonVectorEdit->setText(common_polarization);
               // log("Общая поляризация:"+alice_polarization);

                ui->sharedKeyEdit->setText(seance_key);
               // log("Общий ключ:"+seance_key);


                QString digits = seance_key;
                double shared_percent = seance_key.length();

                digits.replace(" ","");
                shared_percent = (digits.length()+0.0)/(seance_key.length()+0.0)*100.0;
                ui->sharedPercentLabel->setText(QString::number(shared_percent,'g',2)+"%");


                if (shared_percent < 25){
                    ui->eveDetectLabel->setStyleSheet("color:\"red\"");
                    ui->eveDetectLabel->setText("обнаружена!");
                } else {
                    ui->eveDetectLabel->setStyleSheet("color:\"green\"");
                    ui->eveDetectLabel->setText("не обнаружена");
                }

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


              QString digits = seance_key;
              double shared_percent = seance_key.length();

              digits.replace(" ","");
              shared_percent = (digits.length()+0.0)/(seance_key.length()+0.0)*100.0;
              ui->sharedPercentLabel->setText(QString::number(shared_percent,'g',2)+"%");

              if (shared_percent < 25){
                  ui->eveDetectLabel->setStyleSheet("color:\"red\"");
                  ui->eveDetectLabel->setText("обнаружена!");
                  detected = true;
              } else {
                  ui->eveDetectLabel->setStyleSheet("color:\"green\"");
                  ui->eveDetectLabel->setText("не обнаружена");
              }

              polarization="";
              key="";

              if (sum_key.contains(" ") && !detected) {
                  Packet ans;
                  ans.photon=rand_photon();
                  ans.type=MESSAGE|RETRY;
                  slotSendToServer(ans);
              }

          } else if ((packet.type & ANSWER) != ANSWER) {
              Packet ans;

              if ((packet.type & KEY) == KEY) {
              //    log(QDateTime::currentDateTime().toString() + " " + "Отправлен запрос на обмен ключом с Алисой");
                  ans.type=KEY|ANSWER;
              } else {
               //   log(QDateTime::currentDateTime().toString() + " " + "Отправлен запрос на повторную отправку сообшения");
                  ans.type=MESSAGE|ANSWER;

                  if (sum_key.length() > 0) ans.type = ans.type | RETRY;
              }
              ans.photon=ui->vectorEdit->text();
              if (!detected){
                slotSendToServer(ans);
              }
         } else {
            alice_polarization="";
            polarization="";
            key="";

         }

         buffer="";
        }
        m_nNextBlockSize = 0;
    }
}

void Bob::slotError(QAbstractSocket::SocketError err)
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
    //log(strError);
}

void Bob::slotSendToServer(Packet& packet)
{
    for(int i=0; i<= packet.photon.length(); ++i){

        QByteArray  arrBlock;
        QDataStream out(&arrBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_5);

        Packet pckt;
        pckt.type=packet.type;
        pckt.photon=QString(packet.photon[i]);
        pckt.from=id;
        pckt.transaction_no=transaction_no;
        pckt.dateTime=QDateTime::currentDateTime();
        ++transaction_no;

        if (i == packet.photon.length()) {
            pckt.type = packet.type | END;
            pckt.photon = rand_photon();
        }

        out << quint64(0) << pckt;
        out.device()->seek(0);
        out << quint64(arrBlock.size() - sizeof(quint64));

        m_pTcpSocket->write(arrBlock);
    }
}

void Bob::slotConnected()
{
   // log("Подключено к серверу");
}

Bob::~Bob()
{
    delete ui;
}


void Bob::closeEvent(QCloseEvent *event)
{
    stopClient();
    saveSettings();
    event->accept();
}


void Bob::on_startStopAction_triggered()
{
    stopClient();
    startClient(port);
}

void Bob::on_horizontalNormal_clicked()
{
   HORIZONTAL=(char)ui->horizontalNormal->isChecked();
}

void Bob::on_horizontalInverted_clicked()
{
    HORIZONTAL=(char)ui->horizontalNormal->isChecked();
}

void Bob::on_verticalNormal_clicked()
{
    VERTICAL=(char)ui->verticalNormal->isChecked();
}

void Bob::on_verticalInverted_clicked()
{
    VERTICAL=(char)ui->verticalNormal->isChecked();
}

