
#include "modbus.h"

Modbus::Modbus(QObject *parent) : QObject(parent)
{
}

Modbus::~Modbus()
{
    disconnectDevice();
}

// Note: Default parameters are only in the header file
//overload for Modbus RTU connection
bool Modbus::connectDevice(
                            ModbusRtu mode,
                            const std::string &port,
                            int slaveId,
                            BaudRate baud,
                            DataBits dataBits,
                            Parity parity,
                            StopBits stopBits,
                            int timeoutMs,
                            int retries) {

    try {
        disconnectDevice(); // Disconnect any existing connection first

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

            //can be removed
            if (!modbusClient->connectDevice()) {
                qCritical() << "Failed to connect Modbus:" << modbusClient->errorString();
                disconnectDevice(); // Clean up
                return false;
            }
            qInfo()<<"Modbus RTU " << QString::fromStdString(port) <<" Connected Sucessfully";
            connect(modbusClient, &QModbusClient::errorOccurred, this,
                    [=](QModbusDevice::Error error){
                        if (error != QModbusDevice::NoError) {
                            emit errorOccurredSignal(modbusClient->errorString());
                        }
                    });

            return true;


        }}catch(...){
            qCritical() << "Exception in connectDevice(Modbus)";
            return false;
        }
   return false;


}


//overload for Modbus TCP connection
bool Modbus::connectDevice(
                            ModbusTcp mode,
                            const std::string &ip,
                            int tcpPort,
                            int slaveId,
                            int timeoutMs,
                            int retries) {
    try{
        disconnectDevice(); // Disconnect any existing connection first

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



//Disconnect Device for all types of connection
void Modbus::disconnectDevice() {
    try {

            if (modbusClient) {
                if (modbusClient->state() != QModbusDevice::UnconnectedState) {
                    modbusClient->disconnectDevice();
                    qInfo() << "Modbus device disconnected.";
                }
                delete modbusClient;
                modbusClient = nullptr;
            }

    } catch (...) {
        qCritical() << "Exception in disconnectDevice()";
    }
}

//SendData
void Modbus::sendData(const QByteArray &data,RegisterType registerType,int startAddress) {
    try {

        if (modbusClient && modbusClient->state() == QModbusDevice::ConnectedState) {
            // Determine count based on register type
            int count = (static_cast<QModbusDataUnit::RegisterType>(registerType) == QModbusDataUnit::Coils) ? data.size() : (data.size() + 1) / 2;
            if (count == 0) {
                qWarning() << "Modbus sendData: No data to send.";
                return;
            }

            // Create write unit with start address 0
            QModbusDataUnit writeUnit(static_cast<QModbusDataUnit::RegisterType>(registerType), startAddress, count);

            // Fill values
            if ( static_cast<QModbusDataUnit::RegisterType>(registerType)== QModbusDataUnit::Coils) {
                for (int i = 0; i < data.size(); ++i)
//                        writeUnit.setValue(i, static_cast<quint8>(data.at(i)) != 0);
                      writeUnit.setValue(i, (static_cast<quint8>(data.at(i)) & 0x01) != 0);
            } else { // Holding registers
                for (int i = 0, j = 0; i < data.size(); i += 2, ++j) {
                    quint16 value = static_cast<quint8>(data.at(i));
                    if (i + 1 < data.size())
                        value = (value << 8) | static_cast<quint8>(data.at(i + 1));
                    writeUnit.setValue(j, value);
                }
            }

            // Send request
            if (auto *reply = modbusClient->sendWriteRequest(writeUnit, modbusSlaveId)) {
                if (!reply->isFinished()) {
                    connect(reply, &QModbusReply::finished, this, [reply]() {
                        if (reply->error() != QModbusDevice::NoError)
                            qWarning() << "Modbus write error:" << reply->errorString();
                        else
                            qInfo() << "Modbus write successful.";
                        reply->deleteLater();
                    });
                } else {
                    // Handle broadcast replies
                    if (reply->error() != QModbusDevice::NoError)
                        qWarning() << "Modbus write error:" << reply->errorString();
                    delete reply;
                }
            } else {
                qWarning() << "Modbus write request failed:" << modbusClient->errorString();
            }
        } else {
            qWarning() << "Modbus client not connected!";
        }

    } catch (...) {
        qCritical() << "Exception in sendData()";
    }
}

//Modbus requires read request spcfiying which register to read in order to recive data
bool Modbus::readModbusData(Modbus::RegisterType registerType, int startAddress, int numberOfEntries, int slaveId) {
    if (!modbusClient || modbusClient->state() != QModbusDevice::ConnectedState) {
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
                    emit dataReady(data);// <-- EMIT THE SIGNAL
                    emit readReady();
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


void Modbus::reciveData() {
//

}



