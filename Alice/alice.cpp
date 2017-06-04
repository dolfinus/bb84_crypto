#include "alice.h"
#include "ui_alice.h"

char VERTICAL=1;
char HORIZONTAL=0;
QString buffer;
//bool logging;

Alice::Alice(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Alice)
{
    id=ALICE;
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

void Alice::loadSettings()
{
    if (QFile(m_settingsFile).exists()) {
        QSettings settings(m_settingsFile, QSettings::IniFormat);
        QStringList groups = settings.childGroups();
        if (groups.contains("alice")) {
            settings.beginGroup("alice");
                port = settings.value("port", 55551).toInt();

                ui->vectorRadio->setChecked(settings.value("is_vector",true).toBool());
                ui->lengthRadio->setChecked(!settings.value("is_vector",true).toBool());

                ui->vectorEdit->setText(settings.value("vector","XXX+++XXX").toString());
                ui->keyEdit->setText(settings.value("key","010011010").toString());
                ui->lengthEdit->setText(settings.value("length",9).toString());

                bool len = ui->lengthRadio->isChecked();
                ui->generateButton->setEnabled(len);
                ui->lengthEdit->setEnabled(len);
                ui->vectorEdit->setEnabled(!len);
                ui->keyEdit->setEnabled(!len);


                ui->messageEdit->setText(settings.value("message","Сообщение").toString());

                ui->verticalNormal->setChecked(settings.value("vertical",true).toBool());
                ui->verticalInverted->setChecked(!settings.value("vertical",true).toBool());

                ui->horizontalNormal->setChecked(settings.value("horizontal",true).toBool());
                ui->horizontalInverted->setChecked(!settings.value("horizontal",true).toBool());

                //logging=settings.value("logging",true).toBool();
               // ui->logCheckBox->setChecked(logging);
            settings.endGroup();

           // log(QString("Настройки загружены из файла %1").arg(m_settingsFile));
        }
    }
    return;
}

void Alice::saveSettings()
{
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.beginGroup("alice");
        settings.setValue("port",  port);
        settings.setValue("is_vector",ui->vectorRadio->isChecked());
        settings.setValue("vector", ui->vectorEdit->text());
        settings.setValue("length", ui->lengthEdit->text());
        settings.setValue("key", ui->keyEdit->text());
        settings.setValue("message", ui->messageEdit->text());

        settings.setValue("vertical",  ui->verticalNormal->isChecked());
        settings.setValue("horizontal",ui->horizontalNormal->isChecked());

       // settings.setValue("logging",logging);
    settings.endGroup();
    //log(QString("Настройки сохранены в файл %1").arg(m_settingsFile));
}


void Alice::startClient(int port) {
    m_pTcpSocket = new QTcpSocket(this);
    m_pTcpSocket->connectToHost(QHostAddress::LocalHost, port);
    connect(m_pTcpSocket, SIGNAL(connected()), SLOT(slotConnected()));
    connect(m_pTcpSocket, SIGNAL(readyRead()), SLOT(slotReadyRead()));
    connect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this,         SLOT(slotError(QAbstractSocket::SocketError))
    );
}

void Alice::stopClient() {
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
        //log("Но ведь соединение никто не открывал...");
    }
}


void Alice::slotReadyRead()
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

        QString photon=denoisy(packet.photon);

        if ((packet.type & END) != END) {
            buffer += photon;
        } else {
            if ((packet.type & MESSAGE) == MESSAGE) {
              //  log(packet.dateTime.toString() + " " + "Получено сообщение от Боба");
            } else {
              //  log(packet.dateTime.toString() + " " + "Получен запрос на обмен ключами от Боба");
            }

           // log("Фотоны:"+buffer);

            if ((packet.type & RETRY) == RETRY && (packet.type & ANSWER) != ANSWER) {
                buffer=QString("");
                on_messageButton_clicked(true);
            } else {
                QString polarization = ui->vectorEdit->text();
                QString shares_vector;
                QString shared_key;

                ui->bobVectorEdit->setText(buffer);
              //  log("Поляризация Боба:" +buffer);

                for(int i=0; i<buffer.length(); ++i){
                    if (polarization[i] == buffer[i]){
                        shares_vector += polarization[i];
                        shared_key += ui->keyEdit->text()[i];
                    } else {
                        shares_vector += " ";
                        shared_key += " ";
                    }
                }

               // log("Совпавшие элементы:" +shares_vector);

                ui->sharedKeyEdit->setText(shared_key);
                ui->sharedVectorEdit->setText(shares_vector);
              //  log("Общий ключ:" +shared_key);

                Packet ans;

             //   log("Отправка ответа Бобу");

                if ((packet.type & MESSAGE) == MESSAGE) {
                    ans.type=MESSAGE|ANSWER;
                } else {
                    ans.type=KEY|ANSWER;
                }

                if ((packet.type & RETRY) == RETRY) ans.type = ans.type | RETRY;

                ans.photon=polarization;
                slotSendToServer(ans);
            }

            buffer="";
        }

        m_nNextBlockSize = 0;
    }
}

void Alice::slotError(QAbstractSocket::SocketError err)
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
 //   log(strError);
}

void Alice::slotSendToServer(Packet& packet)
{
    for(int i=0; i<= packet.photon.length(); ++i){

        QByteArray  arrBlock;
        QDataStream out(&arrBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_5);

        Packet pckt;

        if (i == packet.photon.length()) {
            pckt.type = packet.type | END;
            pckt.photon = rand_photon();
        } else{
            pckt.type=packet.type;
            pckt.photon=QString(packet.photon[i]);
        }

        pckt.from=id;
        pckt.transaction_no=transaction_no;
        pckt.dateTime=QDateTime::currentDateTime();

        out << quint64(0) << pckt;
        out.device()->seek(0);
        out << quint64(arrBlock.size() - sizeof(quint64));

        m_pTcpSocket->write(arrBlock);
        ++transaction_no;
    }
}

void Alice::slotConnected()
{
  //  log("Подключено к серверу");
}

Alice::~Alice()
{
    delete ui;
}


void Alice::closeEvent(QCloseEvent *event)
{
    stopClient();
    saveSettings();
    event->accept();
}

void Alice::on_generateButton_clicked()
{
    QString vector;
    QString key;
    int length;
    qsrand(static_cast<quint64>(QTime::currentTime().msecsSinceStartOfDay()));

    if (!ui->vectorRadio->isChecked()) {
        length = ui->lengthEdit->text().toInt();
        for(int i=0;i<length;++i){
            vector += rand_polarization();
            key += QString::number(rnd(0,1)) ;
        }
    } else {
        key=ui->keyEdit->text();
        vector=ui->vectorEdit->text();
        length = vector.length();
    }

    ui->vectorEdit->setText(vector);
    ui->keyEdit->setText(key);
}

void Alice::on_exchangeButton_clicked()
{
    Packet packet;
    QString polarization=ui->vectorEdit->text();

    QString key = ui->keyEdit->text();
    QByteArray message = key_to_bytearray(key);
    QString photons=polarize(polarization, message);

    ui->photonsEdit->setText(photons);

   // log(QDateTime::currentDateTime().toString() + " " + "Отправлен запрос на обмен ключом с Бобом");
   // log("Ключ:"+key);
  //  log("Фотоны:"+photons);

    packet.type=KEY;
    packet.photon=photons;
    slotSendToServer(packet);
}

void Alice::on_messageButton_clicked(bool retry=false)
{
    Packet packet;

    QString text        = ui->messageEdit->text();
    QString key         = bytearray_to_key(code_hamming(text_to_bytearray(text)));
    QString polarization= "";
    for(int i=0; i<key.length(); ++i){
        polarization += rand_polarization();
    }
    ui->vectorEdit->setText(polarization);
    ui->keyEdit->setText(  key          );

    QByteArray msg      = key_to_bytearray(key);
    QString    photons  = polarize(polarization, msg);

    ui->photonsEdit->setText(photons);

   // log(QDateTime::currentDateTime().toString() + " " + "Отправлено сообщение Бобу");
  //  log("Текст:"+key);
  //  log("Фотоны:"+photons);

    packet.type=MESSAGE;

    if(retry){
        packet.type = packet.type|RETRY;
    }

    packet.photon=photons;
    slotSendToServer(packet);
}

void Alice::on_startStopAction_triggered()
{
    stopClient();
    startClient(port);
}

void Alice::on_horizontalNormal_clicked()
{
   HORIZONTAL=(char)ui->horizontalNormal->isChecked();
}

void Alice::on_horizontalInverted_clicked()
{
    HORIZONTAL=(char)ui->horizontalNormal->isChecked();
}

void Alice::on_verticalNormal_clicked()
{
    VERTICAL=(char)ui->verticalNormal->isChecked();
}

void Alice::on_verticalInverted_clicked()
{
    VERTICAL=(char)ui->verticalNormal->isChecked();
}


void Alice::on_vectorRadio_toggled(bool checked)
{
    ui->generateButton->setEnabled(false);
    ui->lengthEdit->setEnabled(false);

    ui->vectorEdit->setEnabled(true);
    ui->keyEdit->setEnabled(true);
}

void Alice::on_lengthRadio_toggled(bool checked)
{
    ui->generateButton->setEnabled(true);
    ui->lengthEdit->setEnabled(true);

    ui->vectorEdit->setEnabled(false);
    ui->keyEdit->setEnabled(false);
}
