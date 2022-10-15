#include "mainwindow.h"
#include "ui_mainwindow.h"
#pragma execution_character_set("utf-8")

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("modbusClient");
    modbusthread = new ModbusThread;
    modbusthread->moveToThread(modbusthread);

    writeModel = new WriteRegisterModel(ui->beginAddressSpinBox->value(), ui->valueNumSpinBox->value(), this);
    ui->valueTreeView->setModel(writeModel);
    connect(writeModel, &WriteRegisterModel::updateViewport, [this]() {
        ui->valueTreeView->viewport()->update();
        ui->valueTreeView->reset();
    });

    ui->registerTypeCombox->addItem(tr("Coils"), QModbusDataUnit::Coils);
    ui->registerTypeCombox->addItem(tr("Discrete Inputs"), QModbusDataUnit::DiscreteInputs);
    ui->registerTypeCombox->addItem(tr("Input Registers"), QModbusDataUnit::InputRegisters);
    ui->registerTypeCombox->addItem(tr("Holding Registers"), QModbusDataUnit::HoldingRegisters);

    connect(ui->valueNumSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), writeModel, &WriteRegisterModel::setNumberOfValues);
    connect(ui->valueNumSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]() {
        modbusthread->modbusSet.valueNum = ui->valueNumSpinBox->value();
        if (modbusthread->modbusDevice) {
            if (modbusthread->modbusDevice->state() == QModbusDevice::ConnectedState)
                triggerCmd(modbusthread->ReadModbus);
        }
        treeViewHideRow();
    });

    connect(ui->beginAddressSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), writeModel, &WriteRegisterModel::setStartAddress);
    connect(ui->beginAddressSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]() {
        modbusthread->modbusSet.beginAddress = ui->beginAddressSpinBox->value();
        if (modbusthread->modbusDevice) {
            if (modbusthread->modbusDevice->state() == QModbusDevice::ConnectedState)
                triggerCmd(modbusthread->ReadModbus);
        }
        treeViewHideRow();
    });

    connect(ui->serverSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]() {
        modbusthread->modbusSet.server = ui->serverSpinBox->value();
    });

    iniModbusSettings();
    initButtonSignal();

    connect(modbusthread, &ModbusThread::updateValueShowSignal, this, &MainWindow::updateValueShowSlot);
    connect(modbusthread, &ModbusThread::onStateChanged, this, &MainWindow::onStateChanged);
    connect(this, &MainWindow::emitCmd, modbusthread, &ModbusThread::handleCmd);
    connect(modbusthread, &ModbusThread::onErrorOccurred, [this]() {
        emit modbusthread->onStateChanged(10);
    });

    ui->valueTreeView->setColumnWidth(0, 80);
    ui->label_connectState->setAutoFillBackground(true);
    modbusthread->start();

    timer=new QTimer(this);
    connect(timer,&QTimer::timeout,[this](){
        timer->stop();
        ui->label_statusShow->clear();
    });

//    qDebug()<<"串口端口列表";
//    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
//        QSerialPort serial;
//        serial.setPort(info);
//        qDebug()<<serial.portName();
//    }
//    qDebug()<<"--------------";
//    qDebug()<<"网络端口列表";
//    foreach (QHostAddress hostAddress, QNetworkInterface::allAddresses())  {
//       if(hostAddress.protocol() == QAbstractSocket::IPv4Protocol){
//           qDebug()<<hostAddress.toString();
//       }
//    }
}

MainWindow::~MainWindow()
{
    modbusthread->quit();
    modbusthread->wait();
    saveModbusSettings();
    delete modbusthread;
    delete writeModel;
    delete ui;
    delete timer;
}

void MainWindow::onStateChanged(int state)
{
    switch (state) {
    case QModbusDevice::UnconnectedState: {
        ui->label_connectState->setText(tr("连接断开!"));
        ui->label_connectState->setPalette(QPalette(QPalette::Background, Qt::red));
        ui->connectButton->setEnabled(true);
        ui->param_groupBox->setEnabled(true);
    }break;
    case QModbusDevice::ConnectingState: {
        ui->label_connectState->setText(tr("正在连接中..."));
        ui->label_connectState->setPalette(QPalette(QPalette::Background, Qt::yellow));
        ui->connectButton->setEnabled(false);
        ui->param_groupBox->setEnabled(false);
    }break;
    case QModbusDevice::ConnectedState: {
        ui->label_connectState->setText(tr("连接成功!"));
        ui->label_connectState->setPalette(QPalette(QPalette::Background, Qt::green));
        ui->connectButton->setEnabled(false);
        ui->param_groupBox->setEnabled(false);
        ui->readButton->click();
    }break;
    case QModbusDevice::ClosingState:{
        ui->label_connectState->setText(tr("连接关闭!"));
        ui->label_connectState->setPalette(QPalette(QPalette::Background, Qt::red));
        ui->connectButton->setEnabled(true);
        ui->param_groupBox->setEnabled(true);
    }break;

    case 10:{
        ui->label_connectState->setText(tr("连接出错!"));
        ui->label_connectState->setPalette(QPalette(QPalette::Background, Qt::red));
        ui->connectButton->setEnabled(true);
        ui->param_groupBox->setEnabled(true);
        modbusthread->errString = modbusthread->modbusDevice->errorString();
        updateValueShowSlot(ModbusThread::cmdType::errorOccurs);
    }break;
    default: {
        ui->label_connectState->setText(tr("未建立连接!"));
        ui->label_connectState->setPalette(QPalette(QPalette::Background, Qt::yellow));
    } break;
    }
}

void MainWindow::on_valueTypeCombox_currentIndexChanged(const QString &arg1)
{
    if (arg1 == "Binary") writeModel->valueType = writeModel->Binary;
    else if (arg1 == "Float") writeModel->valueType = writeModel->Float;
    else if (arg1 == "Unsigned") writeModel->valueType = writeModel->Unsigned;
    else if (arg1 == "Signed") writeModel->valueType = writeModel->Signed;
    else if (arg1 == "Hex") writeModel->valueType = writeModel->Hex;
    writeModel->updateViewport();
    treeViewHideRow();
}

void MainWindow::on_registerTypeCombox_currentTextChanged(const QString &arg1)
{
    modbusthread->modbusSet.registerType = ui->registerTypeCombox->currentData().toInt();
    ui->writeButton->setEnabled(arg1 == "Coils" || arg1 == "Holding Registers");
    ui->valueTypeCombox->setEnabled(arg1 == "Input Registers" || arg1 == "Holding Registers");
    if (arg1 == "Coils") {
        ui->valueTreeView->setColumnHidden(writeModel->CoilsColumn, false);
        ui->valueTreeView->setColumnHidden(writeModel->DiscreteColum, true);
        ui->valueTreeView->setColumnHidden(writeModel->InputColumn, true);
        ui->valueTreeView->setColumnHidden(writeModel->HoldingColumn, true);
        ui->valueTypeCombox->setCurrentText("Binary");
    }
    else if (arg1 == "Discrete Inputs") {
        ui->valueTreeView->setColumnHidden(writeModel->CoilsColumn, true);
        ui->valueTreeView->setColumnHidden(writeModel->DiscreteColum, false);
        ui->valueTreeView->setColumnHidden(writeModel->InputColumn, true);
        ui->valueTreeView->setColumnHidden(writeModel->HoldingColumn, true);
        ui->valueTypeCombox->setCurrentText("Binary");
    }
    else if (arg1 == "Input Registers") {
        ui->valueTreeView->setColumnHidden(writeModel->CoilsColumn, true);
        ui->valueTreeView->setColumnHidden(writeModel->DiscreteColum, true);
        ui->valueTreeView->setColumnHidden(writeModel->InputColumn, false);
        ui->valueTreeView->setColumnHidden(writeModel->HoldingColumn, true);
    }
    else if (arg1 == "Holding Registers") {
        ui->valueTreeView->setColumnHidden(writeModel->CoilsColumn, true);
        ui->valueTreeView->setColumnHidden(writeModel->DiscreteColum, true);
        ui->valueTreeView->setColumnHidden(writeModel->InputColumn, true);
        ui->valueTreeView->setColumnHidden(writeModel->HoldingColumn, false);
    }
    if (modbusthread->modbusDevice) {
        if (modbusthread->modbusDevice->state() == QModbusDevice::ConnectedState)
            triggerCmd(ModbusThread::ReadModbus);
    }
}

void MainWindow::updateValueShowSlot(const ModbusThread::cmdType &cmd)
{
    switch (cmd) {
    case ModbusThread::ReadModbus: {
        writeModel->salveChangeData = true;
        for (uint i = 0; i < modbusthread->unitVector.size(); i++) {
            if (modbusthread->modbusSet.registerType == QModbusDataUnit::Coils)
                writeModel->setData(writeModel->index(i, writeModel->CoilsColumn), modbusthread->unitVector[i], Qt::CheckStateRole);

            else if (modbusthread->modbusSet.registerType == QModbusDataUnit::DiscreteInputs)
                writeModel->setData(writeModel->index(i, writeModel->DiscreteColum), modbusthread->unitVector[i], Qt::EditRole);

            else if (modbusthread->modbusSet.registerType == QModbusDataUnit::InputRegisters)
                writeModel->setData(writeModel->index(i, writeModel->InputColumn), modbusthread->unitVector[i], Qt::EditRole);

            else if (modbusthread->modbusSet.registerType == QModbusDataUnit::HoldingRegisters)
                writeModel->setData(writeModel->index(i, writeModel->HoldingColumn), modbusthread->unitVector[i], Qt::EditRole);
        }
        writeModel->salveChangeData = false;
    }break;
    case ModbusThread::errorOccurs: {
        emit updateChartSignal(0);
        ui->label_statusShow->setText(modbusthread->errString);
        timer->start(secondsNum);
    }break;
    default:break;
    }
    ui->readButton->setEnabled(true);
    ui->registerTypeCombox->setEnabled(true);
    ui->registerTypeCombox->setEnabled(true);
    ui->writeButton->setEnabled(true);
}

void MainWindow::treeViewHideRow()
{
    if (ui->valueTypeCombox->currentText() == "Float") {
        for (int i = 1; i < ui->valueNumSpinBox->value(); i += 2)
            ui->valueTreeView->setRowHidden(i, writeModel->index(i, 0), true);
    }
    else {
        for (int i = 1; i < ui->valueNumSpinBox->value(); i += 2)
            ui->valueTreeView->setRowHidden(i, writeModel->index(i, 0), false);
    }
}

void MainWindow::triggerCmd(const ModbusThread::cmdType &cmd)
{
    if (modbusthread->cmd != ModbusThread::Free) {
        ui->label_statusShow->setText("正在处理上次操作，请稍后再处理!");
        timer->start(secondsNum);
        return;
    }
    modbusthread->cmd = cmd;
    emit emitCmd();
}

void MainWindow::saveModbusSettings()
{
    QSettings writeIniFile(modbusIniFilePath, QSettings::IniFormat);

    writeIniFile.setValue("parity", modbusthread->modbusSet.parity);
    writeIniFile.setValue("baud", modbusthread->modbusSet.baud);
    writeIniFile.setValue("dataBits", modbusthread->modbusSet.dataBits);
    writeIniFile.setValue("stopBits", modbusthread->modbusSet.stopBits);
    writeIniFile.setValue("connectType", ui->connectTypeCombox->currentText());
    writeIniFile.setValue("port", ui->portEdit->text());
    writeIniFile.setValue("serverAddress", ui->serverSpinBox->value());
    writeIniFile.setValue("responseTime", modbusthread->modbusSet.responseTime);
    writeIniFile.setValue("numberOfRetries", modbusthread->modbusSet.numberOfRetries);

    writeIniFile.setValue("beginAddress", ui->beginAddressSpinBox->value());
    writeIniFile.setValue("valueNum", ui->valueNumSpinBox->value());
    writeIniFile.setValue("registerType", ui->registerTypeCombox->currentText());
    writeIniFile.setValue("valueType", ui->valueTypeCombox->currentText());
}

void MainWindow::iniModbusSettings()
{
    QFile file(modbusIniFilePath);
    if (file.exists()) {
        QSettings readIniFile(modbusIniFilePath, QSettings::IniFormat);

        if (readIniFile.contains("parity")) ui->parityCombox->setCurrentText(readIniFile.value("parity").toString());
        if (readIniFile.contains("baud")) ui->baudCombox->setCurrentText(readIniFile.value("baud").toString());
        if (readIniFile.contains("dataBits")) ui->dataBitsCombox->setCurrentText(readIniFile.value("dataBits").toString());
        if (readIniFile.contains("stopBits")) ui->stopBitsCombox->setCurrentText(readIniFile.value("stopBits").toString());
        if (readIniFile.contains("connectType")) ui->connectTypeCombox->setCurrentText(readIniFile.value("connectType").toString());
        if (readIniFile.contains("port")) ui->portEdit->setText(readIniFile.value("port").toString());
        if (readIniFile.contains("serverAddress")) ui->serverSpinBox->setValue(readIniFile.value("serverAddress").toInt());
        if (readIniFile.contains("responseTime")) ui->timeoutSpinBox->setValue(readIniFile.value("responseTime").toInt());
        if (readIniFile.contains("numberOfRetries")) ui->retriesSpinBox->setValue(readIniFile.value("numberOfRetries").toInt());

        if (readIniFile.contains("beginAddress")) ui->beginAddressSpinBox->setValue(readIniFile.value("beginAddress").toInt());
        if (readIniFile.contains("valueNum")) ui->valueNumSpinBox->setValue(readIniFile.value("valueNum").toInt());
        if (readIniFile.contains("registerType")) ui->registerTypeCombox->setCurrentText(readIniFile.value("registerType").toString());
        if (readIniFile.contains("valueType")) ui->valueTypeCombox->setCurrentText(readIniFile.value("valueType").toString());

        modbusthread->modbusSet.beginAddress = ui->beginAddressSpinBox->value();
        modbusthread->modbusSet.valueNum = ui->valueNumSpinBox->value();
        modbusthread->modbusSet.server = ui->serverSpinBox->value();
        modbusthread->modbusSet.registerType = ui->registerTypeCombox->currentData().toInt();

        modbusthread->modbusSet.parity = ui->parityCombox->currentIndex();
        modbusthread->modbusSet.baud = ui->baudCombox->currentText().toInt();
        modbusthread->modbusSet.dataBits = ui->dataBitsCombox->currentText().toInt();
        modbusthread->modbusSet.stopBits = ui->stopBitsCombox->currentText().toInt();
        modbusthread->modbusSet.responseTime = ui->timeoutSpinBox->value();
        modbusthread->modbusSet.numberOfRetries = ui->retriesSpinBox->value();
        modbusthread->modbusSet.portText = ui->portEdit->text();
        modbusthread->modbusSet.connectType = static_cast<ModbusThread::ModbusConnection> (ui->connectTypeCombox->currentIndex());
    }
    else{
        ui->parityCombox->setCurrentText(QString::number(modbusthread->modbusSet.parity));
        ui->baudCombox->setCurrentText(QString::number(modbusthread->modbusSet.baud));
        ui->dataBitsCombox->setCurrentText(QString::number(modbusthread->modbusSet.dataBits));
        ui->stopBitsCombox->setCurrentText(QString::number(modbusthread->modbusSet.stopBits));
        ui->connectTypeCombox->setCurrentIndex(modbusthread->modbusSet.connectType);
        ui->portEdit->setText(modbusthread->modbusSet.portText);
        ui->serverSpinBox->setValue(modbusthread->modbusSet.server);
        ui->timeoutSpinBox->setValue(modbusthread->modbusSet.responseTime);
        ui->retriesSpinBox->setValue(modbusthread->modbusSet.numberOfRetries);

        ui->beginAddressSpinBox->setValue(modbusthread->modbusSet.beginAddress);
        ui->valueNumSpinBox->setValue(modbusthread->modbusSet.valueNum);
        ui->registerTypeCombox->setCurrentIndex(ui->registerTypeCombox->findData(modbusthread->modbusSet.registerType));
    }
}

void MainWindow::initButtonSignal()
{
    connect(ui->disconnectButton, &QPushButton::clicked, [this]() {
        if (modbusthread->modbusDevice) {
            triggerCmd(ModbusThread::disConnectModbus);
            ui->connectButton->setEnabled(true);
            ui->param_groupBox->setEnabled(true);
            ui->readButton->setEnabled(true);
            ui->registerTypeCombox->setEnabled(true);
            ui->writeButton->setEnabled(true);
            modbusthread->cmd = ModbusThread::Free;
        }
    });
    connect(ui->connectButton, &QPushButton::clicked, [this]() {
        modbusthread->modbusSet.parity = ui->parityCombox->currentIndex();
        modbusthread->modbusSet.baud = ui->baudCombox->currentText().toInt();
        modbusthread->modbusSet.dataBits = ui->dataBitsCombox->currentText().toInt();
        modbusthread->modbusSet.stopBits = ui->stopBitsCombox->currentText().toInt();
        modbusthread->modbusSet.responseTime = ui->timeoutSpinBox->value();
        modbusthread->modbusSet.numberOfRetries = ui->retriesSpinBox->value();
        modbusthread->modbusSet.portText = ui->portEdit->text();
        modbusthread->modbusSet.connectType = static_cast<ModbusThread::ModbusConnection> (ui->connectTypeCombox->currentIndex());
        triggerCmd(ModbusThread::connectModbus);
    });

    connect(ui->readButton, &QPushButton::clicked, [this]() {
        if (!judgeCmd()) return;
        triggerCmd(ModbusThread::ReadModbus);
    });
    connect(ui->writeButton, &QPushButton::clicked, [this]() {
        if (!judgeCmd()) return;
        modbusthread->writeUnit = QModbusDataUnit(static_cast<QModbusDataUnit::RegisterType>(ui->registerTypeCombox->currentData().toInt()), ui->beginAddressSpinBox->value(), ui->valueNumSpinBox->value());
        QModbusDataUnit::RegisterType table = modbusthread->writeUnit.registerType();
        for (uint i = 0; i < modbusthread->writeUnit.valueCount(); i++) {
            if (table == QModbusDataUnit::Coils)
                modbusthread->writeUnit.setValue(i, writeModel->m_coils[i + modbusthread->writeUnit.startAddress()]);
            /*else if (table == QModbusDataUnit::DiscreteInputs)
            writeUnit.setValue(i, writeModel->m_discreteRegisters[i + writeUnit.startAddress()]);
            else if (table == QModbusDataUnit::InputRegisters)
            writeUnit.setValue(i, writeModel->m_InputRegisters[i + writeUnit.startAddress()]);*/
            else if (table == QModbusDataUnit::HoldingRegisters)
                modbusthread->writeUnit.setValue(i, writeModel->m_holdingRegisters[i + modbusthread->writeUnit.startAddress()]);
        }
        triggerCmd(ModbusThread::WriteModbus);
    });
}

bool MainWindow::judgeCmd()
{
    if (!modbusthread->modbusDevice) {
        ui->label_statusShow->setText(tr("未建立连接对象，操作失败!"));
        timer->start(secondsNum);
        return false;
    }
    return true;
}
