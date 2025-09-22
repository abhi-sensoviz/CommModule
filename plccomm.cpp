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
                            ModbusMode mode,
                            int slaveId,
                            int timeoutMs,
                            int retries,
                            const std::string &portName,
                            BaudRate baud,
                            DataBits dataBits,
                            Parity parity,
                            StopBits stopBits,
                            const std::string &ip,
                            int tcpPort) {
    try {
        disconnectDevice(); // Disconnect any existing connection first
        CurrentType = PlcComm::Type::MODBUS;

        if (mode == ModbusMode::RTU) {
            if (portName.empty()) {
                qCritical() << "Modbus RTU requires a port name!";
                return false;
            }
            auto modbusMaster = new QModbusRtuSerialMaster(this);
            modbusMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, QString::fromStdString(portName));
            modbusMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, static_cast<int>(parity));
            modbusMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baud);
            modbusMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, static_cast<int>(dataBits));
            modbusMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, static_cast<int>(stopBits));
            modbusClient = modbusMaster;
        } else if (mode == ModbusMode::TCP) {
            if (ip.empty()) {
                qCritical() << "Modbus TCP requires an IP address!";
                return false;
            }
            auto modbusMaster = new QModbusTcpClient(this);
            modbusMaster->setConnectionParameter(QModbusDevice::NetworkAddressParameter, QString::fromStdString(ip));
            modbusMaster->setConnectionParameter(QModbusDevice::NetworkPortParameter, tcpPort);
            modbusClient = modbusMaster;
        } else {
            return false; // Unknown mode
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

void PlcComm::sendData(const QByteArray &data) {
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

                QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 0, registers);
                int slaveId = 1; // This should be a parameter or member variable

                if (auto *reply = modbusClient->sendWriteRequest(writeUnit, slaveId)) {
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
             qWarning() << "reciveData() called for Modbus, but reads should be initiated explicitly.";
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


