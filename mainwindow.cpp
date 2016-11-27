#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "QDebug"
#include "QTextCodec"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init_tableWidget_arg();
    init_comboBox_ModelFile();
    init_comboBox_SerialPort();
    init_listWidget_ConfigList();

    connect(Timer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
}

MainWindow::~MainWindow()
{
    delete RunTime;
    delete Timer;
    delete ui;
}

void MainWindow::init_comboBox_ModelFile(){
    //扫描文件显示到选择框
    QDir ModDir(ModelFilePath);
    for(const QFileInfo& ModFileInfo : ModDir.entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable)){
        if(ModFileInfo.suffix().compare(ModelSuffix, Qt::CaseInsensitive) == 0){
            ui->comboBox_ModelFile->addItem(ModFileInfo.fileName());
        }
    }

    //触发选择框变更事件
    emit ui->comboBox_ModelFile->currentIndexChanged(ui->comboBox_ModelFile->currentText());
}

void MainWindow::init_comboBox_SerialPort(){
    //扫描串口显示到选择框
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

    //正则过滤参数, 有序列表去重
    QRegularExpression re(RegularExpression);
    QRegularExpressionMatchIterator remi = re.globalMatch(ModFile.readAll());
    QStringList ParameterSet;
    while(remi.hasNext()){
        QRegularExpressionMatch match = remi.next();
        if (!ParameterSet.contains(match.captured(0))){
            ParameterSet << match.captured(0);
        }
    }

    //循环写入表格
    ui->tableWidget_arg->setRowCount(ParameterSet.size());
    quint16 con = 0;
    for (const QString& Parameter : ParameterSet){
        ui->tableWidget_arg->setItem(con, 0, new QTableWidgetItem(Parameter));
        ui->tableWidget_arg->setItem(con, 1, new QTableWidgetItem(""));
        ui->tableWidget_arg->item(con, 0)->setFlags(Qt::NoItemFlags);
        con++;
    }

    //关闭文件
    ModFile.close();
}

void MainWindow::on_actionWrite_triggered(){
    //禁用手动模式
    ui->lineEdit_Command->setEnabled(false);

    //清空log
    ui->textEdit_Log->clear();

    //读取文件内容
    QFile ModFile(ModelFilePath + "/" + ui->comboBox_ModelFile->currentText());
    if (!ModFile.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this, "错误!", "打开正文模板文件失败.");
        return;
    }
    QString ModText = ModFile.readAll();
    ModFile.close();

    //替换参数
    for (qint16 con = ui->tableWidget_arg->rowCount() - 1; con >= 0; con--){
        ModText.replace(QRegularExpression("{" + ui->tableWidget_arg->item(con,0)->text() + "}"), ui->tableWidget_arg->item(con,1)->text());
    }

    //预计时间/设置滚动条
    RunTime->setValue(0);
    RunTime->show();
    ui->statusBar->addWidget(RunTime);
    Timer->start(get_estimated_time(ModText));

    //初始化串口
    QSerialPort serialport;
    serialport.setPortName(ui->comboBox_SerialPort->currentText()); //串口号
    serialport.setBaudRate(ui->comboBox_Baud->currentText().toInt()); //波特率
    serialport.setDataBits(QSerialPort::Data8); //数据位
    serialport.setParity(QSerialPort::NoParity); //校验位
    serialport.setStopBits(QSerialPort::OneStop); //停止位
    serialport.setFlowControl(QSerialPort::NoFlowControl); //流控
    if(!serialport.open(QIODevice::ReadWrite)){
        QMessageBox::critical(this, "错误!", "打开串口失败.");
        return;
    }

    //解析/发送内容
    if (!ModText.isEmpty()){
        QString data = "";
        for (const QString& Command : ModText.split("\r\n")){
            if (!Command.startsWith("%")){ //不是命令
                if(!Command.isEmpty()){
                    data += Command + "\r\n";
                }
            }else if (Command.startsWith("%Delay")){ //延时
                delay(Command.mid(7).toInt());
            }else if (Command.startsWith("%Enter")){ //回车
                serialport.write("\r\n");
                delay(10);
                ui->textEdit_Log->insertPlainText("\r\n");
            }else if (Command.startsWith("%Send")){ //发送单条命令
                serialport.write(data.toLatin1());
                delay((quint32)(data.length() / (ui->comboBox_Baud->currentText().toDouble()/(8000 + 2048))) + 128);
                ui->textEdit_Log->insertPlainText(data);
                data.clear();
            }else{
                ;
            }
        }
    }
    delay(512);
    serialport.close();

    //写入CSV文件
    write_log();

    //关闭进度条/定时器
    Timer->stop();
    RunTime->hide();
    ui->statusBar->removeWidget(RunTime);
    QMessageBox::information(this, "成功!", "写入完成.");

    //启用手动模式
    ui->lineEdit_Command->setEnabled(true);
}

void MainWindow::on_pushButton_Save_clicked(){
    QSettings ConfigFile(ConfigFileName, QSettings::IniFormat);
    //保存配置
    ConfigFile.beginGroup(ui->lineEdit_ModName->text());
    ConfigFile.setValue("SerialPort", ui->comboBox_SerialPort->currentText());
    ConfigFile.setValue("Baud", ui->comboBox_Baud->currentText());
    ConfigFile.setValue("ModelFile", ui->comboBox_ModelFile->currentText());
    ConfigFile.setValue("LogFile", ui->lineEdit_LogFile->text());
    //保存自定义参数
    for (qint16 con = ui->tableWidget_arg->rowCount() - 1; con >= 0; con--){
        ConfigFile.setValue(ui->tableWidget_arg->item(con, 0)->text(), ui->tableWidget_arg->item(con, 1)->text());
    }
    ConfigFile.endGroup();

    //刷新配置列表
    ui->listWidget_ConfigList->clear();
    init_listWidget_ConfigList();
}

void MainWindow::on_pushButton_Delete_clicked(){
    //删除配置
    QSettings ConfigFile(ConfigFileName, QSettings::IniFormat);
    ConfigFile.remove(ui->listWidget_ConfigList->selectedItems()[0]->text());

    //刷新配置列表
    ui->listWidget_ConfigList->clear();
    init_listWidget_ConfigList();
}

void MainWindow::on_listWidget_ConfigList_itemDoubleClicked(QListWidgetItem *item){
    QSettings ConfigFile(ConfigFileName, QSettings::IniFormat);
    //载入配置
    ui->lineEdit_ModName->setText(item->text());
    ConfigFile.beginGroup(item->text());
    ui->comboBox_SerialPort->setCurrentIndex(ui->comboBox_SerialPort->findText(ConfigFile.value("SerialPort").toString()));
    ui->comboBox_Baud->setCurrentIndex(ui->comboBox_Baud->findText(ConfigFile.value("Baud").toString()));
    ui->comboBox_ModelFile->setCurrentIndex(ui->comboBox_ModelFile->findText(ConfigFile.value("ModelFile").toString()));
    ui->lineEdit_LogFile->setText(ConfigFile.value("LogFile").toString());
    ConfigFile.setValue("LogFile", ui->lineEdit_LogFile->text());
    //载入自定义参数
    for (qint16 con = ui->tableWidget_arg->rowCount() - 1; con >= 0; con--){
        ui->tableWidget_arg->setItem(con, 1, new QTableWidgetItem(ConfigFile.value(ui->tableWidget_arg->item(con, 0)->text()).toString()));
    }
    ConfigFile.endGroup();
}

void MainWindow::on_pushButton_ModelFile_clicked(){
    QDesktopServices::openUrl(QUrl::fromLocalFile(ModelFilePath + "/" + ui->comboBox_ModelFile->currentText()));
}

quint32 MainWindow::get_estimated_time(QString ModText){
    quint32 runtime = 0;
    if (!ModText.isEmpty()){
        QString data = "";
        for (const QString& Command : ModText.split("\r\n")){
            if (!Command.startsWith("%")){ //不是命令
                if(!Command.isEmpty()){
                    data += Command + "\r\n";
                }
            }else if (Command.startsWith("%Delay")){ //延时
                runtime += Command.mid(7).toInt();
            }else if (Command.startsWith("%Enter")){ //回车
                runtime += 8;
            }else if (Command.startsWith("%Send")){ //发送命令
                runtime += (quint32)(data.length() / (ui->comboBox_Baud->currentText().toDouble()/(8000 + 2048))) + 128;
            }else{
                ;
            }
        }
    }
    return (quint32)(runtime/100);
}

void MainWindow::on_checkBox_ShowLog_toggled(bool checked){
    //隐藏/显示详细讯息
    ui->textEdit_Log->setVisible(checked);
    ui->lineEdit_Command->setVisible(checked);
}

void MainWindow::delay(int msec){
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while(QTime::currentTime() < dieTime){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

void MainWindow::on_textEdit_Log_textChanged(){
    ui->textEdit_Log->verticalScrollBar()->setSliderPosition(ui->textEdit_Log->verticalScrollBar()->maximum());
}

void MainWindow::timer_timeout(){
    if (RunTime->value() < 100){
        RunTime->setValue(RunTime->value() + 1);
    }else{
        Timer->stop();
    }
}

void MainWindow::on_lineEdit_Command_returnPressed()
{
    //禁用输入
    ui->lineEdit_Command->setEnabled(false);

    //打开串口
    QSerialPort serialport;
    serialport.setPortName(ui->comboBox_SerialPort->currentText()); //串口号
    serialport.setBaudRate(ui->comboBox_Baud->currentText().toInt()); //波特率
    serialport.setDataBits(QSerialPort::Data8); //数据位
    serialport.setParity(QSerialPort::NoParity); //校验位
    serialport.setStopBits(QSerialPort::OneStop); //停止位
    serialport.setFlowControl(QSerialPort::NoFlowControl); //流控
    if(!serialport.open(QIODevice::ReadWrite)){
        QMessageBox::critical(this, "错误!", "打开串口失败.");
        return;
    }

    //发送数据
    QString data = ui->lineEdit_Command->text() + "\r\n";
    serialport.write(data.toLatin1());
    delay(data.length() / (ui->comboBox_Baud->currentText().toDouble()/(8000 + 2048)) + 128);
    ui->textEdit_Log->insertPlainText(data);
    ui->lineEdit_Command->clear();

    //关闭串口
    serialport.close();

    //启用输入
    ui->lineEdit_Command->setEnabled(true);
    ui->lineEdit_Command->setFocus();
}

void MainWindow::write_log(){
    QFile LogFile(ui->lineEdit_LogFile->text() + LogFileSuffix);
    if (!LogFile.open(QIODevice::ReadWrite)){
        QMessageBox::critical(this, "错误!", "编辑输出文件失败.");
        return;
    }
    LogFile.close();

    if (!LogFile.open(QIODevice::Append)){
        QMessageBox::critical(this, "错误!", "编辑输出文件失败.");
        return;
    }
    QString data = ",";
    for (qint16 con = ui->tableWidget_arg->rowCount() - 1; con >= 0; con--){
        data += ui->tableWidget_arg->item(con, 1)->text() + ",";
    }
    QDataStream LogFileStream(&LogFile);
    data += "\n";
    qDebug()<<QTextCodec::codecForName("UTF-8")->fromUnicode(data.toLatin1().data());
    LogFileStream << QTextCodec::codecForName("UTF-8")->fromUnicode(data.toLatin1().data());
    LogFile.close();
}
