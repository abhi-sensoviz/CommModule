#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    serial= new Serial(this);
    connect(serial,&Serial::dataRecived,this,&MainWindow::on_dataRecived);
    loadPort();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnSendData_clicked(){
    QString data=ui->inputText->text();
    serial->sendData(data.toUtf8());
}

void MainWindow::loadPort(){
    for(size_t i=0;i<PortList.size();i++){
       ui->comboBox->addItem(QString::fromStdString( PortList[i]) );
    }
}

void MainWindow::on_dataRecived(QByteArray data){
    ui->textBox->append(QString(data));

}


void MainWindow::on_btnOpenPort_clicked(){
    QString port=ui->comboBox->currentText();
    serial->connectDevice(port);
}


void MainWindow::on_btnClose_clicked()
{
    serial->disconnectPort();
}
