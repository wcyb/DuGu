#include "optionsdialog.h"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
    setFixedSize(size());
    readSettingsFile();
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

bool OptionsDialog::saveSettingsFile()
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

    settings.beginGroup(sGroup);
    settings.setValue(sPath, ui->lePath->text());
    settings.setValue(sPartId, ui->cbPart->currentIndex());
    settings.setValue(sProgrammerId, ui->cbProgrammer->currentIndex());
    if (m_parts.size() && m_programmers.size()) //if parts and programmers were updated, then save them
    {
        settings.setValue(sParts, m_parts);
        settings.setValue(sProgrammers, m_programmers);
    }
    settings.endGroup();

    if (ui->chbDefault->checkState()) setDefaultTarget();
    else removeDefaultTarget();

    return true;
}

bool OptionsDialog::readSettingsFile()
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);
    int defaultTarget = -1;

    if (!settings.allKeys().size()) return false;//if settings file is empty, then nothing will be done

    settings.beginGroup(sGroup);

    ui->lePath->setText(settings.value(sPath, QString()).toString());
    m_pathToAvrDude = ui->lePath->text();
    if (ui->lePath->text().length() && QFileInfo(m_pathToAvrDude).exists()) ui->bInitialize->setEnabled(true);

    foreach (QVariant value, settings.value(sParts, QMap<QString, QVariant>()).toMap().values()) ui->cbPart->addItem(value.toString());
    foreach (QVariant value, settings.value(sProgrammers, QMap<QString, QVariant>()).toMap().values())
    {
        ui->cbProgrammer->addItem(value.toString());
        ui->cbProgrammer->setItemData(ui->cbProgrammer->count() - 1, ui->cbProgrammer->itemText(ui->cbProgrammer->count() - 1), Qt::ToolTipRole);//add name of programmer as tooltip
    }

    ui->cbPart->setCurrentIndex(settings.value(sPartId, int()).toInt());
    ui->cbProgrammer->setCurrentIndex(settings.value(sProgrammerId, int()).toInt());

    settings.endGroup();

    if (!ui->cbPart->count() || !ui->cbProgrammer->count()) return false; //if no parts or programmers were red from settings, then leave controls disabled and return false

    settings.beginGroup(sTargetsGroup);

    foreach (QString gr, settings.childGroups())
    {
        settings.beginGroup(gr);
        QString targetName(settings.value(sTargetName, QString()).toString());
        ui->cbTargetNames->addItem(targetName);
        ui->cbTargetDefault->addItem(targetName);

        if (settings.value(sTargetDefault, bool(false)).toBool()) defaultTarget = ui->cbTargetDefault->count() - 1;//if there is default target, save position of that target in combobox

        settings.endGroup();
        m_targetsNames.append(QPair<QString, QString>(gr, targetName));//add group name and target name to list
    }

    settings.endGroup();

    if (defaultTarget > -1)
    {
        ui->cbTargetDefault->setCurrentIndex(defaultTarget);
        ui->chbDefault->setCheckState(Qt::CheckState::Checked);
    }

    ui->cbPart->setEnabled(true);
    ui->cbProgrammer->setEnabled(true);

    if (ui->cbTargetNames->count() && ui->cbTargetDefault->count()) enableTargetControls(); //if we have some targets added, then we can enable targets section

    return true;
}

void OptionsDialog::setDefaultTarget(void)
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

    if (!ui->cbTargetDefault->count()) return;//if list is empty then ignore this setting

    removeDefaultTarget();

    settings.beginGroup(sTargetsGroup);
    settings.beginGroup(m_targetsNames[ui->cbTargetDefault->currentIndex()].first);

    settings.setValue(sTargetDefault, true);

    settings.endGroup();
    settings.endGroup();
}

void OptionsDialog::removeDefaultTarget()
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

    settings.beginGroup(sTargetsGroup);

    foreach (QString gr, settings.childGroups())
    {
        //remove "default" from any other group
        settings.beginGroup(gr);
        if (settings.value(sTargetDefault, bool(false)).toBool())
        {
            settings.remove(sTargetDefault);
            settings.endGroup();
            break;
        }
        settings.endGroup();
    }

    settings.endGroup();
}

void OptionsDialog::enableTargetControls()
{
    ui->cbTargetNames->setEnabled(true);
    ui->leTargetName->setEnabled(true);
    ui->bSaveName->setEnabled(true);
    ui->cbTargetDefault->setEnabled(true);
    ui->chbDefault->setEnabled(true);
}

void OptionsDialog::disableTargetControls()
{
    ui->cbTargetNames->setEnabled(false);
    ui->leTargetName->setEnabled(false);
    ui->bSaveName->setEnabled(false);
    ui->cbTargetDefault->setEnabled(false);
    ui->chbDefault->setEnabled(false);
}

void OptionsDialog::rejectChanges()
{
    reject();
}

void OptionsDialog::setPathToAvrDude()
{
    QString file(QFileDialog::getOpenFileName(this, tr("Select AVRDUDE"), QString(), "avrdude (*.exe)"));

    if (file.length())
    {
        ui->lePath->setText(file);
        m_pathToAvrDude = file;
        ui->bInitialize->setEnabled(true);
    }
}

void OptionsDialog::initializeAvrDudeData()
{
    QProcess ad(this);
    ad.setProcessChannelMode(QProcess::MergedChannels);

    if (m_pathToAvrDude.length())
    {
        ad.setProgram(m_pathToAvrDude);
        ad.setArguments(QStringList() << "-c?");//get information about programmers
        ad.start();
        ad.waitForFinished();

        if (ad.exitCode() < 0)
        {
            QMessageBox::critical(this, tr("Error"), tr("Error occured during execution of AVRDUDE."));
            return;
        }

        QString readedProgrammers(ad.readAllStandardOutput());

        QRegularExpression reProgrammers("([^\\s\\=][a-zA-Z0-9.,':\\(\\)<>/_!=\\- ]+[^\\s\\=])");//re to get lines with data
        QRegularExpressionMatchIterator matchesProgrammers = reProgrammers.globalMatch(readedProgrammers);

        if (!matchesProgrammers.hasNext())
        {
            QMessageBox::critical(this, tr("Error"), tr("Error occured during execution of AVRDUDE."));
            return;
        }

        matchesProgrammers.next();//ignore first line
        m_programmers.clear();
        while (matchesProgrammers.hasNext()) //for each line add it to combobox and settings
        {
            QRegularExpressionMatch match = matchesProgrammers.next();

            ui->cbProgrammer->addItem(match.captured(0).section('=', 1, -1).simplified()); //get description
            ui->cbProgrammer->setItemData(ui->cbProgrammer->count() - 1, ui->cbProgrammer->itemText(ui->cbProgrammer->count() - 1), Qt::ToolTipRole); //add tooltip for each programmer
            m_programmers.insert(match.captured(0).section('=', 0, 0).simplified(), ui->cbProgrammer->itemText(ui->cbProgrammer->count() - 1)); //set argument-description pairs
        }

        ad.setArguments(QStringList() << "-p?");//get information about parts
        ad.start();
        ad.waitForFinished();

        if (ad.exitCode() < 0)
        {
            QMessageBox::critical(this, tr("Error"), tr("Error occured during execution of AVRDUDE."));
            return;
        }

        QString readedParts(ad.readAllStandardOutput());

        QRegularExpression reParts("([^\\s\\=][a-zA-Z0-9,:'= ]+[^\\s\\=])");//re to get lines with data
        QRegularExpressionMatchIterator matchesParts = reParts.globalMatch(readedParts);

        if (!matchesParts.hasNext())
        {
            QMessageBox::critical(this, tr("Error"), tr("Error occured during execution of AVRDUDE."));
            return;
        }

        matchesParts.next();//ignore first line
        m_parts.clear();
        while (matchesParts.hasNext()) //for each line add it to combobox and settings
        {
            QRegularExpressionMatch match = matchesParts.next();
            if (match.captured(0).contains("deprecated", Qt::CaseSensitivity::CaseInsensitive)) continue; //ignore lines with obsolete parts

            ui->cbPart->addItem(match.captured(0).section('=', 1, -1).simplified()); //get description
            m_parts.insert(match.captured(0).section('=', 0, 0).simplified(), ui->cbPart->itemText(ui->cbPart->count() - 1)); //set argument-description pairs
        }

        ui->cbPart->setEnabled(true);
        ui->cbProgrammer->setEnabled(true);
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), tr("Path to AVRDUDE is incorrect."));
    }
}

void OptionsDialog::changeTargetName()
{
    QSettings settings(sSettingsFileName, QSettings::IniFormat, this);

    int targetIndex(ui->cbTargetNames->currentIndex());
    QString targetNewName(ui->leTargetName->text());

    if (!targetNewName.length())
    {
        QMessageBox::critical(this, tr("Error"), tr("Length of new name must be greater than 0."));
        return;
    }
    if (!ui->cbTargetNames->count())
    {
        QMessageBox::critical(this, tr("Error"), tr("First you need to create a new target using option from main menu."));
        return;
    }

    settings.beginGroup(sTargetsGroup);
    settings.beginGroup(m_targetsNames[targetIndex].first);

    settings.setValue(sTargetName, targetNewName);

    settings.endGroup();
    settings.endGroup();

    m_targetsNames[targetIndex].second = targetNewName;
    ui->cbTargetNames->setItemText(targetIndex, targetNewName);
    ui->cbTargetDefault->setItemText(targetIndex, targetNewName);
}

void OptionsDialog::saveSettings()
{
    saveSettingsFile();
    accept();
}
