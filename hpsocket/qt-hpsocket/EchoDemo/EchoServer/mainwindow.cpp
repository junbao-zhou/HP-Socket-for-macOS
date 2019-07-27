#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QStringList>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_tcpServer(&m_serverListener)
{
    using namespace std;
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
    QString strPort = ui->lineEdit->text();
    bool bStatus = false;

    QString ipAddress = ui->lineEdit_2->text();
    if(ipAddress.isEmpty()){
        ipAddress = QString::fromLocal8Bit("0.0.0.0");
        ui->listWidget->addItem(QString::fromLocal8Bit("转换失败，使用默认IP地址。"));
    }

    unsigned short usPort = strPort.toUShort(&bStatus);
    if(!bStatus){
        usPort = 8888;
        ui->listWidget->addItem(QString::fromLocal8Bit("转换失败，使用默认端口号。"));
    }

    if(!m_tcpServer->Start(ipAddress.toStdString().c_str(), usPort)){
        ui->listWidget->addItem(QString::fromLocal8Bit("开启监听失败。"));
        ui->listWidget->addItem(QString::fromLocal8Bit(m_tcpServer->GetLastErrorDesc()));
        return;
    }

    ui->listWidget->addItem(QString::fromLocal8Bit("监听中..."));
}

void MainWindow::on_pushButton_2_clicked()
{
    if(!m_tcpServer->Stop()){
        ui->listWidget->addItem(QString::fromLocal8Bit("关闭监听失败。"));
        ui->listWidget->addItem(QString::fromLocal8Bit(m_tcpServer->GetLastErrorDesc()));
        return;
    }

    ui->listWidget->addItem(QString::fromLocal8Bit("已停止..."));
}

void MainWindow::onTimer()
{
    std::list<QString> msg = m_serverListener.getExterMsg();

    if(!msg.empty()){
        ui->listWidget->addItems(QStringList::fromStdList(msg));
    }
}
