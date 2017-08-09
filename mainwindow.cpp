#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <vector>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_QuitOnClose,true);
    ui->ItemInformation->setRowCount(100);
    ui->ItemInformation->setColumnCount(3);
    ui->ItemInformation->setWordWrap(true);
    ui->ItemInformation->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->ItemInformation->resizeColumnsToContents();
    ui->ItemInformation->horizontalHeader()->setStretchLastSection(true);
    ui->ItemInformation->setSpan(0, 1, 1, 2);//combine cells
    ui->ItemInformation->setSpan(1, 1, 1, 2);
    ui->ItemInformation->setSpan(2, 1, 1, 2);

    this -> setWindowTitle("Json Editor");
    connect(ui->actionNewItem,SIGNAL(clicked()),this,SLOT(showTableWidget()));
    connect(ui->Filter,SIGNAL(returnPressed()),this,SLOT(doSearching()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showTableWidget()
{
    if( JsonOpened != true)
    {
        QMessageBox::warning(this,"Warning","No Opened Json",QMessageBox::Yes);
        return;
    }
    MainWindow::new_Dialog = new Dialog;

    newitemlocation = ui->treeJsonFile->currentItem();
    if(newitemlocation == NULL)
    {
        QMessageBox::warning(this,"Warning","Please choose an insert location",QMessageBox::Yes);
        return;
    }
    int addFrameNum;
    QTreeWidgetItem *parent = newitemlocation->parent();
    int addItemNum;
    if(parent == NULL)
    {
        addFrameNum = ui->treeJsonFile->indexOfTopLevelItem(newitemlocation);
        addItemNum = frame_start[addFrameNum]-1;
    }else{
        addFrameNum = ui->treeJsonFile->indexOfTopLevelItem(parent);
        int frame_start_num = frame_start[addFrameNum];
        int row=parent->indexOfChild(newitemlocation);
        addItemNum = frame_start_num+row;
    }

    newitemlocation->setBackground(0,QColor(0, 255, 0, 60));
    new_Dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    new_Dialog->setAttribute(Qt::WA_DeleteOnClose,true);
    new_Dialog->setAttribute(Qt::WA_QuitOnClose,false);
    new_Dialog->show();
    new_Dialog->initNewTable(addFrameNum,addItemNum);
    connect(new_Dialog,SIGNAL(sendItem(int,int,QJsonObject,QString)),this,SLOT(showtheChange(int,int,QJsonObject,QString)));
}

void MainWindow::on_actionOpen_clicked()
{
    JsonOpened = true;

     QString filedir = QFileDialog::getOpenFileName(this, tr("Open File"),"/home/liamzhang/samurai_data/",tr("Json Files(*.json)"));
     if (filedir.isEmpty())
     {
         QMessageBox::warning(this,"Warning",tr("Fail to get a Json file"),QMessageBox::Yes);
         return;
     }

     std::ifstream fin(filedir.toStdString().c_str());
     if (!fin.is_open())
     {
          QMessageBox::warning(this,"Warnning",tr("Can't open the Json file"),QMessageBox::Yes);
          return;
     }

     if(resave_string.size()>0)
     {
         resave_string.clear();
     }
     std::string s;
     char c;
     bool started = false;
     int curved_bracket_stack = 0;

     while (fin.get(c))
     {
          if (c == '{')
          {
              started = true;
              ++curved_bracket_stack;
          }
          if (started == true)
          {
              s.push_back(c);
              if (c == '}')
              {
                 --curved_bracket_stack;
                 if (curved_bracket_stack == 0)
                 {
                     started = false;
                     QString stdtoQstring = QString::fromStdString(s);
                     s.clear();
                     resave_string.append(stdtoQstring);
                 }
             }
         }
     }

     QJsonValue defaultTid;
     QJsonDocument titleInf = QJsonDocument::fromJson(resave_string[0].toUtf8());
     QJsonObject titleInf_obj = titleInf.object();
     defaultTid = titleInf_obj.value(QString("defaultTid"));
     if(defaultTid.isUndefined())
     {
         QMessageBox::warning(this,"Warning","Lack Json Header",QMessageBox::Yes);
         return;
     }

    int frame_num = 1;
    int turn = 1;
    while(turn< resave_string.size())
    {
        QTreeWidgetItem *item = new QTreeWidgetItem;//add father point
        item->setText(0,QString("frame "+QString::number(frame_num)));
        ui->treeJsonFile->addTopLevelItem(item);
        int temp =turn;
        frame_start.push_back(temp);
        for(int i = temp; i<resave_string.size(); i++)
        {
            turn++;
            QJsonDocument eachFunction = QJsonDocument::fromJson(resave_string[i].toUtf8());
            QJsonObject eachFunction_obj = eachFunction.object();
            QJsonValue tid = eachFunction_obj.value(QString("1 tid"));
            QJsonValue functionname = eachFunction_obj.value(QString("2 func_name"));
            QString functionname_str = functionname.toString();

            if(tid == defaultTid && functionname_str.contains("eglSwapBuffers"))
            {
                frame_num++;
                frame_end.push_back(i);
                break;
            }
        }
    }
    frame_end.push_back(resave_string.size()-1);
    fin.close();
}

void MainWindow::on_actionSave_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),"/home/liamzhang/",tr("Json Files(*.json)"));
    if(resave_string.isEmpty())
    {
        QMessageBox::warning(this,"Warning","Fail to save the Json file",QMessageBox::Yes);
        return;
    }
    QByteArray changed_byte_array;
    changed_byte_array.append("[\xa");
    for(int i = 0; i<resave_string.size()-1; i++)
    {
        changed_byte_array.append(resave_string[i].toUtf8());
        changed_byte_array.push_back(",\xa");
    }
    changed_byte_array.append(resave_string[resave_string.size()-1].toUtf8());
    changed_byte_array.append("\xa]");

    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this,"Warning","Fail to save the Json file",QMessageBox::Yes);
        return;
    }
    file.write(changed_byte_array);
    file.close();
}

void MainWindow::on_treeJsonFile_itemSelectionChanged()
{
    QTreeWidgetItem *item = ui->treeJsonFile->currentItem();
    QTreeWidgetItem *parent = item->parent();

    if(parent == NULL)
    {
        int childrencount = item->childCount();
        for(int i =0; i< childrencount; i++)
        {
            QTreeWidgetItem *child = item->child(0);
            delete child;
        }
        int num = ui->treeJsonFile->indexOfTopLevelItem(item);
        for(int i = frame_start[num]; i<=frame_end[num]; i++)
        {
            QJsonDocument eachFunction = QJsonDocument::fromJson(resave_string[i].toUtf8());
            QJsonObject eachFunction_obj = eachFunction.object();
            QJsonValue callno = eachFunction_obj.value(QString("0 call_no"));
            QString callnum = QString::number(callno.toInt(),10);
            QJsonValue functionname = eachFunction_obj.value(QString("2 func_name"));
            QString functionname_str = functionname.toString();
            QString abbreviation = "("+callnum+")"+functionname_str;
            QTreeWidgetItem *item1=new QTreeWidgetItem;
            item1->setText(0,abbreviation);
            item->addChild(item1);
        }
    }else{
        ui->ItemInformation->clear();
        int num =ui->treeJsonFile->indexOfTopLevelItem(parent);
        int frame_start_num = frame_start[num];
        int row=parent->indexOfChild(item);//get the row number in father point(start from 0)
        int currentRowNo = frame_start_num+row;

        int currentTableRow = 0;
        QString selectString = resave_string[currentRowNo];
        QJsonDocument selectItem = QJsonDocument::fromJson(selectString.toUtf8());
        QJsonObject selectItemObject = selectItem.object();

        //the 1st
        QJsonValue callno = selectItemObject.value(QString("0 call_no"));
        QString callnum = QString::number(callno.toInt(),10);
        QTableWidgetItem *item0 = new QTableWidgetItem("call_no:");
        item0->setFlags(item0->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow,0,item0);
        ui->ItemInformation->setItem(currentTableRow,1,new QTableWidgetItem(callnum));
        currentTableRow++;

        //the 2nd
        QJsonValue tidno = selectItemObject.value(QString("1 tid"));
        QString tidnum = QString::number(tidno.toInt(),10);
        QTableWidgetItem *item1 = new QTableWidgetItem("tid:");
        item1->setFlags(item1->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow,0,item1);
        ui->ItemInformation->setItem(currentTableRow,1,new QTableWidgetItem(tidnum));
        currentTableRow++;

        //the 3rd
        QJsonValue functionname = selectItemObject.value(QString("2 func_name"));
        QTableWidgetItem *item2 = new QTableWidgetItem("func_name:");
        item2->setFlags(item2->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow,0,item2);
        ui->ItemInformation->setItem(currentTableRow,1,new QTableWidgetItem(functionname.toString()));
        currentTableRow++;

        //the 4th
        QJsonValue returntype = selectItemObject.value(QString("3 return_type"));
        QTableWidgetItem *item3 = new QTableWidgetItem("return:");
        item3->setFlags(item3->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow,0,item3);
        ui->ItemInformation->setItem(currentTableRow,1,new QTableWidgetItem(returntype.toString()));

        //the 5th
        QJsonValue returnvalue = selectItemObject.value(QString("4 return_value"));
        if(returnvalue.isNull()){
            ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem("null"));
        }else if(returnvalue.isDouble()) {
            QString temp = QString::number(returnvalue.toDouble(),'g',16);
            ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(temp));
        }else if(returnvalue.isObject()){
            currentTableRow++;
            QJsonObject temp = returnvalue.toObject();
            QStringList tempkey = temp.keys();
            QJsonValue tempvalue = *temp.begin();
            ui->ItemInformation->setItem(currentTableRow,1,new QTableWidgetItem(tempkey[0]));
            ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(tempvalue.toString()));
        }else{
            ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(returnvalue.toString()));
        }
        currentTableRow++;
        ui->ItemInformation->setSpan(currentTableRow, 1, 1, 2);

        //the 6th + 7th
        QTableWidgetItem *item4 = new QTableWidgetItem("arg:");
        item4->setFlags(item4->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow,0,item4);
        QTableWidgetItem *item5 = new QTableWidgetItem();
        item5->setFlags(item5->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow,1,item5);
        QJsonArray argtype = selectItemObject["5 arg_type"].toArray();
        QJsonValue argvalue = selectItemObject.value(QString("6 arg_value"));
        QJsonObject argvalueobj = argvalue.toObject();
        QStringList argList = argvalueobj.keys();
        if(argtype.size() >= 1)
        {
            for (int argNum = 0; argNum < argtype.size(); argNum++)
            {
                currentTableRow++;
                ui->ItemInformation->setItem(currentTableRow,0,new QTableWidgetItem(argList[argNum]));
                ui->ItemInformation->setItem(currentTableRow,1,new QTableWidgetItem(argtype[argNum].toString()));
                QJsonValue eacharg = *(argvalueobj.begin()+argNum);
                if(eacharg.isNull())
                {
                    ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem("null"));
                }
                if(eacharg.isDouble())
                {
                    QString temp = QString::number(eacharg.toDouble(),'g',16);
                    ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(temp));
                }
                if(eacharg.isString())
                {
                    ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(eacharg.toString()));
                }

                if(eacharg.isObject())
                {
                    QJsonObject objectarg = eacharg.toObject();
                    QStringList objectargList = objectarg.keys();
                    for(int i = 0; i< objectargList.size(); i++)
                    {
                        ++currentTableRow;
                        ui->ItemInformation->setItem(currentTableRow,1,new QTableWidgetItem(objectargList[i]));
                        QJsonValue objectargItem = *(objectarg.begin()+i);
                        if(objectargItem.isNull())
                        {
                            ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem("null"));
                        }
                        if(objectargItem.isDouble())
                        {
                            QString temp = QString::number(objectargItem.toDouble(),'g',16);
                            ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(temp));
                        }
                        if(objectargItem.isString())
                        {
                            ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(objectargItem.toString()));
                        }

                        if(objectargItem.isArray())
                        {
                            QJsonArray arrayargItem = objectargItem.toArray();
                            if(arrayargItem.size() >= 1)
                            {
                                for (int i = 0; i < arrayargItem.size()-1; i++)
                                {
                                    QString temp = QString::number(arrayargItem[i].toDouble(),'g',16);
                                    ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(temp));
                                    currentTableRow++;
                                }
                                QString temp = QString::number(arrayargItem[arrayargItem.size()-1].toDouble(),'g',16);
                                ui->ItemInformation->setItem(currentTableRow,2,new QTableWidgetItem(temp));
                            }
                        }
                    }
                }
            }
        }
    }
}

void MainWindow::doSearching()
{
    if(list.size()>0)
    {
        for(int i = 0; i<nCount; i++)
        {
            list[i]->setBackground(0,QColor(0, 0, 0, 0));
        }
        list.clear();
    }
    findnum = 0;
    strTemplate = ui->Filter->text();
    if(strTemplate.isEmpty())
    {
        return;
    }

    list = ui->treeJsonFile->findItems(strTemplate,Qt::MatchContains | Qt::MatchRecursive,0);

    nCount = list.count();
    if(nCount < 1)
    {
        QMessageBox::information(this,tr("Results"),tr("No matching"));
        return;
    }

    for(int i = 0; i<nCount; i++)
    {
        list[i]->setBackground(0,QColor(0, 0, 255, 31));
    }

    ui->treeJsonFile->setCurrentItem(list[findnum]);
    ui->treeJsonFile->scrollToItem(list[findnum]);
    ui->treeJsonFile->setFocus();

}

void MainWindow::on_FindNext_clicked()
{
    QString nextTemplate = ui->Filter->text();
    if (nextTemplate != strTemplate)
    {
        QMessageBox::information(this,tr("Results"),tr("Keywords changed"));
        return;
    }
    else{
        if(findnum == nCount-1){
            findnum = 0;
        }else{
            ++findnum;
        }
        ui->treeJsonFile->setCurrentItem(list[findnum]);
        ui->treeJsonFile->scrollToItem(list[findnum]);
        ui->treeJsonFile->setFocus();
    }
}

void MainWindow::on_FindPrevious_clicked()
{
    QString nextTemplate = ui->Filter->text();
    if (nextTemplate != strTemplate)
    {
        QMessageBox::information(this,tr("Results"),tr("Keywords changed"));
        return;
    }
    else{
        if(findnum == 0){
            findnum = nCount-1;
        }else{
            --findnum;
        }
        ui->treeJsonFile->setCurrentItem(list[findnum]);
        ui->treeJsonFile->scrollToItem(list[findnum]);
        ui->treeJsonFile->setFocus();
    }
}

void MainWindow::on_actionSetChange_clicked()
{
    QTreeWidgetItem *item = ui->treeJsonFile->currentItem();
    QTreeWidgetItem *parent = item->parent();
    if(parent == NULL) return;
    int num = ui->treeJsonFile->indexOfTopLevelItem(parent);
    int frame_start_num = frame_start[num];
    int row=parent->indexOfChild(item);
    int changedItemNum = frame_start_num+row;
    int currentTableRow = 0;
    resave_string.removeAt(changedItemNum);

    QJsonObject changedItemObject;
    if(ui->ItemInformation->item(currentTableRow,1) == NULL){return;}
    QString str0 = ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
    int newCall_no = str0.toInt();
    changedItemObject.insert("0 call_no",newCall_no);
    currentTableRow++;

    if(ui->ItemInformation->item(currentTableRow,1) == NULL){return;}
    QString str1 = ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
    int newTid_no = str1.toInt();
    changedItemObject.insert("1 tid",newTid_no);
    currentTableRow++;

    if(ui->ItemInformation->item(currentTableRow,1) == NULL){return;}
    QString str2= ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
    changedItemObject.insert("2 func_name",str2);
    currentTableRow++;

    if(ui->ItemInformation->item(currentTableRow,1) == NULL){return;}
    QString str3= ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
    changedItemObject.insert("3 return_type",str3);
    if(ui->ItemInformation->item(currentTableRow,2) != NULL){
        QString str4= ui->ItemInformation->item(currentTableRow,2)->text().trimmed();
        if(str4 == "null"){
            changedItemObject.insert("4 return_value",QJsonValue::Null);
        }else if ((48<=str4[0]&&str4[0]<=57) || str4[0] == "-") {
            int temp = str4.toDouble();
            changedItemObject.insert("4 return_value",temp);
        }
        else{
            changedItemObject.insert("4 return_value",str4);
        }
    }else if(ui->ItemInformation->item(currentTableRow,2) == NULL && ui->ItemInformation->item(currentTableRow+1,0) == NULL &&
       ui->ItemInformation->item(currentTableRow+1,1) != NULL && ui->ItemInformation->item(currentTableRow+1,2) != NULL ){
        currentTableRow++;
        QJsonObject returnobject;
        QString str4_pointertype= ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
        QString str4_pointervalue= ui->ItemInformation->item(currentTableRow,2)->text().trimmed();
        returnobject.insert(str4_pointertype,str4_pointervalue);
        changedItemObject.insert("4 return_value",returnobject);
    }else{
        return;
    }
    currentTableRow++;

    int arg_num = -1;
    QJsonArray changed_argtype;
    QJsonObject changed_argvalue;
    if(ui->ItemInformation->item(currentTableRow,0) != NULL && ui->ItemInformation->item(currentTableRow,0)->text().contains("arg"))
    {
        currentTableRow++;
        for(int line = currentTableRow; line< ui->ItemInformation->rowCount(); line++)
        {
            if(ui->ItemInformation->item(line,0) != NULL && ui->ItemInformation->item(line,1) != NULL )
            {
                arg_num++;
                QString arg = ui->ItemInformation->item(line,0)->text().trimmed();  //In JSON:arg_value\type
                if(48>arg[0]&&arg[0]>57)
                {
                    QMessageBox::warning(this,"Warning","Args should start with a number",QMessageBox::Yes);
                    return;
                }
                QString arg_type_first = ui->ItemInformation->item(line,1)->text().trimmed();   //In JSON:arg_type
                changed_argtype.insert(arg_num,arg_type_first);
                if(ui->ItemInformation->item(line,2) != NULL && ui->ItemInformation->item(line,2)->text() != "") //arg is not object type
                {
                    QString arg_value = ui->ItemInformation->item(line,2)->text().trimmed();    //In JSON:arg_value\value
                    if(arg_value == "null")
                    {
                        changed_argvalue.insert(arg,QJsonValue::Null);
                    }
                    else if ((48<=arg_value[0]&&arg_value[0]<=57) || arg_value[0] == "-")
                    {
                        double temp = arg_value.toDouble();
                        changed_argvalue.insert(arg,temp);
                    }
                    else
                    {
                        changed_argvalue.insert(arg,arg_value);
                    }
                }else if((ui->ItemInformation->item(line,2) == NULL && ui->ItemInformation->item(line+1,0) == NULL &&
                         ui->ItemInformation->item(line+1,1) != NULL) ||
                         (ui->ItemInformation->item(line,2) != NULL && ui->ItemInformation->item(line,2)->text() == "")){//arg is object type
                    QJsonObject argvalue_object;
                    QString arg_type_second = ui->ItemInformation->item(line+1,1)->text().trimmed();
                    if(arg_type_first != "Array" && arg_type_first != "array" )// objectarg contains pointer
                    {
                        if(ui->ItemInformation->item(line+1,2) != NULL)
                        {
                            QString arg_value = ui->ItemInformation->item(line+1,2)->text().trimmed();
                            if ((48<=arg_value[0]&&arg_value[0]<=57) || arg_value[0] == "-")
                            {
                                double temp = arg_value.toDouble();
                                argvalue_object.insert(arg_type_second,temp);
                            }
                            else{
                                argvalue_object.insert(arg_type_second,arg_value);
                            }
                        }else{
                            argvalue_object.insert(arg_type_second,QJsonValue::Null);
                        }
                    }else{// objectarg contains array
                        QJsonArray argvalueobject_array;
                        int array_arg_length = 0;
                        if(ui->ItemInformation->item(line+1,2) != NULL)
                        {
                            for(int i = line+1; i< ui->ItemInformation->rowCount(); i++)
                            {
                                if(ui->ItemInformation->item(i+1,1) == NULL && ui->ItemInformation->item(i+1,2) != NULL) {
                                    array_arg_length++;
                                }else{
                                    array_arg_length++;
                                    break;
                                }
                            }
                            for(int i =0; i<array_arg_length; i++)
                            {
                                QString temp = ui->ItemInformation->item(line+1+i,2)->text().trimmed();
                                if ((48<=temp[0]&&temp[0]<=57) || temp[0] == "-")
                                {
                                    float temp_double = temp.toDouble();
                                    argvalueobject_array.insert(i,temp_double);
                                }
                                else
                                {
                                    argvalueobject_array.insert(i,temp);
                                }
                            }
                        }
                        argvalue_object.insert(arg_type_second,argvalueobject_array);
                    }
                    changed_argvalue.insert(arg,argvalue_object);
                }else{return;}
            }
        }
        if(arg_num == -1)
        {
            changedItemObject.insert("5 arg_type",changed_argtype);
            changedItemObject.insert("6 arg_value",QJsonValue::Null);
        }else{
            changedItemObject.insert("5 arg_type",changed_argtype);
            changedItemObject.insert("6 arg_value",changed_argvalue);
        }
    }
    QJsonDocument document(changedItemObject);
    QString changedString = document.toJson(QJsonDocument::Indented);
    changedString.remove(changedString.size()-1,1);
    resave_string.insert(changedItemNum,changedString);
    ui->treeJsonFile->clearSelection();
    ui->treeJsonFile->setCurrentItem(item,0);
}

void MainWindow::showtheChange(int framenum,int callnum,QJsonObject addItemObject, QString addText)
{
    newitemlocation->setBackground(0,QColor(0, 0, 0, 0));
    frame_end[framenum] = frame_end[framenum]+1;
    for(int i=framenum+1; i<int(frame_end.size()); i++)
    {
        frame_start[i] = frame_start[i]+1;
        frame_end[i] = frame_end[i]+1;
    }

    QJsonDocument document(addItemObject);
    QString addString = document.toJson(QJsonDocument::Indented);
    addString.remove(addString.size()-1,1);
    resave_string.insert(callnum+1,addString);

    QTreeWidgetItem *item1=new QTreeWidgetItem;
    item1->setText(0,addText);
    if(newitemlocation->parent()==NULL)
    {
        newitemlocation->insertChild(0,item1);
    }else{
        newitemlocation->parent()->insertChild(callnum-frame_start[framenum]+1,item1);
    }
    ui->treeJsonFile->clearSelection();
    ui->treeJsonFile->setCurrentItem(item1);
    ui->treeJsonFile->scrollToItem(item1);
    new_Dialog->close();//close the subwindow
}

void MainWindow::on_actionAdd_clicked()
{
    if( JsonOpened != true)
    {
        QMessageBox::warning(this,"Warning","No Opened Json",QMessageBox::Yes);
        return;
    }
    QTreeWidgetItem *item = ui->treeJsonFile->currentItem();
    QTreeWidgetItem *parent = item->parent();
    if(parent == NULL) return;
    int num = ui->treeJsonFile->indexOfTopLevelItem(parent);
    int frame_start_num = frame_start[num];
    int row=parent->indexOfChild(item);
    int addItemNum = frame_start_num+row;

    QJsonObject addItemObject;
    int currentTableRow = 0;

    if(ui->ItemInformation->item(currentTableRow,1) == NULL){return;}
    QString str0 = ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
    int newCall_no = str0.toInt();
    addItemObject.insert("0 call_no",newCall_no);
    currentTableRow++;

    if(ui->ItemInformation->item(currentTableRow,1) == NULL){return;}
    QString str1 = ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
    int newTid_no = str1.toInt();
    addItemObject.insert("1 tid",newTid_no);
    currentTableRow++;

    if(ui->ItemInformation->item(currentTableRow,1) == NULL){return;}
    QString str2= ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
    addItemObject.insert("2 func_name",str2);
    currentTableRow++;

    if(ui->ItemInformation->item(currentTableRow,1) == NULL){return;}
    QString str3= ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
    addItemObject.insert("3 return_type",str3);
    if(ui->ItemInformation->item(currentTableRow,2) != NULL){
        QString str4= ui->ItemInformation->item(currentTableRow,2)->text().trimmed();
        if(str4 == "null"){
            addItemObject.insert("4 return_value",QJsonValue::Null);
        }else if ((48<=str4[0]&&str4[0]<=57) || str4[0] == "-") {
            int temp = str4.toDouble();
            addItemObject.insert("4 return_value",temp);
        }
        else{
            addItemObject.insert("4 return_value",str4);
        }
    }else if(ui->ItemInformation->item(currentTableRow,2) == NULL && ui->ItemInformation->item(currentTableRow+1,0) == NULL &&
       ui->ItemInformation->item(currentTableRow+1,1) != NULL && ui->ItemInformation->item(currentTableRow+1,2) != NULL ){
        currentTableRow++;
        QJsonObject returnobject;
        QString str4_pointertype= ui->ItemInformation->item(currentTableRow,1)->text().trimmed();
        QString str4_pointervalue= ui->ItemInformation->item(currentTableRow,2)->text().trimmed();
        returnobject.insert(str4_pointertype,str4_pointervalue);
        addItemObject.insert("4 return_value",returnobject);
    }else{
        return;
    }
    currentTableRow++;

    int arg_num = -1;
    QJsonArray changed_argtype;
    QJsonObject changed_argvalue;
    if(ui->ItemInformation->item(currentTableRow,0) != NULL && ui->ItemInformation->item(currentTableRow,0)->text().contains("arg"))
    {
        currentTableRow++;
        for(int line = currentTableRow; line< ui->ItemInformation->rowCount(); line++)
        {
            if(ui->ItemInformation->item(line,0) != NULL && ui->ItemInformation->item(line,1) != NULL )
            {
                arg_num++;
                QString arg = ui->ItemInformation->item(line,0)->text().trimmed();  //In JSON:arg_value\type
                if(48>arg[0] || arg[0]>57)
                {
                    QMessageBox::warning(this,"Warning","Args should start with a number",QMessageBox::Yes);
                    return;
                }
                QString arg_type_first = ui->ItemInformation->item(line,1)->text().trimmed();   //In JSON:arg_type
                changed_argtype.insert(arg_num,arg_type_first);
                if(ui->ItemInformation->item(line,2) != NULL) //arg is not object type
                {
                    QString arg_value = ui->ItemInformation->item(line,2)->text().trimmed();    //In JSON:arg_value\value
                    if(arg_value == "null")
                    {
                        changed_argvalue.insert(arg,QJsonValue::Null);
                    }
                    else if ((48<=arg_value[0]&&arg_value[0]<=57) || arg_value[0] == "-")
                    {
                        double temp = arg_value.toDouble();
                        changed_argvalue.insert(arg,temp);
                    }
                    else
                    {
                        changed_argvalue.insert(arg,arg_value);
                    }
                }else if(ui->ItemInformation->item(line,2) == NULL && ui->ItemInformation->item(line+1,0) == NULL &&
                         ui->ItemInformation->item(line+1,1) != NULL){//arg is object type
                    QJsonObject argvalue_object;
                    QString arg_type_second = ui->ItemInformation->item(line+1,1)->text().trimmed();
                    if(arg_type_first != "Array" && arg_type_first != "array" )// objectarg contains pointer
                    {
                        if(ui->ItemInformation->item(line+1,2) != NULL)
                        {
                            QString arg_value = ui->ItemInformation->item(line+1,2)->text().trimmed();
                            if ((48<=arg_value[0]&&arg_value[0]<=57) || arg_value[0] == "-")
                            {
                                double temp = arg_value.toFloat();
                                argvalue_object.insert(arg_type_second,temp);
                            }
                            else{
                                argvalue_object.insert(arg_type_second,arg_value);
                            }
                        }else{
                            argvalue_object.insert(arg_type_second,QJsonValue::Null);
                        }
                    }else{// objectarg contains array
                        QJsonArray argvalueobject_array;
                        int array_arg_length = 0;
                        if(ui->ItemInformation->item(line+1,2) != NULL)
                        {
                            for(int i = line+1; i< ui->ItemInformation->rowCount(); i++)
                            {
                                if(ui->ItemInformation->item(i+1,1) == NULL && ui->ItemInformation->item(i+1,2) != NULL) {
                                    array_arg_length++;
                                }else{
                                    array_arg_length++;
                                    break;
                                }
                            }
                            for(int i =0; i<array_arg_length; i++)
                            {
                                QString temp = ui->ItemInformation->item(line+1+i,2)->text().trimmed();
                                if ((48<=temp[0]&&temp[0]<=57) || temp[0] == "-")
                                {
                                    double temp_double = temp.toDouble();
                                    argvalueobject_array.insert(i,temp_double);
                                }
                                else
                                {
                                    argvalueobject_array.insert(i,temp);
                                }
                            }
                        }
                        argvalue_object.insert(arg_type_second,argvalueobject_array);
                    }
                    changed_argvalue.insert(arg,argvalue_object);
                }else{return;}
            }
        }
        if(arg_num == -1)
        {
            addItemObject.insert("5 arg_type",changed_argtype);
            addItemObject.insert("6 arg_value",QJsonValue::Null);
        }else{
            addItemObject.insert("5 arg_type",changed_argtype);
            addItemObject.insert("6 arg_value",changed_argvalue);
        }
    }

    QJsonDocument document(addItemObject);
    QString addString = document.toJson(QJsonDocument::Indented);
    addString.remove(addString.size()-1,1);
    resave_string.insert(addItemNum+1,addString);

    QTreeWidgetItem *addItem=new QTreeWidgetItem;
    QString addText = "("+str0+")"+str2;
    addItem->setText(0,addText);
    parent->insertChild(row+1,addItem);

    frame_end[num] = frame_end[num]+1;
    for(int i=num+1; i<int(frame_end.size()); i++)
    {
        frame_start[i] = frame_start[i]+1;
        frame_end[i] = frame_end[i]+1;
    }

    ui->treeJsonFile->clearSelection();
    ui->treeJsonFile->setCurrentItem(addItem);
}

void MainWindow::on_actionDelete_clicked()
{
    QTreeWidgetItem *item = ui->treeJsonFile->currentItem();
    if(item == NULL) return;
    QTreeWidgetItem *parent = item->parent();
    int row;

    if(parent == NULL)
    {
        int FrameNum = ui->treeJsonFile->indexOfTopLevelItem(item);
        int frame_start_num = frame_start[FrameNum];
        int frame_end_num = frame_end[FrameNum];

        for (int i = frame_start_num; i <= frame_end_num; i++)
        {
          delete item->takeChild(0); //delete son point from father point
          resave_string.removeAt(frame_start_num);
        }

        std::vector<int>::iterator i1 = frame_start.begin();
        std::vector<int>::iterator i2 = frame_end.begin();
        frame_start.erase(i1+FrameNum);
        frame_end.erase(i2+FrameNum);
        for(int j=FrameNum; j<int(frame_end.size()); j++)
        {
            frame_start[j] = frame_start[j]-frame_end_num+frame_start_num-1;
            frame_end[j] = frame_end[j]-frame_end_num+frame_start_num-1;
        }

        delete ui->treeJsonFile->takeTopLevelItem(FrameNum); //delete father point
        //delete item;
        ui->treeJsonFile->clearSelection();
        ui->treeJsonFile->setCurrentItem(ui->treeJsonFile->topLevelItem(FrameNum));

    }else{
        int childcount = parent->childCount();
        int FrameNum = ui->treeJsonFile->indexOfTopLevelItem(parent);
        int frame_start_num = frame_start[FrameNum];
        row = parent->indexOfChild(item);
        int deleteItemNum = frame_start_num+row;
        resave_string.removeAt(deleteItemNum);
        delete parent->takeChild(row);
        frame_end[FrameNum] = frame_end[FrameNum]-1;
        for(int i=FrameNum+1; i<int(frame_end.size()); i++)
        {
            frame_start[i] = frame_start[i]-1;
            frame_end[i] = frame_end[i]-1;
        }
        ui->treeJsonFile->clearSelection();
        ui->treeJsonFile->setCurrentItem(parent);
        if(childcount>1)
        {
            if(row == childcount-1)
            {
               ui->treeJsonFile->setCurrentItem(parent->child(row-1));
            }else{
                ui->treeJsonFile->setCurrentItem(parent->child(row));
            }
        }
    }
}
