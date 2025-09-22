#ifndef PLCCOMM_H
#define PLCCOMM_H

// Qt Core
#include <QObject>
#include <QTimer>
#include <QDebug>

// Qt SerialPort
#include <QSerialPort>

// Qt Network
#include <QTcpSocket>

// Qt Modbus
#include <QModbusClient>
#include <QModbusRtuSerialMaster>
#include <QModbusTcpClient>

// Standard Library
#include <string>
#include <iostream>

// Use std namespace for cout/cerr as in the original file
using std::cout;
using std::cerr;
using std::endl;

class PlcComm : public QObject
{
    Q_OBJECT
public:
    explicit PlcComm(QObject *parent = nullptr);
    ~PlcComm();

    // Communication type enums
    enum class Type {
        NONE, // Represents an inactive state
        SERIAL,
        TCP,
        MODBUS
    };

    // Protocol-specific markers for function overloading
    enum class SerialType { SERIAL };
    enum class TcpType { TCP };
    enum class ModbusType { MODBUS };

    // TCP configuration enums
    enum class TcpRole {
        Client,
        Server
    };

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
        One = QSerialPort::OneStop,
        OneAndHalf = QSerialPort::OneAndHalfStop,
        Two = QSerialPort::TwoStop
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

    // Overloaded connection methods with default parameters
    bool connectDevice(SerialType type,
                       const std::string &port = "/tmp/ttyV0",
                       BaudRate baud = BaudRate::Baud9600,
                       DataBits bits = DataBits::Data8,
                       Parity parity = Parity::None,
                       StopBits stopBits = StopBits::One,
                       FlowControl flow = FlowControl::None);

    bool connectDevice(TcpType type,
                       const std::string &ip = "127.0.0.1",
                       int port = 8080,
                       TcpRole role = TcpRole::Client,
                       bool keepAlive = true,
                       bool noDelay = true,
                       int reconnectMs = 0);

    bool connectDevice(ModbusType type,
                       ModbusRtu mode,
                       const std::string &portName="tmp/ttyv0",
                       BaudRate baud = Baud9600,
                       DataBits dataBits = DataBits::Data8,
                       Parity parity = Parity::None,
                       StopBits stopBits = StopBits::One,
                       int slaveId=1,
                       int timeoutMs = 1000,
                       int retries = 3);

    // TCP
    bool connectDevice(ModbusType type,
                       ModbusTcp mode,
                       const std::string &ip="127.0.0.0",
                       int tcpPort = 8080,
                       int slaveId=1,
                       int timeoutMs = 1000,
                       int retries = 3);




    // Public methods
    void disconnectDevice();
    void sendData(const QByteArray &data,RegisterType registerType=RegisterType::HoldingRegisters);

    bool readModbusData(RegisterType registerType, int startAddress, int numberOfEntries, int slaveId = -1);

    // Public members
    QByteArray buffer;

signals:
    void dataRecived(const QByteArray &data);

private slots:
    void reciveData();
    void checkConnection();
    void errorConnection(QAbstractSocket::SocketError err);

private:
    // Pointers to communication objects
    QSerialPort *serial = nullptr;
    QTcpSocket *socket = nullptr;
    QModbusClient *modbusClient = nullptr;

    Type CurrentType = Type::NONE;
    int modbusSlaveId = 1;


};

#endif // PLCCOMM_H
