#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

//    ui->setupUi(this);
//    serial= new Serial(this);//serial port
//    connect(serial,&Serial::dataRecived,this,&MainWindow::on_dataRecived);
//    socket=new Tcp(this);//tcp port
//    connect(socket,&Tcp::dataRecived,this,&MainWindow::on_dataRecived);
//    loadPort();



    ui->setupUi(this);
    plc=new PlcComm();
    connect(plc,&PlcComm::dataRecived,this,&MainWindow::on_dataRecived);
    loadPort();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnSendData_clicked(){
    QString data=ui->inputText->text();
//    serial->sendData(data.toUtf8());//send to serial
//    socket->sendData(data.toUtf8());//send to tcp
    plc->sendData(data.toUtf8());
}

void MainWindow::loadPort(){
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

//    if(ui->comboBox->currentText()==QString("Modbus"))
//    plc->connectDevice(PlcComm::ModbusType::MODBUS);


}


void MainWindow::on_btnClose_clicked()
{

    plc->disconnectDevice();
}
