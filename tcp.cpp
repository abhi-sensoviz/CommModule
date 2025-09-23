#include "tcp.h"

Tcp::Tcp(QObject *parent) : QObject(parent)
{

}
Tcp::~Tcp()
{
    disconnectDevice();
}

//overload for Tcp connection
bool Tcp::connectDevice(
                   const std::string &ip,
                   int port,
                   TcpRole role,
                   bool keepAlive,
                   bool noDelay,
                   int reconnectMs) {
    try {
        disconnectDevice(); // Disconnect any existing connection first


        socket = new QTcpSocket(this);

        socket->setSocketOption(QAbstractSocket::KeepAliveOption, keepAlive);
        socket->setSocketOption(QAbstractSocket::LowDelayOption, noDelay);

        QString qip = QString::fromStdString(ip);
        if (role == TcpRole::Client) {
            connect(socket, &QTcpSocket::readyRead, this, &Tcp::reciveData);

            connect(socket, &QTcpSocket::readyRead, this, [this]() {
                emit readReady();
            });

            connect(socket, &QTcpSocket::connected, this, [=]{ std::cout << "Connected to TCP host." << std::endl; });
            connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [=](QAbstractSocket::SocketError){ if(socket) std::cerr << "Cannot connect: " << socket->errorString().toStdString() << std::endl; });



            if (reconnectMs > 0) {
                QTimer *timer = new QTimer(this);
                timer->setObjectName("reconnectTimer"); // Name for later retrieval
                timer->setSingleShot(true);
                connect(socket, &QTcpSocket::disconnected, this, [=]() mutable {
                    if (socket) {
                         qWarning() << "Socket disconnected, retry in" << reconnectMs << "ms";
                         timer->start(reconnectMs);
                    } else {
                         timer->deleteLater();
                    }
                });
                connect(timer, &QTimer::timeout, this, [=]() {
                    if (socket) {
                        qInfo() << "Attempting to reconnect...";
                        socket->connectToHost(qip, port);
                    }
                });
            }
            qInfo() << "Connecting to TCP host" << qip << ":" << port;
            socket->connectToHost(qip, port);
            return true;
        }
        // Could add TcpRole::Server logic here in the future
    } catch (...) {
        cerr << "Exception in connectDevice(TCP)" << endl;
    }
    return false;
}


//Disconnect Device for all types of connection
void Tcp::disconnectDevice() {
    try {

            if (socket) {
                // Stop any pending reconnect timers
                auto *timer = findChild<QTimer*>("reconnectTimer");
                if (timer) {
                    timer->stop();
                    delete timer;
                }
                socket->disconnect(this); // Disconnect signals
                if (socket->state() != QAbstractSocket::UnconnectedState) {
                    socket->disconnectFromHost();
                    socket->waitForDisconnected(100);
                }
                delete socket;
                socket = nullptr;
                qInfo() << "TCP socket disconnected.";
                }

    } catch (...) {
        qCritical() << "Exception in disconnectDevice()";
    }
}



//SendData
void Tcp::sendData(const QByteArray &data) {
    try {

        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                        socket->write(data);
                        socket->flush();
                        qInfo() << "Sent via TCP:" << data.toHex(':');
                    } else { qWarning() << "TCP socket not connected!"; }
    }
     catch (...) {
        qCritical() << "Exception in sendData()";
    }
}

//recive data is called whenerver data is available (redReady emmited) in conn and emits dataReady() singal
void Tcp::reciveData() {
    try {

        if (socket && socket->bytesAvailable() > 0) {
            QByteArray data = socket->readAll();
            qInfo() << "Received via TCP:" << data.toHex(':');
            buffer.append(data);
            emit dataReady(buffer);
            buffer.clear();
        }
    } catch (...) {
        qCritical() << "Exception in reciveData()";
    }
}



