#ifndef PACKET_H
#define PACKET_H
#include <QtCore/qdatastream.h>
#include <QtCore/qdatetime.h>
#include <QDebug>
#include <QTextCodec>
#include <random>
#include <math.h>

extern char VERTICAL;
extern char HORIZONTAL;

#define KEY     1
#define MESSAGE 2
#define ANSWER  4
#define RETRY   8
#define END     128

#define ALICE 0
#define BOB   1
#define EVE   2

class Packet
{
public:
    qint16     transaction_no;
    QDateTime  dateTime;
    qint32     type;
    qint32     from;
    QString    photon;
};

QDataStream &operator>>(QDataStream &stream, Packet &pckt);
QDataStream &operator<<(QDataStream &stream, const Packet &pckt);

qint32 rnd(int min=0, int max=100);
qint32 rnd_poisson(double mean=1);

QByteArray key_to_bytearray( QString key);
QByteArray text_to_bytearray(QString text);

QString bytearray_to_key( QByteArray message);
QString bytearray_to_text(QByteArray message);

QString noisy(  QString input, qint32 noise);
QString denoisy(QString raw);

QChar rand_photon();
QChar rand_polarization();

QString polarize(     QString polarization, QByteArray message);
QByteArray depolarize(QString polarization, QString    photons);

QByteArray code_hamming(  QByteArray message);
QByteArray decode_hamming(QByteArray message);

QString format_HTML(QString photons, QString stock);

#endif // PACKET_H
