#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(size());
    m_ad = new QProcess(this);
    m_targetsGroup = new QActionGroup(ui->menuTargets);
    readSettings(true);
    ui->cbOperationType->addItems(m_operationTypes);
    ui->cbMemoryType->addItems(m_memoryTypes);
    ui->tbOptions->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    addAdditionalOptions();
    ui->statusBar->showMessage(tr("Ready"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setPathToDataFile()
{
    QString file(getFile());

    if (file.length())
    {
        ui->leFilePath->setText(file);
        m_pathToFirmware = file;
        ui->bStart->setEnabled(true);
    }
}

void MainWindow::changeCheckboxState(int)
{
    QCheckBox* cb = dynamic_cast<QCheckBox*>(sender());
    cb->isChecked() ? cb->setText(tr("Enabled")) : cb->setText(tr("Disabled"));
}

void MainWindow::selectFile()
{
    QString file(getFile());

    if (file.length())
    {
        if (!ui->tbOptions->item(sender()->objectName().toInt(), 1))//if at given position widget not exist, create new
        {
            QTableWidgetItem* itm = new QTableWidgetItem(file);//table will become parent after adding this widget
            itm->setToolTip(file);
            ui->tbOptions->setItem(sender()->objectName().toInt(), 1, itm);//object name is x coordinate in table
        }
        else ui->tbOptions->item(sender()->objectName().toInt(), 1)->setText(file);//if widget exist then set new text with path to file
    }
}

void MainWindow::startOperation()
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);
    QString pathToAvrDude;
    QString pathToFile(ui->leFilePath->text());
    QString operationType;
    QStringList arguments;

    m_ad->setProcessChannelMode(QProcess::MergedChannels);//set this to get output from AVRDUDE

    settings.beginGroup(sGroup);
    pathToAvrDude = settings.value(sPath, QString()).toString();
    arguments.append("-p"); arguments.append(settings.value(sParts, QMap<QString, QVariant>()).toMap().key(ui->cbPart->currentText()));//get argument based on selected description
    arguments.append("-c"); arguments.append(settings.value(sProgrammers, QMap<QString, QVariant>()).toMap().key(ui->cbProgrammer->currentText()));
    settings.endGroup();

    if (pathToAvrDude.isEmpty())
    {
        QMessageBox::critical(this, tr("Error"), tr("Path to AVRDUDE has not been set.\nTo set path, use options menu."));
        return;
    }

    m_ad->setProgram(pathToAvrDude);

    switch (ui->cbOperationType->currentIndex())
    {
        case 0:
            operationType = "w";
            break;
        case 1:
            operationType = "r";
            break;
        case 2:
            operationType = "v";
            break;
    }

    QString fileType = (ui->cbOperationType->currentIndex() != 1) ? "a" : getFileTypeForSave(pathToFile);//if we are writing, then set file type to 'a' so AVRDUDE will recognize this by itself, otherwise determine appropriate arg

    if ((ui->cbMemoryType->currentIndex() != 2 || operationType == "r") && pathToFile.isEmpty()) //if memory type is not other (to write fusebits or use other additional options), we need file to write or read to
    {
        QMessageBox::critical(this, tr("Error"), tr("Path to file has not been set.\nSet path to execute selected operation."));
        return;
    }

    if (operationType == "w" || operationType == "v")
    {
        if (ui->chbLowFuse->isChecked()) arguments.append("-U"); arguments.append("lfuse:" + operationType + ":0x" + ui->leLowFuse->text() + ":m");
        if (ui->chbHighFuse->isChecked()) arguments.append("-U"); arguments.append("hfuse:" + operationType + ":0x" +  ui->leHighFuse->text() + ":m");
        if (ui->chbExtendedFuse->isChecked()) arguments.append("-U"); arguments.append("efuse:" + operationType + ":0x" +  ui->leExtendedFuse->text() + ":m");
        if (ui->chbSingleFuse->isChecked()) arguments.append("-U"); arguments.append("fuse:" + operationType + ":0x" + ui->leSingleFuse->text() + ":m");
    }
    else
    {
        QFileInfo f(pathToFile);
        if (ui->chbLowFuse->isChecked()) arguments.append("-U"); arguments.append("lfuse:" + operationType + ":" + f.absoluteDir().path() + f.baseName() + "_lfuse" + f.completeSuffix() + ":h");
        if (ui->chbHighFuse->isChecked()) arguments.append("-U"); arguments.append("hfuse:" + operationType + ":" + f.absoluteDir().path() + f.baseName() + "_hfuse" + f.completeSuffix() + ":h");
        if (ui->chbExtendedFuse->isChecked()) arguments.append("-U"); arguments.append("efuse:" + operationType + ":" + f.absoluteDir().path() + f.baseName() + "_efuse" + f.completeSuffix() + ":h");
        if (ui->chbSingleFuse->isChecked()) arguments.append("-U"); arguments.append("fuse:" + operationType + ":" + f.absoluteDir().path() + f.baseName() + "_fuse" + f.completeSuffix() + ":h");
    }

    if (ui->leProgrammerPort->text().length()) arguments.append("-P"); arguments.append(ui->leProgrammerPort->text());

    if (ui->cbMemoryType->currentIndex() == 0)
    {
        arguments.append("-U"); arguments.append("flash:" + operationType + ":" + pathToFile + ":" + fileType);
    }
    else if (ui->cbMemoryType->currentIndex() == 1)
    {
        arguments.append("-U"); arguments.append("eeprom:" + operationType + ":" + pathToFile + ":" + fileType);
    }

    for (unsigned short i = 0; i < ui->tbOptions->rowCount(); i++)//check all additional arguments if default value changed, if so then add changed parameter to AVRDUDE
    {
        switch (getOptionType(i))
        {
            case additionalOptionsTypes::text://text and file path are treated the same, function getOptionWithArg will determine correct argument call
            case additionalOptionsTypes::filePath:
                if (!ui->tbOptions->model()->index(i, 1).data().toString().isEmpty()) arguments.append(getOptionWithArg(i, ui->tbOptions->model()->index(i, 1).data().toString(), operationType, fileType));
                break;

            case additionalOptionsTypes::logic:
            {
                QCheckBox* cb = ui->tbOptions->cellWidget(i, 1)->findChild<QCheckBox*>(QString::number(i));
                if (cb->checkState()) arguments.append(getOptionWithArg(i, "", "", ""));
            }
            break;

            case additionalOptionsTypes::itemList:
            {
                QComboBox* cb = ui->tbOptions->cellWidget(i, 1)->findChild<QComboBox*>(QString::number(i));
                if (cb->currentIndex()) arguments.append(getOptionWithArg(i, cb->currentText(), "", ""));
            }
            break;
        }
    }

    arguments.append("-s");
    m_ad->setArguments(arguments);
    connect(m_ad, &QProcess::readyReadStandardOutput, this, &MainWindow::updateOutputInformation);
    connect(m_ad, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [ = ](/*int exitCode, QProcess::ExitStatus exitStatus*/) {ui->statusBar->showMessage(tr("Ready")); ui->bStart->setEnabled(true);});
    ui->pteOutput->clear();
    ui->progressBar->setValue(0);
    ui->bStart->setEnabled(false);
    m_ad->start();
}

void MainWindow::updateOutputInformation()
{
    QString output(m_ad->readAllStandardOutput());

    ui->pteOutput->moveCursor(QTextCursor::End);
    ui->pteOutput->insertPlainText(output);
    ui->pteOutput->moveCursor(QTextCursor::End);

    QRegularExpression reOperation("\\w+ \\| ");//re to get operation type
    QRegularExpression reProgress("(#)+");//re to get precentage
    QRegularExpressionMatchIterator matchesOp = reOperation.globalMatch(output);
    QRegularExpressionMatchIterator matchesPr = reProgress.globalMatch(output);

    if (matchesOp.hasNext())
    {
        ui->statusBar->showMessage(matchesOp.next().captured() + QFileInfo(ui->leFilePath->text()).fileName());
        ui->progressBar->setValue(0);//if operation changes, change progress bar value
    }
    if (matchesPr.hasNext())
    {
        QString prc(matchesPr.next().captured());//get all '#' chars from output
        ui->progressBar->setValue(ui->progressBar->value() + (prc.length() * 2));//one '#' means 2% progress, max is 50 '#' chars on output
    }
}

void MainWindow::addAdditionalOptions()
{
    for (unsigned short i = 0; i < getOptionsCount(); i++)
    {
        QPair<QString, QWidget*> p = getOptionWidget(i);
        if (p.first.isEmpty()) ui->tbOptions->setCellWidget(i, 0, p.second);//if description of widget is empty (ex. button) then set it in first column
        else
        {
            QWidget* w = new QWidget();//else create widget containing description wrapped in layout
            QHBoxLayout* layout = new QHBoxLayout(w);
            layout->addItem(new QSpacerItem(5, 0));
            layout->addWidget(new QLabel(p.first));
            layout->addItem(new QSpacerItem(5, 0));
            layout->setMargin(0);
            w->setLayout(layout);
            w->setMinimumSize(w->sizeHint());

            ui->tbOptions->setCellWidget(i, 0, w);//and set this widget inside first column
            ui->tbOptions->setCellWidget(i, 1, p.second);//and second widget, generated in getOptionWidget (ex. combobox) in second column
        }
    }
    ui->tbOptions->horizontalHeader()->show();//after adding all widgets, show horizontal column headers
    ui->tbOptions->setHorizontalHeaderLabels(QStringList() << tr("Option") << tr("Value"));//and change their names
}

void MainWindow::addTargetToMenu(QString targetName, QString nameInMenu)
{
    QAction* act = new QAction(nameInMenu, m_targetsGroup);
    act->setObjectName(targetName);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered(bool)), this, SLOT(changeTarget(bool)));//use SIGNAL and SLOT because function is overloaded
    ui->menuTargets->addAction(act);
    m_currentTarget = targetName;//update current target name
}

void MainWindow::removeTargetFromMenu(QString targetName)
{
    ui->menuTargets->removeAction(ui->menuTargets->findChild<QAction*>(targetName));
}

void MainWindow::readSettings(bool updateCurrentData)
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

    if (!settings.allKeys().size())
    {
        QMessageBox::information(this, tr("Information"), tr("You need to initialize informations about parts and programmers.\nTo do this, use options menu."));
        return;
    }

    settings.beginGroup(sGroup);
    m_parts = settings.value(sParts, QMap<QString, QVariant>()).toMap();
    m_programmers = settings.value(sProgrammers, QMap<QString, QVariant>()).toMap();

    ui->cbPart->clear();
    ui->cbProgrammer->clear();

    foreach(QVariant value, settings.value(sParts, QMap<QString, QVariant>()).toMap().values()) ui->cbPart->addItem(value.toString());
    foreach (QVariant value, settings.value(sProgrammers, QMap<QString, QVariant>()).toMap().values())
    {
        ui->cbProgrammer->addItem(value.toString());
        ui->cbProgrammer->setItemData(ui->cbProgrammer->count() - 1, ui->cbProgrammer->itemText(ui->cbProgrammer->count() - 1), Qt::ToolTipRole);
    }

    ui->cbPart->setCurrentIndex(settings.value(sPartId, int()).toInt());
    ui->cbProgrammer->setCurrentIndex(settings.value(sProgrammerId, int()).toInt());
    settings.endGroup();

    if (!ui->cbPart->count() || !ui->cbProgrammer->count())
    {
        QMessageBox::information(this, tr("Information"), tr("To start using this program, you need to\ninitialize informations about parts and programmers.\nTo do this, use options menu."));
        return;
    }

    delete m_targetsGroup;
    m_targetsGroup = new QActionGroup(ui->menuTargets);

    settings.beginGroup(sTargetsGroup);

    foreach (QString tg, settings.childGroups())
    {
        settings.beginGroup(tg);
        addTargetToMenu(tg, settings.value(sTargetName).toString());
        if (settings.value(sTargetDefault, bool(false)).toBool() && updateCurrentData)//if some target has been set as default, then read data from that target and update interface if it was allowed by arg
        {
            changeTarget(tg);
            m_targetsGroup->findChild<QAction*>(tg)->setChecked(true);
        }
        settings.endGroup();
    }

    settings.endGroup();

    ui->cbPart->setEnabled(true);
    ui->cbProgrammer->setEnabled(true);
    ui->bSelectFile->setEnabled(true);
    ui->bStart->setEnabled(true);
    ui->actionAddCurrentAsNewTarget->setEnabled(true);
    ui->actionRemoveSelectedTarget->setEnabled(true);
}

QString MainWindow::getFileTypeForSave(QString pathToFile) const
{
    QStringList data = QFileInfo(pathToFile).fileName().split('.');

    if (data.length() == 1) return "r";

    if (data.last() == "hex") return "i";
    else if (data.last() == "srec") return "s";

    return "r";
}

QString MainWindow::getFile()
{
    QString file;

    if (ui->cbOperationType->currentIndex() != 1) file = QFileDialog::getOpenFileName(this, tr("Select data file"), QString(), "Intel HEX (*.hex);;Motorola S-record (*.srec);;Raw binary (*.bin);;ELF (*.elf);;Custom (*.*)");//for write or verify
    else
    {
        file = QFileDialog::getSaveFileName(this, tr("Select data file"), QString(), "Intel HEX (*.hex);;Motorola S-record (*.srec);;Raw binary (*.bin);;Custom (*.*)");
        if (!QFileInfo(file).exists()) if (!QFile(file).open(QIODevice::OpenModeFlag::ReadWrite)) QMessageBox::critical(this, tr("Error"), tr("Selected file cannot be created.\nCheck if you have required premissions."));
    }

    return file;
}

QPair<QString, QWidget*> MainWindow::getOptionWidget(unsigned short optionNumber)
{
    if (optionNumber >= sizeof(m_additionalOptionsValues)) return QPair<QString, QWidget*>("", nullptr);

    QPair<QString, QWidget*> res;
    QWidget* w = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(w);
    layout->addItem(new QSpacerItem(5, 0));

    switch (m_additionalOptionsValues[optionNumber])//create new widget depending on selected option, objectName will be set to optionNumber
    {
        case additionalOptionsTypes::logic:
        {
            QCheckBox* cb = new QCheckBox(tr("Disabled"), w);
            cb->setObjectName(QString::number(optionNumber));
            cb->setToolTip(m_additionalOptionsToolTip[optionNumber]);
            connect(cb, &QCheckBox::stateChanged, this, &MainWindow::changeCheckboxState);
            res.first = m_additionalOptionsDesc[optionNumber];
            layout->addWidget(cb);
        }
        break;

        case additionalOptionsTypes::text:
        {
            QLabel* l = new QLabel(m_additionalOptionsDesc[optionNumber], w);
            l->setObjectName(QString::number(optionNumber));
            l->setToolTip(m_additionalOptionsToolTip[optionNumber]);
            res.first = "";
            layout->addWidget(l);
        }
        break;

        case additionalOptionsTypes::filePath:
        {
            QPushButton* b = new QPushButton(m_additionalOptionsDesc[optionNumber], w);
            b->setObjectName(QString::number(optionNumber));
            b->setToolTip(m_additionalOptionsToolTip[optionNumber]);
            connect(b, &QPushButton::clicked, this, &MainWindow::selectFile);
            res.first = "";
            layout->addWidget(b);
        }
        break;

        case additionalOptionsTypes::itemList:
        {
            QComboBox* cb = new QComboBox(w);
            cb->setObjectName(QString::number(optionNumber));
            cb->setToolTip(m_additionalOptionsToolTip[optionNumber]);
            for (short i = 0; i < m_itemLists[0][m_currentList].size(); i++)
            {
                cb->addItem(m_itemLists[0][m_currentList][i]);
            }
            res.first = m_additionalOptionsDesc[optionNumber];
            layout->addWidget(cb);
            m_currentList++;
        }
        break;
    }

    layout->addItem(new QSpacerItem(5, 0));
    layout->setMargin(0);
    w->setLayout(layout);
    w->setMinimumSize(w->sizeHint());
    res.second = w;
    return res;
}

QString MainWindow::getOptionArg(unsigned short optionNumber) const
{
    if (optionNumber < m_additionalOptionsArg.size()) return m_additionalOptionsArg[optionNumber];
    return QString();
}

MainWindow::additionalOptionsTypes MainWindow::getOptionType(unsigned short optionNumber) const
{
    return m_additionalOptionsValues[optionNumber];
}

unsigned short MainWindow::getOptionsCount() const
{
    return sizeof(m_additionalOptionsValues);
}

QStringList MainWindow::getOptionWithArg(unsigned short optionNumber, QString argument, QString operationType, QString valueType) const
{
    if (optionNumber >= sizeof(m_additionalOptionsValues)) return QStringList();

    if (m_additionalOptionsArg[optionNumber].length() > 2) return QStringList() << "-U" << m_additionalOptionsArg[optionNumber] + ":" + operationType + ":" + argument + ":" + valueType;

    return QStringList() << m_additionalOptionsArg[optionNumber] << argument;
}

void MainWindow::showOptions()
{
    OptionsDialog* dlg = new OptionsDialog(this);
    int res = dlg->exec();
    delete dlg;

    if (res == QDialog::Accepted) readSettings(false);
}

void MainWindow::showAbout()
{
    QString aboutCaption = QMessageBox::tr(
                               "<h2>About DuGu</h2>"
                               "<p>This program is GUI made using Qt version %1 for AVRDUDE.</p>"
                           ).arg(QLatin1String(QT_VERSION_STR));
    QString aboutText = QMessageBox::tr(
                            "<p>Main program icon (microchip) and options icon (cog), both without changes, "
                            "were used under CC BY 4.0 license from Font Awesome icons set. "
                            "License for these icons can be found <a href=\"%1\">here</a>.</p>"
                            "<p>Program version: %2</p>"
                            "<p>For updates and source code check <a href=\"%3\">my Github page</a>.</p>"
                            "<p>Copyright (C) 2019 Wojciech Cybowski</p>"
                            "<p></p>"
                        ).arg(QStringLiteral("https://fontawesome.com/license"), QCoreApplication::applicationVersion(), QStringLiteral("https://github.com/wcyb"));
    QMessageBox msg(this);
    msg.setWindowTitle(tr("About DuGu"));
    msg.setText(aboutCaption);
    msg.setInformativeText(aboutText);
    msg.setIconPixmap(QPixmap(":/images/microchip-solid.png").scaled(256, 256, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
    msg.exec();
}

void MainWindow::addTarget()
{
    bool freeGroupIdFound = false;
    QString newGroupName;
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

    settings.beginGroup(sTargetsGroup);

    for (unsigned short freeGroupId = 0; freeGroupId < settings.childGroups().size(); freeGroupId++)
    {
        //try to find first free group ID and create new group with that ID
        newGroupName = QString(sTargetData) + "_" + QString::number(freeGroupId);
        if (settings.childGroups().indexOf(newGroupName) != -1) freeGroupId++;
        else
        {
            settings.beginGroup(newGroupName);
            freeGroupIdFound = true;
            break;
        }
    }
    //if all ID's are taken, get last ID and add 1 to create new group ID
    if (!freeGroupIdFound && settings.childGroups().size())//check if there are other groups
    {
        newGroupName = QString(sTargetData) + "_" + QString::number(settings.childGroups().last().split('_').last().toShort() + 1);
        settings.beginGroup(newGroupName);
    }
    else if (!freeGroupIdFound)
    {
        newGroupName = QString(sTargetData) + "_" + QString::number(0);
        settings.beginGroup(newGroupName);//if group list is empty, add first group
    }

    settings.setValue(sTargetName, ui->cbPart->currentText() + ' ' + QFileInfo(ui->leFilePath->text()).fileName());

    settings.setValue(sTargetPartName, ui->cbPart->currentText());
    settings.setValue(sTargetPartId, ui->cbPart->currentIndex());

    if (ui->chbLowFuse->isChecked()) settings.setValue(sTargetLowFuse, ui->leLowFuse->text());
    if (ui->chbHighFuse->isChecked()) settings.setValue(sTargetHighFuse, ui->leHighFuse->text());
    if (ui->chbExtendedFuse->isChecked()) settings.setValue(sTargetExtendedFuse, ui->leExtendedFuse->text());
    if (ui->chbSingleFuse->isChecked()) settings.setValue(sTargetSingleFuse, ui->leSingleFuse->text());

    settings.setValue(sTargetProgrammerName, ui->cbProgrammer->currentText());
    settings.setValue(sTargetProgrammerId, ui->cbProgrammer->currentIndex());

    if (ui->leProgrammerPort->text().length()) settings.setValue(sTargetProgrammerPortName, ui->leProgrammerPort->text());
    if (ui->leFilePath->text().length()) settings.setValue(sTargetFile, ui->leFilePath->text());

    settings.setValue(sTargetOperationTypeName, ui->cbOperationType->currentText());
    settings.setValue(sTargetOperationTypeId, ui->cbOperationType->currentIndex());
    settings.setValue(sTargetMemoryTypeName, ui->cbMemoryType->currentText());
    settings.setValue(sTargetMemoryTypeId, ui->cbMemoryType->currentIndex());

    for (unsigned short i = 0; i < ui->tbOptions->rowCount(); i++)
    {
        switch (getOptionType(i))
        {
            case additionalOptionsTypes::text:
            case additionalOptionsTypes::filePath:
                if (!ui->tbOptions->model()->index(i, 1).data().toString().isEmpty()) settings.setValue(QString::number(i), ui->tbOptions->model()->index(i, 1).data().toString());
                break;

            case additionalOptionsTypes::logic:
            {
                QCheckBox* cb = ui->tbOptions->cellWidget(i, 1)->findChild<QCheckBox*>(QString::number(i));
                if (cb->checkState()) settings.setValue(QString::number(i), true);
            }
            break;

            case additionalOptionsTypes::itemList:
            {
                QComboBox* cb = ui->tbOptions->cellWidget(i, 1)->findChild<QComboBox*>(QString::number(i));
                if (cb->currentIndex()) settings.setValue(QString::number(i), cb->currentIndex());
            }
            break;
        }
    }

    addTargetToMenu(newGroupName, settings.value(sTargetName).toString());

    settings.endGroup();
    settings.endGroup();
}

void MainWindow::changeTarget(bool)
{
    changeTarget(sender()->objectName());
}

void MainWindow::changeTarget(QString newTarget)
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

    m_currentTarget = newTarget.isEmpty() ? sender()->objectName() : newTarget;

    settings.beginGroup(sTargetsGroup);
    settings.beginGroup(m_currentTarget);

    ui->cbPart->setCurrentIndex(settings.value(sTargetPartId, int()).toInt());

    if (!settings.value(sTargetLowFuse, QString()).toString().isEmpty()) ui->leLowFuse->setText(settings.value(sTargetLowFuse, QString()).toString());
    if (!settings.value(sTargetHighFuse, QString()).toString().isEmpty()) ui->leHighFuse->setText(settings.value(sTargetHighFuse, QString()).toString());
    if (!settings.value(sTargetExtendedFuse, QString()).toString().isEmpty()) ui->leExtendedFuse->setText(settings.value(sTargetExtendedFuse, QString()).toString());
    if (!settings.value(sTargetSingleFuse, QString()).toString().isEmpty()) ui->leSingleFuse->setText(settings.value(sTargetSingleFuse, QString()).toString());

    ui->cbProgrammer->setCurrentIndex(settings.value(sTargetProgrammerId, int()).toInt());

    if (!settings.value(sTargetProgrammerPortName, QString()).toString().isEmpty()) ui->leProgrammerPort->setText(settings.value(sTargetProgrammerPortName, QString()).toString());
    if (!settings.value(sTargetFile, QString()).toString().isEmpty()) ui->leFilePath->setText(settings.value(sTargetFile, QString()).toString());

    ui->cbOperationType->setCurrentIndex(settings.value(sTargetOperationTypeId, int()).toInt());
    ui->cbMemoryType->setCurrentIndex(settings.value(sTargetMemoryTypeId, int()).toInt());

    for (unsigned short i = 0; i < ui->tbOptions->rowCount(); i++)
    {
        if (settings.value(QString::number(i), QString()).toString().isEmpty()) continue;//if some value was not changed, then check next

        switch (getOptionType(i))
        {
            case additionalOptionsTypes::text:
            case additionalOptionsTypes::filePath:
                ui->tbOptions->model()->index(i, 1).data() = settings.value(QString::number(i), QString()).toString();
                break;

            case additionalOptionsTypes::logic:
                ui->tbOptions->cellWidget(i, 1)->findChild<QCheckBox*>(QString::number(i))->setCheckState(Qt::CheckState::Checked);
                break;

            case additionalOptionsTypes::itemList:
                ui->tbOptions->cellWidget(i, 1)->findChild<QComboBox*>(QString::number(i))->setCurrentIndex(settings.value(QString::number(i), int()).toInt());
                break;
        }
    }

    settings.endGroup();
    settings.endGroup();
}

void MainWindow::removeTarget()
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

    if (m_currentTarget.isEmpty()) return;//if no target is currently selected, then nothing will be removed

    settings.beginGroup(sTargetsGroup);

    if (settings.childGroups().indexOf(m_currentTarget) != -1)
    {
        settings.beginGroup(m_currentTarget);
        settings.remove("");
        settings.endGroup();
        removeTargetFromMenu(m_currentTarget);
    }

    settings.endGroup();

    m_currentTarget = QString();//update variable to indicate that no target is selected
}
