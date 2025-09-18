#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <iostream>
#include <strings.h>
#include "serial.h"
using namespace std;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadPort();
    array<string, 2> PortList = { "/dev/pts/2", "/dev/pts/3" };
    Serial *serial=nullptr;

private slots:
    void on_btnSendData_clicked();
    void on_dataRecived(QByteArray data);

    void on_btnOpenPort_clicked();

    void on_btnClose_clicked();


private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
