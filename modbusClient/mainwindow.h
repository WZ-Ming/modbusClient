#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QCoreApplication>
#include <QSettings>
#include <QTextCodec>
#include <QFile>
#include <QTimer>
#include <QSerialPortInfo>
#include <QNetworkInterface>
#include "writeregistermodel.h"
#include "modbusthread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_valueTypeCombox_currentIndexChanged(const QString &arg);
    void on_registerTypeCombox_currentTextChanged(const QString &arg1);
    void updateValueShowSlot(const ModbusThread::cmdType &cmd);
    void onStateChanged(int state);

private:
    void iniModbusSettings();
    void initButtonSignal();

    void saveModbusSettings();
    void treeViewHideRow();
    bool judgeCmd();

    void triggerCmd(const ModbusThread::cmdType &cmd);

    const QString modbusIniFilePath = QCoreApplication::applicationDirPath() + "/modbusSettings.ini";
    const int secondsNum=5000;

    WriteRegisterModel *writeModel;
    ModbusThread *modbusthread = nullptr;
    Ui::MainWindow *ui;
    QTimer *timer=nullptr;

signals:
    void updateChartSignal(uchar state);
    void emitCmd();

};
#endif // MAINWINDOW_H
