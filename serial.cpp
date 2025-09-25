#include "serial.h"

Serial::Serial(QObject *parent) : QObject(parent)
{

}


Serial::~Serial()
{
    disconnectDevice();
}

// Note: Default parameters are only in the header file

//overload for serial connection
bool Serial::connectDevice(
                            const std::string &port,
                            BaudRate baud,
                            DataBits bits,
                            Parity parity,
                            StopBits stopBits,
                            FlowControl flow) {
    try{
        disconnectDevice(); // Disconnect any existing connection first

        serial = new QSerialPort(this);

        serial->setPortName(QString::fromStdString(port));
        serial->setBaudRate(baud); // No cast needed for unscoped enum
        serial->setDataBits(static_cast<QSerialPort::DataBits>(bits));
        serial->setParity(static_cast<QSerialPort::Parity>(parity));
        serial->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
        serial->setFlowControl(static_cast<QSerialPort::FlowControl>(flow));

        if (serial->open(QIODevice::ReadWrite)) {
            cout << "Port opened " << port << endl;
            connect(serial, &QSerialPort::readyRead, this, &Serial::reciveData);
            connect(serial, &QSerialPort::readyRead, this, [this]() {
                emit readReady();
            });
            connect(serial, &QSerialPort::errorOccurred, this,
                    [=](QSerialPort::SerialPortError error){
                        if (error != QSerialPort::NoError) {
                            emit errorOccurredSignal(serial->errorString());
                        }
                    });

            return true;
        } else {
            cerr << "error in opening serial " << port << " : " << serial->errorString().toStdString() << endl;
            disconnectDevice(); // Clean up on failure
        }
    } catch(...) {
        cerr << "Exception in connectDevice(Serial)" << endl;
    }
    return false;
}




//Disconnect Device for all types of connection
void Serial::disconnectDevice() {
    try {

            if (serial) {
                if (serial->isOpen()) {
                    serial->close();
                    cout << "Serial port disconnected."<<endl;
                }
                delete serial;
                serial = nullptr;
            }

    } catch (...) {
        cerr << "Exception in disconnectDevice()"<<endl;
    }
}

//SendData
void Serial::sendData(const QByteArray &data) {
    try {
        if (serial && serial->isOpen()) {
            //crc
            serial->write(data);
            serial->flush();
            cout << "Sent via Serial:" << data.toHex(':').toStdString() <<endl;
        } else { cout<<"Serial port not open!"; }

    } catch (...) {
        cout<< "Exception in sendData()";
    }
}


//recive data is called whenerver data is available (redReady emmited) in conn and emits dataReady() singal
void Serial::reciveData() {
    try {

        if (serial && serial->bytesAvailable() > 0) {
            QByteArray data = serial->readAll();
            cout<< "Received via Serial:" << data.toHex(':').toStdString();
            buffer.append(data);
            emit dataReady(buffer);
            buffer.clear();
        }

    } catch (...) {
        cerr << "Exception in reciveData()"<<endl;
    }
}



