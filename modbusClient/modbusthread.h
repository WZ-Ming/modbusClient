#ifndef MODBUSTHREAD_H
#define MODBUSTHREAD_H

#include<QThread>
#include<QModbusClient>
#include<QSerialPort>
#include<QVector>
#include<QModbusDataUnit>
#include<QSerialPort>
#include<QModbusTcpClient>
#include<QModbusRtuSerialMaster>
#include<QUrl>
#include<qDebug>
#include<QMetaType>

class ModbusThread :public QThread
{
    Q_OBJECT
public:
    ModbusThread();
    ~ModbusThread();

    QString errString;

    QVector<quint16> unitVector;
    QModbusClient *modbusDevice = nullptr;

    enum ModbusConnection { Serial=0, Tcp };
    enum cmdType { ReadModbus = 0, Free, WriteModbus, errorOccurs, connectModbus, disConnectModbus }cmd = Free;

    struct Settings {
        int parity = QSerialPort::NoParity;
        int baud = QSerialPort::Baud115200;
        int dataBits = QSerialPort::Data8;
        int stopBits = QSerialPort::OneStop;
        int responseTime = 1000;
        int numberOfRetries = 3;
        QString portText = "192.168.0.222:502";
        ModbusConnection connectType = Tcp;

        int beginAddress=0;
        int valueNum=10;
        int registerType=QModbusDataUnit::Coils;
        int server=1;
    }modbusSet;

    QModbusDataUnit writeUnit;
    QModbusDataUnit readUnit;

signals:
    void updateValueShowSignal(const cmdType &);
    void onStateChanged(int);
    void onErrorOccurred(QModbusDevice::Error);

private slots:
    void readReady();

public slots:
    void handleCmd();

protected:
    void run();

private:
    void sendReadRequest();
    void sendWriteRequest();

};

#endif // MODBUSTHREAD_H
