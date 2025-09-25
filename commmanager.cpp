#include "commmanager.h"

CommManager::CommManager(QObject *parent) : QObject(parent)
{

}
CommManager::~CommManager(){

}


//frame and protocol handling--------------------

//package frame with startLimiter and endLimiter
QByteArray CommManager::addFrameDelimiter(const QByteArray &payload,char startLimiter, char endLimiter){
    if(payload.length()==0){
        qCritical()<<"Not valid payload passed to add FrameDelimiter"<<endl;
        return nullptr;
    }
    QByteArray frame;
    frame.append(startLimiter);
    frame.append(payload);
    frame.append(endLimiter);
    return frame;
}

//package frame with delimater and seperator
QByteArray CommManager::addFrameDelimiter(const QList<QByteArray> &payload,char startLimiter, char endLimiter,char seperator){
    if(payload.length()==0){
        qCritical()<<"Not valid payload passed to add FrameDelimiter"<<endl;
        return nullptr;
    }
    QByteArray frame;
    frame.append(startLimiter);
    for(int i=0;i<payload.length();i++){
        frame.append(seperator);
        frame.append(payload[i]);
    }
    frame.append(endLimiter);
    return frame;
}

//extract payload from rawframe
QByteArray CommManager::stripFrameDelimiter(const QByteArray &rawFrame,char startLimiter, char endLimiter){
    int startIndex=rawFrame.indexOf(startLimiter);
    int endIndex=rawFrame.indexOf(endLimiter);
    if(startIndex==-1||endIndex==-1 || endIndex<startIndex){
        qCritical() << "No Valid Packet"<<endl;
        return QByteArray();
    }
    return rawFrame.mid(startIndex+1,endIndex-startIndex-1);

}

//extract payload seperated with delimator
QList<QByteArray> CommManager::stripFrameDelimiter(const QByteArray &rawFrame,char startLimiter, char endLimiter,char seperator){

    int startIndex=rawFrame.indexOf(startLimiter);
    int endIndex=rawFrame.indexOf(endLimiter);

    if(startIndex==-1||endIndex==-1 || endIndex<startIndex){
        qCritical() << "No Valid Packet"<<endl;
        return QList<QByteArray>();
    }
    QByteArray payload=rawFrame.mid(startIndex+1,startIndex-endIndex-1);
    return payload.split(seperator);

}

//split multiple frames/packets
QList<QByteArray> CommManager::splitFrames(const QByteArray &rawBuffer,char startLimiter, char endLimiter){

    QList<QByteArray> frameList;
    int start=-1;

    for(int i=0;i<rawBuffer.length();i++){
       if(rawBuffer[i]==startLimiter)start=i;
       else if(rawBuffer[i]==endLimiter && start!=-1){
           int end=i-start+1;
           frameList.append(rawBuffer.mid(start,end));
           start=-1;
       }
    }
    return frameList;
}


//crc ---------------------------
//crc check
quint16 CommManager::calculateCRC(const QByteArray &data)
{
    quint16 crc = 0xFFFF; // initial value
    for (auto b : data) {
        crc ^= static_cast<quint8>(b);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001; // polynomial for CRC-16-IBM
            else
                crc >>= 1;
        }
    }
    return crc;
}
bool CommManager::validateCRC(const QByteArray &frame)
{
    if (frame.size() < 3) // at least 1 byte data + 2 bytes CRC
        return false;

    QByteArray data = frame.left(frame.size() - 2); // payload without CRC
    quint16 crcInFrame = static_cast<quint8>(frame[frame.size() - 2]) |
                         (static_cast<quint8>(frame[frame.size() - 1]) << 8);

    quint16 crcCalc = calculateCRC(data);

    return crcCalc == crcInFrame;
}

QByteArray CommManager::appendCRC(const QByteArray &frame)
{
    quint16 crc = calculateCRC(frame);
    QByteArray out = frame;
    out.append(static_cast<char>(crc & 0xFF));       // low byte
    out.append(static_cast<char>((crc >> 8) & 0xFF)); // high byte
    return out;
}



//Data conversion--------------

//extract Integer form raw frame
int CommManager::extractIntegerValue(const QByteArray &rawFrame,char startLimiter, char endLimiter){

    QByteArray payload = stripFrameDelimiter(rawFrame,startLimiter,endLimiter);
    QString valuestr=QString::fromUtf8(payload);
    bool ok = false;
    int value = valuestr.toInt(&ok);

    if (!ok)
    {
        qDebug() << "Failed to convert to integer:" << valuestr;
        return -1;
    }

    return value;

}

//extract intiger from packet with seperator
QList<int> CommManager::extractIntegerValue(const QByteArray &rawFrame,char startLimiter, char endLimiter,char seperator){
    QList<QByteArray> list=stripFrameDelimiter(rawFrame,startLimiter,endLimiter,seperator);
    QList<int> values;
    for (const QByteArray &val:list) {
        bool ok = false;
        int value = QString::fromUtf8(val).toInt(&ok);
        if (ok)
        {
            values.append(value);
        }
        else
        {
            qDebug() << "Invalid integer conversion for:" << val;
        }
    }
    return values;
}

//convert bytes to int
int CommManager::bytesToInt(const QByteArray &data, bool littleEndian, bool sign){

    if (data.isEmpty() || data.size() > 4) {
        qWarning() << "Invalid data size for int conversion:" << data.size();
        return 0;
    }

    QDataStream ds(data);
    ds.setByteOrder(littleEndian ? QDataStream::LittleEndian : QDataStream::BigEndian);

    quint32 value = 0;

    // Read according to size
    switch (data.size()) {
        case 1: { quint8 v; ds >> v; value = v; break; }
        case 2: { quint16 v; ds >> v; value = v; break; }
        case 3: {
            // No native 24-bit type, read manually
            quint8 b1, b2, b3;
            ds >> b1 >> b2 >> b3;
            if (littleEndian)
                value = b1 | (b2 << 8) | (b3 << 16);
            else
                value = (b1 << 16) | (b2 << 8) | b3;
            break;
        }
        case 4: { quint32 v; ds >> v; value = v; break; }
    }

    // Handle signed conversion
    if (sign) {
        switch (data.size()) {
            case 1: return static_cast<qint8>(value);
            case 2: return static_cast<qint16>(value);
            case 3: return static_cast<qint32>((value << 8) >> 8); // sign extend 24-bit
            case 4: return static_cast<qint32>(value);
        }
    }

    return static_cast<int>(value);


}
//convert int to bytes
QByteArray CommManager::intToBytes(int value, int size, bool littleEndian){
    if (size < 1 || size > 4) {
        qWarning() << "Invalid size for intToBytes:" << size;
        return QByteArray();
    }

    QByteArray data;
    data.resize(size);

    for (int i = 0; i < size; ++i) {
        int shift = littleEndian ? i * 8 : (size - 1 - i) * 8;
        data[i] = static_cast<char>((value >> shift) & 0xFF);
    }

    return data;
}

//convert byte to bits array
QBitArray CommManager::byteToBits(quint8 byte){
    QBitArray bits(8);
    for (int i = 0; i < 8; ++i) {
        bits[i] = (byte >> (7 - i)) & 0x01; // MSB first
    }
    return bits;

}


