
#include "plccomm.h"

PlcComm::PlcComm(QObject *parent) : QObject(parent)
{
}

PlcComm::~PlcComm()
{
    disconnectDevice();
}

// Note: Default parameters are only in the header file
bool PlcComm::connectDevice(SerialType type,
                            const std::string &port,
                            BaudRate baud,
                            DataBits bits,
                            Parity parity,
                            StopBits stopBits,
                            FlowControl flow) {
    try{
        disconnectDevice(); // Disconnect any existing connection first
        CurrentType = PlcComm::Type::SERIAL;

        serial = new QSerialPort(this);

        serial->setPortName(QString::fromStdString(port));
        serial->setBaudRate(baud); // No cast needed for unscoped enum
        serial->setDataBits(static_cast<QSerialPort::DataBits>(bits));
        serial->setParity(static_cast<QSerialPort::Parity>(parity));
        serial->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
        serial->setFlowControl(static_cast<QSerialPort::FlowControl>(flow));

        if (serial->open(QIODevice::ReadWrite)) {
            cout << "Port opened " << port << endl;
            connect(serial, &QSerialPort::readyRead, this, &PlcComm::reciveData);
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

bool PlcComm::connectDevice(TcpType type,
                   const std::string &ip,
                   int port,
                   TcpRole role,
                   bool keepAlive,
                   bool noDelay,
                   int reconnectMs) {
    try {
        disconnectDevice(); // Disconnect any existing connection first
        CurrentType = PlcComm::Type::TCP;

        socket = new QTcpSocket(this);

        socket->setSocketOption(QAbstractSocket::KeepAliveOption, keepAlive);
        socket->setSocketOption(QAbstractSocket::LowDelayOption, noDelay);

        QString qip = QString::fromStdString(ip);
        if (role == TcpRole::Client) {
            connect(socket, &QTcpSocket::readyRead, this, &PlcComm::reciveData);
            connect(socket, &QTcpSocket::connected, this, &PlcComm::checkConnection);
            connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &PlcComm::errorConnection);

            if (reconnectMs > 0) {
                QTimer *timer = new QTimer(this);
                timer->setObjectName("reconnectTimer"); // Name for later retrieval
                timer->setSingleShot(true);
                connect(socket, &QTcpSocket::disconnected, this, [=]() mutable {
                    if (CurrentType == Type::TCP && socket) {
                         qWarning() << "Socket disconnected, retry in" << reconnectMs << "ms";
                         timer->start(reconnectMs);
                    } else {
                         timer->deleteLater();
                    }
                });
                connect(timer, &QTimer::timeout, this, [=]() {
                    if (CurrentType == Type::TCP && socket) {
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


bool PlcComm::connectDevice(ModbusType type,
                            ModbusRtu mode,
                            const std::string &port,
                            BaudRate baud,
                            DataBits dataBits,
                            Parity parity,
                            StopBits stopBits,
                            int slaveId,
                            int timeoutMs,
                            int retries) {

    try {
        disconnectDevice(); // Disconnect any existing connection first
        CurrentType = PlcComm::Type::MODBUS;
        this->modbusSlaveId = slaveId; // Store the slave ID

        if (mode == ModbusRtu::RTU) {
            if (port.empty()) {
                qCritical() << "Modbus RTU requires a port name!";
                return false;
            }
            auto modbusMaster = new QModbusRtuSerialMaster(this);
            modbusMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, QString::fromStdString(port));
            modbusMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, static_cast<int>(parity));
            modbusMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baud);
            modbusMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, static_cast<int>(dataBits));
            modbusMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, static_cast<int>(stopBits));
            modbusClient = modbusMaster;

            modbusClient->setTimeout(timeoutMs);
            modbusClient->setNumberOfRetries(retries);

            if (!modbusClient->connectDevice()) {
                qCritical() << "Failed to connect Modbus:" << modbusClient->errorString();
                disconnectDevice(); // Clean up
                return false;
            }
            qInfo()<<"Modbus RTU " << QString::fromStdString(port) <<" Connected Sucessfully";
            return true;


        }}catch(...){
            qCritical() << "Exception in connectDevice(Modbus)";
            return false;
        }
   return false;


}



bool PlcComm::connectDevice(ModbusType type,
                            ModbusTcp mode,
                            const std::string &ip,
                            int tcpPort,
                            int slaveId,
                            int timeoutMs,
                            int retries) {
    try{
        disconnectDevice(); // Disconnect any existing connection first
        CurrentType = PlcComm::Type::MODBUS;
        this->modbusSlaveId = slaveId; // Store the slave ID

        if (mode == ModbusTcp::TCP) {
            if (ip.empty()) {
                qCritical() << "Modbus TCP requires an IP address!";
                return false;
            }
            auto modbusMaster = new QModbusTcpClient(this);
            modbusMaster->setConnectionParameter(QModbusDevice::NetworkAddressParameter, QString::fromStdString(ip));
            modbusMaster->setConnectionParameter(QModbusDevice::NetworkPortParameter, tcpPort);
            modbusClient = modbusMaster;
        }

        modbusClient->setTimeout(timeoutMs);
        modbusClient->setNumberOfRetries(retries);

        if (!modbusClient->connectDevice()) {
            qCritical() << "Failed to connect Modbus:" << modbusClient->errorString();
            disconnectDevice(); // Clean up
            return false;
        }

        qInfo() << "Modbus client connected successfully.";
        return true;

    } catch (...) {
        qCritical() << "Exception in connectDevice(Modbus)";
        return false;
    }
}



void PlcComm::disconnectDevice() {
    try {
        switch (CurrentType) {
            case Type::SERIAL:
                if (serial) {
                    if (serial->isOpen()) {
                        serial->close();
                        qInfo() << "Serial port disconnected.";
                    }
                    delete serial;
                    serial = nullptr;
                }
                break;

            case Type::TCP:
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
                break;

            case Type::MODBUS:
                if (modbusClient) {
                    if (modbusClient->state() != QModbusDevice::UnconnectedState) {
                        modbusClient->disconnectDevice();
                        qInfo() << "Modbus device disconnected.";
                    }
                    delete modbusClient;
                    modbusClient = nullptr;
                }
                break;

            case Type::NONE:
                break; // Nothing to do
        }
        CurrentType = Type::NONE; // Reset state
    } catch (...) {
        qCritical() << "Exception in disconnectDevice()";
    }
}

void PlcComm::sendData(const QByteArray &data,RegisterType registerType) {
    try {
        switch (CurrentType) {
        case Type::SERIAL:
            if (serial && serial->isOpen()) {
                serial->write(data);
                serial->flush();
                qInfo() << "Sent via Serial:" << data.toHex(':');
            } else { qWarning() << "Serial port not open!"; }
            break;

        case Type::TCP:
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(data);
                socket->flush();
                qInfo() << "Sent via TCP:" << data.toHex(':');
            } else { qWarning() << "TCP socket not connected!"; }
            break;

        case Type::MODBUS:
            if (modbusClient && modbusClient->state() == QModbusDevice::ConnectedState) {
                QVector<quint16> registers;
                for (int i = 0; i < data.size(); i += 2) {
                    quint16 value = static_cast<quint8>(data.at(i));
                    if (i + 1 < data.size()) {
                        value = (value << 8) | static_cast<quint8>(data.at(i + 1));
                    }
                    registers.append(value);
                }

                if (registers.isEmpty()) {
                    qWarning() << "Modbus sendData: No data to send.";
                    return;
                }
                //change the register type here
                QModbusDataUnit writeUnit(static_cast<QModbusDataUnit::RegisterType>(registerType), 0, registers);

                if (auto *reply = modbusClient->sendWriteRequest(writeUnit, modbusSlaveId)) {
                    if (!reply->isFinished()) {
                        connect(reply, &QModbusReply::finished, this, [reply]() {
                            if (reply->error() != QModbusDevice::NoError) {
                                qWarning() << "Modbus write error:" << reply->errorString();
                            } else { qInfo() << "Modbus write successful."; }
                            reply->deleteLater();
                        });
                    } else { // Handle finished replies (e.g., broadcast)
                         if (reply->error() != QModbusDevice::NoError) {
                            qWarning() << "Modbus write error:" << reply->errorString();
                         }
                        delete reply;
                    }
                } else { qWarning() << "Modbus write request failed:" << modbusClient->errorString(); }
            } else { qWarning() << "Modbus client not connected!"; }
            break;

        case Type::NONE:
            qWarning() << "No active connection to send data!";
            break;
        }
    } catch (...) {
        qCritical() << "Exception in sendData()";
    }
}

bool PlcComm::readModbusData(PlcComm::RegisterType registerType, int startAddress, int numberOfEntries, int slaveId) {
    if (CurrentType != Type::MODBUS || !modbusClient || modbusClient->state() != QModbusDevice::ConnectedState) {
        qWarning() << "Modbus client is not connected.";
        return false;
    }
    // Convert your wrapper enum to Qt enum

    // Create the Modbus data unit
    QModbusDataUnit readUnit(static_cast<QModbusDataUnit::RegisterType>(registerType), startAddress, numberOfEntries);

    int targetSlaveId = (slaveId > 0) ? slaveId : this->modbusSlaveId;

    if (auto *reply = modbusClient->sendReadRequest(readUnit, targetSlaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::NoError) {
                    const QModbusDataUnit unit = reply->result();
                    QByteArray data;
                    for (const quint16 val : unit.values()) {
                        // Big-endian conversion
                        data.append(static_cast<char>((val >> 8) & 0xFF));
                        data.append(static_cast<char>(val & 0xFF));
                    }
                    qInfo() << "Modbus read successful. Data:" << data.toHex(':');
                    emit dataRecived(data);// <-- EMIT THE SIGNAL
                } else {
                    qWarning() << "Modbus read error:" << reply->errorString();
                }
                reply->deleteLater();
            });
        } else {
            // Handle immediate reply (e.g., error)
            if (reply->error() != QModbusDevice::NoError) {
                 qWarning() << "Modbus read error:" << reply->errorString();
            }
            delete reply;
        }
        return true;
    } else {
        qWarning() << "Modbus read request failed:" << modbusClient->errorString();
        return false;
    }
}

void PlcComm::reciveData() {
    try {
        switch (CurrentType) {
        case Type::SERIAL:
            if (serial && serial->bytesAvailable() > 0) {
                QByteArray data = serial->readAll();
                qInfo() << "Received via Serial:" << data.toHex(':');
                buffer.append(data);
                emit dataRecived(buffer);
                buffer.clear();
            }
            break;

        case Type::TCP:
            if (socket && socket->bytesAvailable() > 0) {
                QByteArray data = socket->readAll();
                qInfo() << "Received via TCP:" << data.toHex(':');
                buffer.append(data);
                emit dataRecived(buffer);
                buffer.clear();
            }
            break;

        case Type::MODBUS:
             qWarning() << "reciveData() called for Modbus, but reads should be initiated explicitly via readModbusData().";
             break;

        case Type::NONE:
             break; // Should not happen
        }
    } catch (...) {
        qCritical() << "Exception in reciveData()";
    }
}

void PlcComm::checkConnection() {
    cout << "Connected to TCP host." << endl;
}

void PlcComm::errorConnection(QAbstractSocket::SocketError /*err*/) {
    if(socket) {
        cerr << "Cannot connect: " << socket->errorString().toStdString() << endl;
    }
}

