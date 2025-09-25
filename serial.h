#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QSerialPort>
#include <iostream>
// Use std namespace for cout/cerr as in the original file
using std::cout;
using std::cerr;
using std::endl;

class Serial : public QObject
{
    Q_OBJECT
public:
    explicit Serial(QObject *parent = nullptr);
    ~Serial();

    // Serial port configuration enums (values match QSerialPort)

    enum BaudRate {
        Baud1200   = QSerialPort::Baud1200,
        Baud2400   = QSerialPort::Baud2400,
        Baud4800   = QSerialPort::Baud4800,
        Baud9600   = QSerialPort::Baud9600,
        Baud19200  = QSerialPort::Baud19200,
        Baud38400  = QSerialPort::Baud38400,
        Baud57600  = QSerialPort::Baud57600,
        Baud115200 = QSerialPort::Baud115200
    };
    enum class DataBits {
        Data5 = QSerialPort::Data5,
        Data6 = QSerialPort::Data6,
        Data7 = QSerialPort::Data7,
        Data8 = QSerialPort::Data8
    };
    enum class Parity {
        None  = QSerialPort::NoParity,
        Even  = QSerialPort::EvenParity,
        Odd   = QSerialPort::OddParity,
        Mark  = QSerialPort::MarkParity,
        Space = QSerialPort::SpaceParity
    };
    enum class StopBits {
        OneStop = QSerialPort::OneStop,
        OneAndHalfStop = QSerialPort::OneAndHalfStop,
        TwoStop = QSerialPort::TwoStop
    };
    enum class FlowControl {
        None     = QSerialPort::NoFlowControl,
        Hardware = QSerialPort::HardwareControl,
        Software = QSerialPort::SoftwareControl
    };


    bool connectDevice(
                       const std::string &port = "/dev/pts/2", //default virtual port
                       BaudRate baud = BaudRate::Baud9600,
                       DataBits bits = DataBits::Data8,
                       Parity parity = Parity::None,
                       StopBits stopBits = StopBits::OneStop,
                       FlowControl flow = FlowControl::None);

    void disconnectDevice();
    void sendData(const QByteArray &data);

    // Public members
    QByteArray buffer;

signals:
    void dataReady(const QByteArray &data);
    void readReady();
    void errorOccurredSignal(const QString &msg);

private slots:
    void reciveData();

public:
    // Pointers to communication objects
    QSerialPort *serial = nullptr;
};

#endif // SERIAL_H
