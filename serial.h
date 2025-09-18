#ifndef SERIAL_H
#define SERIAL_H


#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <iostream>
#include <strings.h>
#include <QObject>

using namespace std;
class Serial : public QObject{
Q_OBJECT

public:
    explicit Serial(QObject *parnt=nullptr);
    void disconnectDevice();
    bool connectDevice(QString);
    bool sendData(QByteArray );
    QByteArray data=QByteArray();
signals:
    void dataRecived(QByteArray data);

private slots:
    void reciveData();

private:
    QSerialPort *port=nullptr;


};

#endif // SERIAL_H
