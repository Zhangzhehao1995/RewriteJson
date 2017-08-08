#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextBlock>
#include <QWidget>
#include <QHeaderView>
#include <fstream>
#include <string>
#include "dialog.h"
#include <QDebug>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    Dialog *new_Dialog;

private slots:
    void on_actionOpen_clicked();

    void on_treeJsonFile_itemSelectionChanged();

    void on_actionSave_clicked();

    void showTableWidget();

    void doSearching();

    void on_FindNext_clicked();

    void on_FindPrevious_clicked();

    void on_actionSetChange_clicked();

    void on_actionAdd_clicked();

    void on_actionDelete_clicked();

    void showtheChange(int framenum, int callnum, QJsonObject addItemObject, QString addText);



private:
    Ui::MainWindow *ui;
    QStringList resave_string;
    std::vector<int> frame_start;
    std::vector<int> frame_end;

    bool JsonOpened;

    QString strTemplate;
    QList<QTreeWidgetItem *> list;
    int findnum;
    int nCount;

    QTreeWidgetItem *newitemlocation;
};

#endif // MAINWINDOW_H
