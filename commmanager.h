#ifndef COMMMANAGER_H
#define COMMMANAGER_H

#include <QObject>
#include <QBitArray>
#include <QDebug>
#include <QDataStream>


class CommManager : public QObject
{
    Q_OBJECT
public:
    explicit CommManager(QObject *parent = nullptr);
    ~CommManager();

    //frame and protocol handling
    QByteArray addFrameDelimiter(const QByteArray &payload,char startLimiter, char endLimiter);
    QByteArray addFrameDelimiter(const QList<QByteArray> &payload,char startLimiter, char endLimiter,char seperator);
    QByteArray stripFrameDelimiter(const QByteArray &rawFrame,char startLimiter, char endLimiter);
    QList<QByteArray> stripFrameDelimiter(const QByteArray &rawFrame,char startLimiter, char endLimiter,char seperator);
   QList<QByteArray> splitFrames(const QByteArray &rawBuffer,char startLimiter, char endLimiter); //split multiple frames/packets
    int extractIntegerValue(const QByteArray &rawFrame,char startLimiter, char endLimiter);
    QList<int> extractIntegerValue(const QByteArray &rawFrame,char startLimiter, char endLimiter,char seperator);


    //crc
    quint16 calculateCRC(const QByteArray &data);
    bool validateCRC(const QByteArray &frame);
    QByteArray appendCRC(const QByteArray &frame);


    //Data conversion
    int bytesToInt(const QByteArray &data, bool littleEndian, bool sign);
    QByteArray intToBytes(int value, int size, bool littleEndian);
    QBitArray byteToBits(quint8 byte);




signals:

public slots:
};

#endif // COMMMANAGER_H
