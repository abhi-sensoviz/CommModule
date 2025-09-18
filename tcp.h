#ifndef TCP_H
#define TCP_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <iostream>

using namespace std;
class Tcp : public QObject
{
    Q_OBJECT
public:
    explicit Tcp(QObject *parent = nullptr);
    void disconnectDevice();
    bool connectDevice(QString,quint16);
    bool sendData(QByteArray );
    QByteArray data=QByteArray();
signals:
    void dataRecived(QByteArray data);

private slots:
    void reciveData();
    void checkConnection();
    void errorConnection(QAbstractSocket::SocketError);

private:

    QTcpSocket *socket=nullptr;


public slots:
};

#endif // TCP_H
