#include "packet.h"

QDataStream &operator>>(QDataStream &stream, Packet &pckt) {
    stream >> pckt.transaction_no;
    stream >> pckt.dateTime;
    stream >> pckt.type;
    stream >> pckt.from;
    stream >> pckt.photon;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const Packet &pckt) {
    stream << pckt.transaction_no;
    stream << pckt.dateTime;
    stream << pckt.type;
    stream << pckt.from;
    stream << pckt.photon;
    return stream;
}

qint32 rnd(int min, int max){

#ifdef __MINGW32__
    std::mt19937 generator(qrand());
#else
    std::random_device rand;
    std::mt19937 generator(rand());
#endif
    std::uniform_int_distribution<> distribution(min,max);

    return distribution(generator);
}

qint32 rnd_poisson(double mean){

#ifdef __MINGW32__
    std::mt19937 generator(qrand());
#else
    std::random_device rand;
    std::mt19937 generator(rand());
#endif
    std::poisson_distribution<int> distribution(mean);

    return distribution(generator);
}

QChar rand_photon(){
    switch(rnd(0,3)) {
        case 0: return QChar('|' );
        case 1: return QChar('-' );
        case 2: return QChar('/' );
        case 3: return QChar('\\');
    }
}

QChar rand_polarization(){
    return rnd(0,1) ? 'X' : '+';
}

QString noisy(QString input, qint32 noise){
    if(input.contains("+") || input.contains("X")) return input;

    for(int i=0; i<input.count(); ++i){
        if (rnd() < noise) {
            input[i]=rand_photon();
        }
    }

    return input;
}

QString denoisy(QString raw){
    if(raw.contains("+") || raw.contains("X")) return QString(raw[0]);

    int count_slash=0;
    int count_antislash=0;
    int count_dash=0;
    int count_line=0;
    int _max=-1;

    for(int i=0; i<raw.count(); ++i){
               if (QString(raw[i]) == QString('|' )){
            ++count_line;
            _max=std::max<int>(_max,count_line);
        } else if (QString(raw[i]) == QString('-' )){
            ++count_dash;
            _max=std::max<int>(_max,count_dash);
        } else if (QString(raw[i]) == QString('/')){
            ++count_slash;
            _max=std::max<int>(_max,count_slash);
        } else if (QString(raw[i]) == QString('\\')){
            ++count_antislash;
            _max=std::max<int>(_max,count_antislash);
        }
    }
        if (_max == count_slash){
        return QString('/');
    } else if (_max == count_antislash){
        return QString('\\');
    } else if (_max == count_dash){
        return QString('-');
    } else if (_max == count_line){
        return QString('|');
    } else {
        return QString(' ');
    }

}

QString polarize(QString polarization, QByteArray message){
    QString result=QString("");

    for(int i=0; i<message.length(); ++i){
        int k = i % polarization.length();
        if (polarization[k] == QChar('X')){
            result.append(message[i] == VERTICAL   ? '\\' : '/');
        } else {
            result.append(message[i] == HORIZONTAL ? '-' : '|' );
        }
    }
    return result;
}

QByteArray depolarize(QString polarization, QString photons){
    QByteArray result;
    qsrand(QTime(0,0,0).msecsTo(QTime::currentTime()));

    for(int i=0; i<photons.length(); ++i){
        int k =  i % polarization.length();
        if (photons[i] == QChar(' ')) {
            result.append(' ');
            continue;
        }

        if (polarization[k] == QChar('X')){
            if (photons[i] == QChar('\\')) {
                result.append(VERTICAL);
            } else if (photons[i] == QChar('/')) {
                result.append((char)(1-VERTICAL));
            } else {
                result.append(rnd(0,1));
            }
        } else {
            if (photons[i] == QChar('-')) {
                result.append(HORIZONTAL);
            } else if (photons[i] == QChar('|')) {
                result.append((char)(1-HORIZONTAL));
            } else {
                result.append(rnd(0,1));
            }
        }
    }
    return result;
}

QByteArray key_to_bytearray(QString key){
    QByteArray result;

    for(int i=0; i< key.length(); ++i) {
        if (key[i] == QChar('0')){
            result.append((char)0);
        } else if (key[i] == QChar('1')){
            result.append((char)1);
        } else {
            result.append((char)2);
        }
    }

    return result;
}

QString bytearray_to_key(QByteArray message){
    QString result;

    for(int i=0; i< message.length(); ++i) {
        if (message[i] < 2) {
            result.append(QString::number(message[i]));
        } else {
            result.append(' ');
        }
    }

    return result;
}


QByteArray text_to_bytearray(QString text){
    QByteArray result;

    QTextCodec * textCodec = QTextCodec::codecForName("UTF-8");
    QByteArray message = textCodec->fromUnicode(QString(text));

    for(int i=0; i<message.length(); ++i){
        uchar msg = message[i];
        result.push_back((msg & 1)   / 1);
        result.push_back((msg & 2)   / 2);
        result.push_back((msg & 4)   / 4);
        result.push_back((msg & 8)   / 8);
        result.push_back((msg & 16)  / 16);
        result.push_back((msg & 32)  / 32);
        result.push_back((msg & 64)  / 64);
        result.push_back((msg & 128) / 128);
    }
    return result;
}

QString bytearray_to_text(QByteArray message){
    QByteArray result;

    for(int i=0; i < message.length(); i += 8){
        uchar msg=0;
        msg += message[i+0] * 1;
        msg += message[i+1] * 2;
        msg += message[i+2] * 4;
        msg += message[i+3] * 8;
        msg += message[i+4] * 16;
        msg += message[i+5] * 32;
        msg += message[i+6] * 64;
        msg += message[i+7] * 128;
        result.push_back(msg);
    }

    QTextCodec * textCodec = QTextCodec::codecForName("UTF-8");
    return textCodec->toUnicode(result);
}

QByteArray code_hamming(QByteArray message){
    QByteArray result;

    for(int i=0; i<message.length(); i+=4){
        QByteArray res;

        res[0] = message[i+0];
        res[1] = message[i+1];
        res[2] = message[i+2];
        res[4] = message[i+3];

        res[6]=res[0]^res[2]^res[4];
        res[5]=res[0]^res[1]^res[4];
        res[3]=res[0]^res[1]^res[2];

        result +=res;
    }
    return result;
}

QByteArray decode_hamming(QByteArray message){
    QByteArray result;

    for(int i=0; i<message.length(); i+=7){
        QByteArray tmp, res;
        int c1,c2,c3,c;

        tmp[0] = message[i+0];
        tmp[1] = message[i+1];
        tmp[2] = message[i+2];
        tmp[3] = message[i+3];
        tmp[4] = message[i+4];
        tmp[5] = message[i+5];
        tmp[6] = message[i+6];

        c1 = tmp[6]^tmp[4]^tmp[2]^tmp[0];
        c2 = tmp[5]^tmp[4]^tmp[1]^tmp[0];
        c3 = tmp[3]^tmp[2]^tmp[1]^tmp[0];
        c  = c3*4+c2*2+c1;

        if (c != 0) {
            tmp[7-c] = 1-tmp[7-c];
        }

        res[0]=tmp[0];
        res[1]=tmp[1];
        res[2]=tmp[2];
        res[3]=tmp[4];

        result +=res;
    }
    return result;
}
