#ifndef TCP_H
#define TCP_H

#include <QObject>
#include <QTcpSocket>
#include <iostream>
#include <QTimer>
#include <QDebug>


// Use std namespace for cout/cerr as in the original file
using std::cout;
using std::cerr;
using std::endl;
class Tcp : public QObject
{
    Q_OBJECT
public:
    explicit Tcp(QObject *parent = nullptr);
    ~Tcp();

    // TCP configuration enums
    enum class TcpRole {
        Client,
        Server
    };

    bool connectDevice(
                       const std::string &ip = "127.0.0.1",
                       int port = 8080,
                       TcpRole role = TcpRole::Client,
                       bool keepAlive = true,
                       bool noDelay = true,
                       int reconnectMs = 0);



    // Public methods
    void disconnectDevice();
    void sendData(const QByteArray &data);


    // Public members
    QByteArray buffer;

signals:
    void dataReady(const QByteArray &data);
    void readReady();

private slots:
    void reciveData();

private:
    // Pointers to communication objects

     QTcpSocket *socket = nullptr;

};

#endif // TCP_H
