#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QDataStream>
#include <QListWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_Save_clicked();

    void on_comboBox_ModelFile_currentIndexChanged(const QString &arg1);

    void on_actionWrite_triggered();

    void on_listWidget_ConfigList_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    void init_comboBox_ModelFile();
    void init_comboBox_SerialPort();
    void init_tableWidget_arg();
    void init_listWidget_ConfigList();

    const QString ModelFilePath = "./ModelFiles";
    const QString RegularExpression = "(?<={)\\S*?(?=})";
    const QString ConfigFileName = "config.ini";
};

#endif // MAINWINDOW_H
