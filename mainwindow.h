#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "optionsdialog.h"
#include "settingsnames.h"
#include <QActionGroup>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    //types of widgets that can be added to table with options
    enum class additionalOptionsTypes : uint8_t { logic, text, filePath, itemList };

private:
    //updates fusebytes placeholder text
    void updateFusePlaceholderText(QRegularExpressionMatchIterator* regexIterator, QWidget* fuseByteLineEdit);
    //after each operation updates values of fusebytes if they are empty
    void updateFusebytesValues(QString adOutput);
    //creates and populates table with additional options
    void addAdditionalOptions(void);
    //first arg is name of target from settings, second is name displayed in menu
    void addTargetToMenu(QString targetName, QString nameInMenu);
    //arg is name of target from settings
    void removeTargetFromMenu(QString targetName);
    //if arg is true then interface will be updated with new data
    void readSettings(bool updateCurrentData = false);
    //returns file type for AVRDUDE depending on provided path to file
    QString getFileType(QString pathToFile) const;
    //returns path to selected file and creates it in case user wants to save to given file
    QString getFile(void);

    /*returns description and widged depending on selected option from additional options
    in case if selected option has type text, then description will be empty
    if option number is incorrect, empty string and nullptr will be returned*/
    QPair<QString, QWidget*> getOptionWidget(unsigned short optionNumber);
    //returns argument for AVRDUDE for given option
    QString getOptionArg(unsigned short optionNumber) const;
    //returns option type for given option
    additionalOptionsTypes getOptionType(unsigned short optionNumber) const;
    //returns additional options count
    unsigned short getOptionsCount(void) const;
    //returns formatted option that is ready to add as argument to AVRDUDE
    QStringList getOptionWithArg(unsigned short optionNumber,
                                 QString argument,
                                 QString operationType,
                                 QString valueType) const;

private slots:
    void showOptions(void);
    void showAbout(void);
    void addTarget(void);
    void changeTarget(bool);
    void changeTarget(QString newTarget = QString());
    void removeTarget(void);
    void setPathToDataFile(void);
    void changeCheckboxState(int);
    void selectFile(void);
    void startOperation(void);
    void updateOutputInformation(void);

private:
    Ui::MainWindow* ui;

private:
    //pointer to AVRDUDE process
    QProcess* m_ad;
    //currently selected target name
    QString m_currentTarget;
    //pointer to targets group in menu
    QActionGroup* m_targetsGroup;
    QString m_pathToFirmware;
    //map with argument and name for parts
    QMap<QString, QVariant> m_parts;
    //map with argument and name for programmers
    QMap<QString, QVariant> m_programmers;

    const QStringList m_operationTypes = {tr("Write"), tr("Read"), tr("Verify")};

    const QStringList m_memoryTypes = {tr("Flash"), tr("EEPROM"), tr("Other")};

    const QStringList m_additionalOptionsArg = {"-b",          "-B",      "-C",          "-D",       "-e",
                                                "-E",          "-F",      "-i",          "-O",       "-u",
                                                "calibration", "lock",    "signature",   "fuse0",    "fuse1",
                                                "fuse2",       "fuse3",   "application", "apptable", "boot",
                                                "prodsig",     "usersig", "-V",          "-x",       ""};

    const QStringList m_additionalOptionsDesc = {tr("Baudrate"),
                                                 tr("Bitclock"),
                                                 tr("Config file"),
                                                 tr("Disable auto erase"),
                                                 tr("Chip erase"),
                                                 tr("Parallel port exit state"),
                                                 tr("Override signature check"),
                                                 tr("BitBang delay"),
                                                 tr("RC osc. calibration"),
                                                 tr("Disable fuse checking"),
                                                 tr("RC osc. calibration data"),
                                                 tr("Lock byte"),
                                                 tr("Device signature"),
                                                 tr("ATxmega fuse 0"),
                                                 tr("ATxmega fuse 1"),
                                                 tr("ATxmega fuse 2"),
                                                 tr("ATxmega fuse 3"),
                                                 tr("ATxmega app area"),
                                                 tr("ATxmega app table"),
                                                 tr("ATxmega boot"),
                                                 tr("ATxmega prod. signature"),
                                                 tr("ATxmega user signature"),
                                                 tr("Disable auto verify"),
                                                 tr("Ext. param. for programmer"),
                                                 tr("Custom arguments")};

    const QStringList m_additionalOptionsToolTip = {tr("Baudrate"),
                                                    tr("Bitclock"),
                                                    tr("Select config file"),
                                                    tr("Disable auto erase"),
                                                    tr("Chip erase"),
                                                    tr("Parallel port exit state"),
                                                    tr("Override signature check"),
                                                    tr("BitBang delay"),
                                                    tr("Perform RC oscillator calibration"),
                                                    tr("Disable fuse checking"),
                                                    tr("RC oscillator calibration data"),
                                                    tr("Lock byte"),
                                                    tr("Device signature"),
                                                    tr("ATxmega fuse 0"),
                                                    tr("ATxmega fuse 1"),
                                                    tr("ATxmega fuse 2"),
                                                    tr("ATxmega fuse 3"),
                                                    tr("Select ATxmega application area data"),
                                                    tr("Select ATxmega application table data"),
                                                    tr("Select ATxmega boot data"),
                                                    tr("ATxmega production signature"),
                                                    tr("ATxmega user signature"),
                                                    tr("Disable auto verify after writing"),
                                                    tr("Extended parameters for programmer"),
                                                    tr("Custom arguments")};

    //used only when additional options list is loaded for first time (program launch) to keep track of which m_itemLists is needed
    short m_currentList = 0;

    //list of lists with items for each combobox used in additional options table
    const QStringList m_itemLists[1][1] = {{QStringList{"", "reset", "noreset", "vcc", "novcc", "d_high", "d_low"}}};

    const additionalOptionsTypes m_additionalOptionsValues[25]
        = {additionalOptionsTypes::text,     additionalOptionsTypes::text,     additionalOptionsTypes::filePath,
           additionalOptionsTypes::logic,    additionalOptionsTypes::logic,    additionalOptionsTypes::itemList,
           additionalOptionsTypes::logic,    additionalOptionsTypes::text,     additionalOptionsTypes::logic,
           additionalOptionsTypes::logic,    additionalOptionsTypes::text,     additionalOptionsTypes::text,
           additionalOptionsTypes::text,     additionalOptionsTypes::text,     additionalOptionsTypes::text,
           additionalOptionsTypes::text,     additionalOptionsTypes::text,     additionalOptionsTypes::filePath,
           additionalOptionsTypes::filePath, additionalOptionsTypes::filePath, additionalOptionsTypes::text,
           additionalOptionsTypes::text,     additionalOptionsTypes::logic,    additionalOptionsTypes::text,
           additionalOptionsTypes::text};
};

#endif // MAINWINDOW_H
