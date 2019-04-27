#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include "settingsnames.h"

namespace Ui
{
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog();

private:
    Ui::OptionsDialog *ui;

private:
    QString m_pathToAvrDude;
    //map with argument and name for parts
    QMap<QString, QVariant> m_parts;
    //map with argument and name for programmers
    QMap<QString, QVariant> m_programmers;
    //index of default part in combobox
    short m_defaultPart = 0;
    //index of default programmer in combobox
    short m_defaultProgrammer = 0;
    //list of pairs of name from settings and name in menu of targets
    QList<QPair<QString, QString>> m_targetsNames;

private:
    bool saveSettingsFile(void);
    bool readSettingsFile(void);
    //sets new default target and removes any previous selected as default
    void setDefaultTarget(void);
    void removeDefaultTarget(void);
    void enableTargetControls(void);
    void disableTargetControls(void);

private slots:
    void rejectChanges(void);
    void saveSettings(void);
    void setPathToAvrDude(void);
    //reads supported parts and programmers from AVRDUDE
    void initializeAvrDudeData(void);
    //changes name of selected target
    void changeTargetName(void);
};

#endif // OPTIONSDIALOG_H
