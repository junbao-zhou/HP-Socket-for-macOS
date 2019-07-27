#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_tcpClient(&m_clientListener)
{
    ui->setupUi(this);

    QTimer* pTimer = new QTimer();

    connect(pTimer, &QTimer::timeout, this, &MainWindow::onTimer);
    pTimer->start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString strPort = ui->lineEdit_2->text();
    bool bStatus = false;

    QString ipAddress = ui->lineEdit->text();
    if(ipAddress.isEmpty()){
        ipAddress = QString::fromLocal8Bit("0.0.0.0");
        ui->listWidget->addItem(QString::fromLocal8Bit("转换失败，使用默认IP地址。"));
    }

    unsigned short usPort = strPort.toUShort(&bStatus);
    if(!bStatus){
        usPort = 8888;
        ui->listWidget->addItem(QString::fromLocal8Bit("转换失败，使用默认端口号。"));
    }

    if(!m_tcpClient->Start(ipAddress.toStdString().c_str(), usPort)){
        ui->listWidget->addItem(QString::fromLocal8Bit("连接失败。"));
        ui->listWidget->addItem(QString::fromLocal8Bit(m_tcpClient->GetLastErrorDesc()));
        return;
    }

    ui->listWidget->addItem(QString::fromLocal8Bit("连接完成..."));
}

void MainWindow::on_pushButton_2_clicked()
{
    if(!m_tcpClient->Stop()){
        ui->listWidget->addItem(QString::fromLocal8Bit("关闭连接失败。"));
        ui->listWidget->addItem(QString::fromLocal8Bit(m_tcpClient->GetLastErrorDesc()));
        return;
    }

    ui->listWidget->addItem(QString::fromLocal8Bit("已停止..."));
}

void MainWindow::on_pushButton_3_clicked()
{
    //textBrowser
    if(m_tcpClient->IsConnected()){
        QString sendText = ui->textEdit->toPlainText();
        std::string stdStrText = sendText.toStdString();
        if(!m_tcpClient->Send((const BYTE*)stdStrText.c_str(), stdStrText.size())){
            ui->listWidget->addItem(QString::fromLocal8Bit("发送失败。"));
            return;
        }
    }else{
        ui->listWidget->addItem(QString::fromLocal8Bit("无法发送。"));
    }
}


void MainWindow::onTimer()
{
    std::list<QString> msg = m_clientListener.getExterMsg();

    if(!msg.empty()){
        ui->listWidget->addItems(QStringList::fromStdList(msg));
    }
}
