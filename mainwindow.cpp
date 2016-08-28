#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init_tableWidget_arg();
    init_comboBox_ModelFile();
    init_comboBox_SerialPort();
    init_listWidget_ConfigList();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init_comboBox_ModelFile(){
    //扫描文件显示到选择框
    QDir ModDir(ModelFilePath);
    foreach(QFileInfo ModFileInfo, ModDir.entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable)){
        if(ModFileInfo.suffix().compare("mod", Qt::CaseInsensitive) == 0){
            this->ui->comboBox_ModelFile->addItem(ModFileInfo.fileName());
        }
    }

    //触发选择框变更事件
    emit this->ui->comboBox_ModelFile->currentIndexChanged(this->ui->comboBox_ModelFile->currentText());
}

void MainWindow::init_comboBox_SerialPort(){
    //扫描串口显示到选择框
    foreach(QSerialPortInfo SerialPortList, QSerialPortInfo::availablePorts()){
        this->ui->comboBox_SerialPort->addItem(SerialPortList.portName());
    }
}

void MainWindow::init_tableWidget_arg(){
    this->ui->tableWidget_arg->setEditTriggers(QAbstractItemView::AllEditTriggers); //禁用横向滚动条
    this->ui->tableWidget_arg->setColumnWidth(0 ,150); //设置第一列列宽为150
}

void MainWindow::init_listWidget_ConfigList(){
    //打开配置文件
    QFile ConfigFile(ConfigFileName);
    if (!ConfigFile.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this, "严重错误!", "打开配置文件失败!");
        return;
    }

    //正则匹配所有节
    QRegularExpression re("(?<=\\[).*?(?=\\])");
    QRegularExpressionMatchIterator remi = re.globalMatch(ConfigFile.readAll());
    while(remi.hasNext()){
        QRegularExpressionMatch match = remi.next();
        this->ui->listWidget_ConfigList->addItem(match.captured(0));
    }

    //关闭文件
    ConfigFile.close();
}

void MainWindow::on_pushButton_Save_clicked(){

}

void MainWindow::on_comboBox_ModelFile_currentIndexChanged(const QString &arg1){
    //清空表格
    this->ui->tableWidget_arg->clear();

    //读取模板文件
    QFile ModFile(ModelFilePath + "/" + arg1);
    if (!ModFile.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this, "严重错误!", "打开模板文件失败!");
        return;
    }

    //正则过滤参数, 集合去重
    QRegularExpression re(RegularExpression);
    QRegularExpressionMatchIterator remi = re.globalMatch(ModFile.readAll());
    QSet<QString> ParameterSet;
    while(remi.hasNext()){
        QRegularExpressionMatch match = remi.next();
        ParameterSet << match.captured(0);;
    }

    //循环写入表格
    this->ui->tableWidget_arg->setRowCount(ParameterSet.size());
    quint8 con = 0;
    foreach(QString Parameter, ParameterSet){
        this->ui->tableWidget_arg->setItem(con, 0, new QTableWidgetItem(Parameter));
        this->ui->tableWidget_arg->item(con, 0)->setFlags(Qt::NoItemFlags);
        con++;
    }

    //关闭文件
    ModFile.close();
}

void MainWindow::on_actionWrite_triggered(){

}

void MainWindow::on_listWidget_ConfigList_itemDoubleClicked(QListWidgetItem *item)
{
    qDebug()<<item->text();
}
