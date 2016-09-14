#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QThread>


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

    //分割文件
    QStringList ModList = ModText.split(Commas);

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

    //解析第一部分
    if (!ModList[0].isEmpty()){
        for (const QString& Command : ModList[0].split("\r\n")){
            if (Command.startsWith("%")){
                switch (Command.mid(1, 1).constData()->toLatin1()){
                    case 'D': //延时
                        delay(Command.mid(3).toInt());
                        break;
                    case 'E': //回车
                        serialport.write("\r\n");
                        break;
                    case 'S': //发送
                        serialport.write(Command.mid(3).toLatin1() + "\r\n");
                        break;
                    default:
                        break;
                }
            }else if(!Command.isEmpty()){
                serialport.write(Command.toLatin1() + "\r\n");
            }
        }
    }

    //解析第二部分
    if (!ModList[1].isEmpty()){
        quint32 con = 0;
        QString data = "";
        for (const QString& Command : ModList[1].split("\r\n")){
            if (++con > ui->lineEdit_Quantity->text().toUInt()){
                serialport.write(data.toLatin1());
                data.clear();
                if (ui->checkBox_Blank->isChecked()){
                    serialport.write("\r\n");
                }
                delay(ui->lineEdit_Interval->text().toUInt());
                con = 0;
            }else{
                data += Command + "\r\n";
            }
        }
        if (con != 0){
            serialport.write(data.toLatin1());
            data.clear();
            if (ui->checkBox_Blank->isChecked()){
                serialport.write("\r\n");
            }
            delay(ui->lineEdit_Interval->text().toUInt());
            con = 0;
        }
    }

    //解析第三部分
    if (!ModList[2].isEmpty()){
        for (const QString& Command : ModList[2].split("\r\n")){
            if (Command.startsWith("%")){
                switch (Command.mid(1, 1).constData()->toLatin1()){
                    case 'D': //延时
                        delay(Command.mid(3).toInt());
                        break;
                    case 'E': //回车
                        serialport.write("\r\n");
                        break;
                    case 'S': //发送
                        serialport.write(Command.mid(3).toLatin1() + "\r\n");
                        break;
                    default:
                        break;
                }
            }else if(!Command.isEmpty()){
                serialport.write(Command.toLatin1() + "\r\n");
            }
        }
    }
    delay(256);
    serialport.close();
    QMessageBox::information(this, "成功!", "写入完成.");
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
    ConfigFile.setValue("ModelFile", ui->comboBox_ModelFile->currentText());
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
    ui->comboBox_ModelFile->setCurrentIndex(ui->comboBox_ModelFile->findText(ConfigFile.value("ModelFile").toString()));
    ConfigFile.endGroup();
}

void MainWindow::on_pushButton_ModelFile_clicked(){
    QDesktopServices::openUrl(QUrl::fromLocalFile(ModelFilePath + "/" + ui->comboBox_ModelFile->currentText()));
}

quint32 MainWindow::get_estimated_time(QString ModText){
    quint32 runtime = 0;
    quint16 con = 0;
    for (const QString& Command : ModText.split("\r\n")){
        if (Command.startsWith("%")){
            switch(Command.mid(1, 1).constData()->toLatin1()){
                case 'D': //延时
                    runtime += Command.mid(3).toInt() + 10;
                    break;
                default:
                    runtime += 10;
                    break;
            }
        }else {
            if (++con >= ui->lineEdit_Quantity->text().toInt()){
                runtime += ui->lineEdit_Interval->text().toInt() + 10;
                con = 0;
            }
        }
    }
    return runtime + ui->lineEdit_Interval->text().toInt() + 1256;
}

void MainWindow::on_checkBox_ShowLog_toggled(bool checked){
    //隐藏/显示详细讯息
    ui->textEdit_Log->setVisible(checked);
}

void MainWindow::delay(int msec){
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while(QTime::currentTime() < dieTime){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

void MainWindow::on_pushButton_clicked()
{
    QSerialPort *serialport = new QSerialPort();
    serialport->setPortName("COM3"); //串口号
    serialport->setBaudRate(QSerialPort::Baud9600); //波特率
    serialport->setDataBits(QSerialPort::Data8); //数据位
    serialport->setParity(QSerialPort::NoParity); //校验位
    serialport->setStopBits(QSerialPort::OneStop); //停止位
    serialport->setFlowControl(QSerialPort::NoFlowControl); //流控
    if(!serialport->open(QIODevice::WriteOnly)){
        QMessageBox::critical(this, "错误!", "打开串口失败.");
        return;
    }

    serialport->write("233333\r\n");
    delay(2000);
    serialport->close();
}
