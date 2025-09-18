#include "tcp.h"

Tcp::Tcp(QObject *parent) : QObject(parent)
{

}

bool Tcp::connectDevice(QString ip,quint16 portno){

    if(socket!=nullptr){
        socket->disconnectFromHost();
        delete socket;
        socket=nullptr;
    }
    try {

        socket=new QTcpSocket(this);
        socket->connectToHost(ip,portno);
        connect(socket,&QTcpSocket::readyRead,this,&Tcp::reciveData);
        connect(socket, &QTcpSocket::connected, this, &Tcp::checkConnection);
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorConnection(QAbstractSocket::SocketError)));

//        connect(socket, &QTcpSocket::error, this, &Tcp::errorConnection);
        return true;

    } catch (...) {
        cout <<"Faild connection to "<< ip.toStdString() << " "<<to_string(portno)<<endl;
    }

    return false;

}

bool Tcp::sendData(QByteArray data){
    try {

        socket->write(data);
        return true;
    } catch (...) {
        cerr<<"error in sending data"<<endl;

    }
    return false;
}

void Tcp::reciveData(){
    try {
        if(socket->isOpen()){
            data.append(socket->readAll());
            if(data.length()>3){//change condition
                emit dataRecived(data);
                data.clear();
                return;
            }

        }

    } catch (...) {
        cerr<<"error in reciving Tcp data"<<endl;

    }

}
void Tcp::disconnectDevice(){
    if (socket) {
        socket->disconnectFromHost();
        delete socket;
        socket=nullptr;
    }
}
void Tcp::checkConnection(){
    cout <<"connected to Tcp port"<<endl;
}
void Tcp::errorConnection(QAbstractSocket::SocketError err){
    cout <<"Cannot connect "<<err<<endl;
}




