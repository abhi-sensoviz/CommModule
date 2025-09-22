#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){


    ui->setupUi(this);
    plc=new PlcComm();
    connect(plc,&PlcComm::dataRecived,this,&MainWindow::on_dataRecived);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnSendData_clicked(){

    QString data=ui->inputText->text();
    plc->sendData(data.toUtf8());

}


void MainWindow::on_dataRecived(QByteArray data){
    ui->textBox->append(QString(data));

}


void MainWindow::on_btnOpenPort_clicked(){
    QString port= ui->portname->text();
    if(ui->comboBox->currentText()==QString("Serial"))
    plc->connectDevice(PlcComm::SerialType::SERIAL,port.toStdString());

    if(ui->comboBox->currentText()==QString("TCP"))
    plc->connectDevice(PlcComm::TcpType::TCP,port.toStdString());

    if(ui->comboBox->currentText()==QString("Modbus RTU"))
    plc->connectDevice(PlcComm::ModbusType::MODBUS,PlcComm::ModbusRtu::RTU,port.toStdString());

    if(ui->comboBox->currentText()==QString("Modbus TCP"))
    plc->connectDevice(PlcComm::ModbusType::MODBUS,PlcComm::ModbusTcp::TCP,port.toStdString());



}


void MainWindow::on_btnClose_clicked()
{

    plc->disconnectDevice();
}

void MainWindow::on_btnReq_clicked()
{
//   plc->readModbusData(PlcComm::RegisterType::InputRegisters,0,5,1);

     plc->readModbusData(PlcComm::RegisterType::HoldingRegisters, 0, 5, 1);
}
