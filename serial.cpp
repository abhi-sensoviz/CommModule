#include "serial.h"

Serial::Serial(QObject* parent){

}

//connect device
bool Serial::connectDevice(QString portname){

    if(port!=nullptr){
        port->close();
        delete port;
    }
    port=new QSerialPort(this);

    //configuring port read config from file if needed
    port->setPortName(portname);
    port->setBaudRate(QSerialPort::Baud9600);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);

    //opean port
    if(port->open(QIODevice::ReadWrite)){
        cout<<"Port opened "<<portname.toStdString()<<endl;
        connect(port,&QSerialPort::readyRead,this,&Serial::reciveData);
        return true;
    }else{
        cerr<<"error in opening port"<<portname.toStdString()<<endl;
        return false;
    }


}


//send data
bool Serial::sendData(QByteArray data){

    if(port==nullptr){
        cerr <<"cant send data"<<endl;
        return false;
    }
    if(port->isWritable()){
        if(port->write(data)){
            cout <<"write sucessfull"<<endl;
            return true;
        }else{
            cerr<<"write failed"<<endl;
            return false;
        }
    }else{
        cerr<<"port not opaned"<<endl;
        return false;
    }

}


//recive data
void Serial::reciveData(){
    if(port==nullptr){
        cerr<<"cant read data port is null"<<endl;
        return;
    }if(port->isOpen()){
        QByteArray data=port->readAll();
        emit dataRecived(data);
    }else{
        cerr<<"cant get data"<<endl;
        return;
    }


}


//disconnect port
void Serial::disconnectPort(){

    if(port && port->isOpen()){
        port->close();
        delete port;
        port=nullptr;
        cout <<"port closed"<<endl;
    }else{
        cerr <<"error in closing port"<<endl;
    }
    return;
}











