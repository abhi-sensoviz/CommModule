#include "serial.h"

Serial::Serial(QObject* parent): QObject (parent){

}

//connect device
bool Serial::connectDevice(QString portname){

    try {

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
            cerr<<"error in opening port "<<portname.toStdString()<<endl;
        }
    } catch (...) {
        cerr<<"error in opening port "<<portname.toStdString()<<endl;
        return false;

    }
    return false;

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
void Serial::reciveData(){  //modify to include prefix and suffix
    try {
    if(port->isOpen()){
        data.append(port->readAll());
//        /emit dataRecived(data); //comment out
        //check if complete data is available and then emit
        //clear after data is recived completly
        //dummy condition
        if(data.length()>3){
            emit(dataRecived(data));
            data.clear();
            return;
        }
    }
    } catch (...) {
        cerr<<"error in reading data"<<endl;
    }

}


//disconnect port
void Serial::disconnectDevice(){

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











