#ifndef MODBUS_H
#define MODBUS_H


// Qt Core
#include <QObject>
#include <QTimer>
#include <QDebug>
#include <QSerialPort>

// Qt Modbus
#include <QModbusClient>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>
#include <QModbusReply>
#include <QModbusDataUnit>

// Standard Library
#include <string>
#include <iostream>

// Use std namespace for cout/cerr as in the original file
using std::cout;
using std::cerr;
using std::endl;



class Modbus : public QObject
{
    Q_OBJECT
public:
    explicit Modbus(QObject *parent = nullptr);
    ~Modbus();

    // Modbus configuration enums
    enum class ModbusTcp {TCP};
    enum class ModbusRtu{RTU};

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

    enum class RegisterType {
        Invalid         = 0,
        DiscreteInputs  = QModbusDataUnit::DiscreteInputs,
        Coils           = QModbusDataUnit::Coils,
        InputRegisters  = QModbusDataUnit::InputRegisters,
        HoldingRegisters= QModbusDataUnit::HoldingRegisters
    };

    bool connectDevice(
                       ModbusRtu mode,
                       const std::string &portName="tmp/ttyv0",
                       int slaveId=1,
                       BaudRate baud = Baud9600,
                       DataBits dataBits = DataBits::Data8,
                       Parity parity = Parity::None,
                       StopBits stopBits = StopBits::OneStop,
                       int timeoutMs = 1000,
                       int retries = 3);

    // TCP
    bool connectDevice(
                       ModbusTcp mode,
                       const std::string &ip="127.0.0.0",
                       int tcpPort = 8080,
                       int slaveId=1,
                       int timeoutMs = 1000,
                       int retries = 3);




    // Public methods
    void disconnectDevice();
    void sendData(const QByteArray &data,RegisterType registerType=RegisterType::HoldingRegisters,int startAddress=0);

    bool readModbusData(RegisterType registerType, int startAddress, int numberOfEntries, int slaveId = -1);

    // Public members
    QByteArray buffer;

signals:
    void dataReady(const QByteArray &data);
    void readReady();
private slots:
    void reciveData();

private:
    // Pointers to communication objects
    QModbusClient *modbusClient = nullptr;

    int modbusSlaveId = 1;
};
#endif // MODBUS_H
