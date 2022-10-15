#include "modbusthread.h"

ModbusThread::ModbusThread()
{
    qRegisterMetaType<cmdType>("cmdType");
}

ModbusThread::~ModbusThread()
{

}

void ModbusThread::handleCmd()
{
    switch (cmd) {
    case disConnectModbus: {
        modbusDevice->disconnectDevice();
        cmd = Free;
    }break;
    case connectModbus: {
        if (modbusDevice) {
            modbusDevice->disconnectDevice();
            delete modbusDevice;
        }
        if (modbusSet.connectType == Serial) {
            modbusDevice = new QModbusRtuSerialMaster(this);
            if (!modbusDevice) {
                errString = "创建modbus Serial 服务端对象失败!";
                emit updateValueShowSignal(errorOccurs);
                cmd = Free;
                return;
            }
            else {
                modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, modbusSet.portText);
                modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, modbusSet.parity);
                modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, modbusSet.baud);
                modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, modbusSet.dataBits);
                modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, modbusSet.stopBits);
            }
        }
        else {
            modbusDevice = new QModbusTcpClient(this);
            if (!modbusDevice) {
                errString = "创建modbus Tcp 客户端对象失败!";
                emit updateValueShowSignal(errorOccurs);
                cmd = Free;
                return;
            }
            else {
                const QUrl url = QUrl::fromUserInput(modbusSet.portText);
                modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, url.port());
                modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, url.host());
            }
        }
        modbusDevice->setTimeout(modbusSet.responseTime);
        modbusDevice->setNumberOfRetries(modbusSet.numberOfRetries);
        connect(modbusDevice, &QModbusClient::stateChanged, this, &ModbusThread::onStateChanged);
        connect(modbusDevice, &QModbusClient::errorOccurred, this, &ModbusThread::onErrorOccurred);
        modbusDevice->connectDevice();
        cmd = Free;
    }break;
    case ReadModbus: {
        readUnit = QModbusDataUnit(static_cast<QModbusDataUnit::RegisterType>(modbusSet.registerType), modbusSet.beginAddress, modbusSet.valueNum);
        sendReadRequest();
    }break;
    case WriteModbus: {
        sendWriteRequest();
    }break;
    default:break;
    }
}

void ModbusThread::sendReadRequest()
{
    auto *reply = modbusDevice->sendReadRequest(readUnit, modbusSet.server);
    if (reply) {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusThread::readReady);
        else {
            errString = tr("读取失败: ") + modbusDevice->errorString();
            emit updateValueShowSignal(errorOccurs);
            delete reply;
            cmd = Free;
        }
    }
    else {
        errString = tr("读取失败: ") + modbusDevice->errorString();
        emit updateValueShowSignal(errorOccurs);
        delete reply;
        cmd = Free;
    }
}

void ModbusThread::sendWriteRequest()
{
    auto *reply = modbusDevice->sendWriteRequest(writeUnit, modbusSet.server);
    if (reply) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, [this, reply]() {
                if (reply->error() == QModbusDevice::NoError) {
                    cmd = ReadModbus;
                    handleCmd();
                }
                else {
                    errString = tr("Write error:%1(Mobus exception: 0x%2)")
                        .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
                    emit updateValueShowSignal(errorOccurs);
                    cmd = Free;
                }
                delete reply;
            });
        }
    }
    else {
        errString = tr("写入失败: ") + modbusDevice->errorString();
        emit updateValueShowSignal(errorOccurs);
        delete reply;
        cmd = Free;
    }
}

void ModbusThread::readReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;
    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        unitVector.clear();
        for (int i = 0; i < unit.valueCount(); i++)
            unitVector.append(unit.value(i));
        emit updateValueShowSignal(ReadModbus);
    }
    else if (reply->error() == QModbusDevice::TimeoutError) {
        errString = tr("超时: ") + modbusDevice->errorString();
        modbusDevice->disconnectDevice();
        emit updateValueShowSignal(errorOccurs);
    }
    else {
        errString = tr("Read error:%1(code: 0x%2)").arg(reply->errorString()).arg(reply->error(), -1, 16);
        emit updateValueShowSignal(errorOccurs);
    }
    cmd = Free;
    reply->deleteLater();
}

void ModbusThread::run()
{
    exec();
    if (modbusDevice) {
        if (modbusDevice->state() == QModbusDevice::ConnectedState)
            modbusDevice->disconnectDevice();
        delete modbusDevice;
    }
    modbusDevice = nullptr;
}
