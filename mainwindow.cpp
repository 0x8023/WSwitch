#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDesktopServices>

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
/*
    //STL 风格迭代器
    QList<QFileInfo> ModFileList = ModDir.entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable);
    QMutableListIterator<QFileInfo> ModFileIter(ModFileList);
    while(ModFileIter.hasNext()){
        QFileInfo Filter = ModFileIter.next();
        if(Filter.suffix().compare("mod", Qt::CaseInsensitive) == 0){
            ui->comboBox_ModelFile->addItem(Filter.fileName());
        }
    }

    //Qt 风格迭代器
    foreach(QFileInfo ModFileInfo, ModDir.entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable)){
        if(ModFileInfo.suffix().compare("mod", Qt::CaseInsensitive) == 0){
            ui->comboBox_ModelFile->addItem(ModFileInfo.fileName());
        }
    }
*/
    //C++17 风格迭代器
    for(const QFileInfo& ModFileInfo : ModDir.entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable)){
        if(ModFileInfo.suffix().compare(ModelSuffix, Qt::CaseInsensitive) == 0){
            ui->comboBox_ModelFile->addItem(ModFileInfo.fileName());
            ui->comboBox_AfterModelFile->addItem(ModFileInfo.fileName());
            ui->comboBox_BeforeModelFile->addItem(ModFileInfo.fileName());
        }
    }

    //触发选择框变更事件
    emit ui->comboBox_ModelFile->currentIndexChanged(ui->comboBox_ModelFile->currentText());
}

void MainWindow::init_comboBox_SerialPort(){
    //扫描串口显示到选择框
/*
    //STL 风格迭代器
    QList<QSerialPortInfo> SerialPortList = QSerialPortInfo::availablePorts();
    QMutableListIterator<QSerialPortInfo> SerialPortIterator(SerialPortList);
    while(SerialPortIterator.hasNext()){
        ui->comboBox_SerialPort->addItem(SerialPortIterator.next().portName());
    }

    //Qt 风格迭代器
    foreach(QSerialPortInfo SerialPortList, QSerialPortInfo::availablePorts()){
        ui->comboBox_SerialPort->addItem(SerialPortList.portName());
    }
*/
    //C++17 风格迭代器
    for(const QSerialPortInfo& SerialPortList : QSerialPortInfo::availablePorts()){
        ui->comboBox_SerialPort->addItem(SerialPortList.portName());
    }
}

void MainWindow::init_tableWidget_arg(){
    ui->tableWidget_arg->setEditTriggers(QAbstractItemView::AllEditTriggers); //禁用横向滚动条
    ui->tableWidget_arg->setColumnWidth(0 ,150); //设置第一列列宽为150
}

void MainWindow::init_listWidget_ConfigList(){
    //读取配置文件
    QSettings ConfigFile(ConfigFileName, QSettings::IniFormat);
    ui->listWidget_ConfigList->addItems(ConfigFile.childGroups());
}

void MainWindow::on_comboBox_ModelFile_currentIndexChanged(const QString &arg1){
    //清空表格
    ui->tableWidget_arg->clear();

    //读取模板文件
    QFile ModFile(ModelFilePath + "/" + arg1);
    if (!ModFile.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this, "错误!", "打开模板文件失败.");
        return;
    }

    //正则过滤参数, 集合去重
    QRegularExpression re(RegularExpression);
    QRegularExpressionMatchIterator remi = re.globalMatch(ModFile.readAll());
    QSet<QString> ParameterSet;
    while(remi.hasNext()){
        QRegularExpressionMatch match = remi.next();
        ParameterSet << match.captured(0);
    }

    //循环写入表格
    ui->tableWidget_arg->setRowCount(2);
    ui->tableWidget_arg->setRowCount(ParameterSet.size());
    quint16 con = 0;
/*
    //Qt 风格迭代器
    foreach(QString Parameter, ParameterSet){
        ui->tableWidget_arg->setItem(con, 0, new QTableWidgetItem(Parameter));
        ui->tableWidget_arg->item(con, 0)->setFlags(Qt::NoItemFlags);
        con++;
    }
*/
    //C++17 风格迭代器
    for(const QString& Parameter : ParameterSet){
        ui->tableWidget_arg->setItem(con, 0, new QTableWidgetItem(Parameter));
        ui->tableWidget_arg->setItem(con, 1, new QTableWidgetItem(""));
        ui->tableWidget_arg->item(con, 0)->setFlags(Qt::NoItemFlags);
        con++;
    }

    //关闭文件
    ModFile.close();
}

void MainWindow::on_actionWrite_triggered(){
    //打开文件
    QFile ModFile(ModelFilePath + "/" + ui->comboBox_ModelFile->currentText());
    if (!ModFile.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this, "错误!", "打开模板文件失败.");
        return;
    }

    //读取/关闭文件并替换参数
    QString ModText = ModFile.readAll();
    ModFile.close();
    for(qint16 con = ui->tableWidget_arg->rowCount() - 1; con >= 0; con--){
        ModText.replace(QRegularExpression("{" + ui->tableWidget_arg->item(con,0)->text() + "}"), ui->tableWidget_arg->item(con,1)->text());
    }

    for(const QString& Command : ModText.split("\r\n")){
        switch(Command.mid(1, 1).constData()->toLatin1()){
            case 'D':
                break;
            case 'E':
                break;
            case 'S':
                break;
            default:
                break;
        }
    }

    for(const QString& Command : ModText.split("\r\n")){
        qDebug()<<Command;
    }

    for(const QString& Command : ModText.split("\r\n")){
        qDebug()<<Command;
    }

}

void MainWindow::on_pushButton_Save_clicked(){
    //保存配置
    QSettings ConfigFile(ConfigFileName, QSettings::IniFormat);
    ConfigFile.beginGroup(ui->lineEdit_ModName->text());
    ConfigFile.setValue("SerialPort", ui->comboBox_SerialPort->currentText());
    ConfigFile.setValue("Baud", ui->comboBox_Baud->currentText());
    ConfigFile.setValue("Interval", ui->lineEdit_Interval->text());
    ConfigFile.setValue("Quantity", ui->lineEdit_Quantity->text());
    ConfigFile.setValue("Blank", ui->checkBox_Blank->isChecked());

    ConfigFile.setValue("BeforeModelFile", ui->comboBox_BeforeModelFile->currentText());
    ConfigFile.setValue("ModelFile", ui->comboBox_ModelFile->currentText());
    ConfigFile.setValue("AfterModelFile", ui->comboBox_AfterModelFile->currentText());

    ConfigFile.endGroup();
}

void MainWindow::on_listWidget_ConfigList_itemDoubleClicked(QListWidgetItem *item){
    //载入配置
    QSettings ConfigFile(ConfigFileName, QSettings::IniFormat);
    ui->lineEdit_ModName->setText(item->text());
    ConfigFile.beginGroup(item->text());
    ui->comboBox_SerialPort->setCurrentIndex(ui->comboBox_SerialPort->findText(ConfigFile.value("SerialPort").toString()));
    ui->comboBox_Baud->setCurrentText(ConfigFile.value("Baud").toString());
    ui->lineEdit_Interval->setText(ConfigFile.value("Interval").toString());
    ui->lineEdit_Quantity->setText(ConfigFile.value("Quantity").toString());
    ui->checkBox_Blank->setChecked(ConfigFile.value("Blank").toBool());

    ui->comboBox_BeforeModelFile->setCurrentIndex(ui->comboBox_BeforeModelFile->findText(ConfigFile.value("BeforeModelFile").toString()));
    ui->comboBox_ModelFile->setCurrentIndex(ui->comboBox_ModelFile->findText(ConfigFile.value("ModelFile").toString()));
    ui->comboBox_AfterModelFile->setCurrentIndex(ui->comboBox_AfterModelFile->findText(ConfigFile.value("AfterModelFile").toString()));

    ConfigFile.endGroup();
}

void MainWindow::on_pushButton_BeforeModelFile_clicked(){
    QDesktopServices::openUrl(QUrl::fromLocalFile(ModelFilePath + "/" + ui->comboBox_BeforeModelFile->currentText()));
}

void MainWindow::on_pushButton_ModelFile_clicked(){
    QDesktopServices::openUrl(QUrl::fromLocalFile(ModelFilePath + "/" + ui->comboBox_ModelFile->currentText()));
}

void MainWindow::on_pushButton_AfterModelFile_clicked(){
    QDesktopServices::openUrl(QUrl::fromLocalFile(ModelFilePath + "/" + ui->comboBox_AfterModelFile->currentText()));
}

