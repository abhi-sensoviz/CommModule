#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){


    ui->setupUi(this);
//    plc=new PlcComm();
//    connect(plc,&PlcComm::dataReady,this,&MainWindow::on_dataRecived);

    serial=new Serial();
    connect(serial,&Serial::dataReady,this,&MainWindow::on_dataRecived);
    tcp=new Tcp();
    connect(tcp,&Tcp::dataReady,this,&MainWindow::on_dataRecived);
    modbus=new Modbus();
    connect(modbus,&Modbus::dataReady,this,&MainWindow::on_dataRecived);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnSendData_clicked(){

    QString data=ui->inputText->text();
    QString regType=ui->regType->currentText();

    if(ui->comboBox->currentText()==QString("Serial"))
        serial->sendData(data.toUtf8());
    if(ui->comboBox->currentText()==QString("TCP"))
        tcp->sendData(data.toUtf8());
    else if(ui->comboBox->currentText()==QString("Modbus RTU") ||ui->comboBox->currentText()==QString("Modbus TCP")){
        if(regType==QString("Coil (r/w)"))
            modbus->sendData(data.toUtf8(),Modbus::RegisterType::Coils,ui->strtAddr->text().toInt());
        if(regType==QString("Holding Register (r/w)"))
            modbus->sendData(data.toUtf8(),Modbus::RegisterType::HoldingRegisters,ui->strtAddr->text().toInt());}


}


void MainWindow::on_dataRecived(QByteArray data){
    ui->textBox->append(">> "+QString(data));

}


void MainWindow::on_btnOpenPort_clicked(){
    QString port= ui->portname->text();
    int ipport=ui->portNo->text().toInt();
    if(ui->comboBox->currentText()==QString("Serial"))
    serial->connectDevice(port.toStdString());

    if(ui->comboBox->currentText()==QString("TCP"))
    tcp->connectDevice(port.toStdString(),ipport);

    if(ui->comboBox->currentText()==QString("Modbus RTU"))
    modbus->connectDevice(Modbus::ModbusRtu::RTU,port.toStdString(),ipport);

    if(ui->comboBox->currentText()==QString("Modbus TCP"))
    modbus->connectDevice(Modbus::ModbusTcp::TCP,port.toStdString(),ipport);



}


void MainWindow::on_btnClose_clicked()
{
    QString port= ui->portname->text();

    if(ui->comboBox->currentText()==QString("Serial"))
    serial->disconnectDevice();
    if(ui->comboBox->currentText()==QString("TCP"))
    tcp->disconnectDevice();
    if(ui->comboBox->currentText()==QString("Modbus RTU") ||ui->comboBox->currentText()==QString("Modbus TCP"))
    modbus->disconnectDevice();


}

void MainWindow::on_btnReq_clicked()
{
    QString regType=ui->regType->currentText();
    if(regType==QString("Coil (r/w)"))
        modbus->readModbusData(Modbus::RegisterType::Coils,ui->strtAddr->text().toInt(),5,1);
    if(regType==QString("Holding Register (r/w)"))
        modbus->readModbusData(Modbus::RegisterType::HoldingRegisters,ui->strtAddr->text().toInt(),1,1);
    if(regType==QString("Discreet Inputs (read only)"))
        modbus->readModbusData(Modbus::RegisterType::DiscreteInputs,ui->strtAddr->text().toInt(),1,1);
    if(regType==QString("Input Register (read only)"))
        modbus->readModbusData(Modbus::RegisterType::InputRegisters,ui->strtAddr->text().toInt(),1,1);
}









