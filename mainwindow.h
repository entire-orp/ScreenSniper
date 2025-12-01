#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QJsonObject>
#include "screenshotwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString getText(const QString &key, const QString &defaultText = "") const;

private slots:
    void onCaptureScreen();
    void onCaptureArea();
    void onCaptureWindow();
    void onSettings();
    void onAbout();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void setupUI();
    void setupTrayIcon();
    void setupConnections();
    void loadLanguageSettings();
    void saveLanguageSettings();
    void switchLanguage(const QString &language);
    void updateUI();

    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QString currentLanguage;  // 当前语言设置: "zh", "en", "zhHK"
    QJsonObject translations; // 当前语言的翻译数据
};

#endif // MAINWINDOW_H
