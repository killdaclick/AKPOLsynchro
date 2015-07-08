#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QFile>
#include <QTextStream>
#include "AboutWindow.h"

#define NO_REMOVABLE_MEDIA		"Nie wykryto nośników wymiennych typu pendrive!"
#define REMOVABLE_MEDIA_FOUND	"Wykryto napędy wymienne: "
#define STATUSBAR_TEMP_MSG_TIMEOUT		3000
#define CONFIG_FILE_NAME		"config.ini"
#define CONFIG_DSTPATH			"destinationPath = "
#define CONFIG_TIMEOUT			"timeout = "
#define CONFIG_LOGTOFILE		"logToFile = "
#define CONFIG_MAXFILE_SIZE		"synchroMaxSize = "
#define CONFIG_TIMEOUT_MIN		15
#define CONFIG_TIMEOUT_MAX		3600
#define CONFIG_TIMEOUT_DEF		300
#define CONFIG_LOGTOFILE_DEF	"false"
#define APP_VERSION_ABOUT_STR	"<html><head/><body><p><span style=' font-size:12pt; font-weight:600;'>VER_REPLACE</span></p></body></html>"
#define APP_VERSION_ABOUT_REPLACE_STR	"VER_REPLACE"
#define LOG_FILE_NAME			"log.txt"
#define LOG_MAX_SIZE			50000000
#define SYNCHRO_MAXFILE_SIZE	50000000

const int appVer = 0x010000;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

	void scanMedia( void );
	void copyFiles( const QString& path );
	bool readConfig( const QString& path );
	bool writeConfig( const QString& path );
	QString getVersionString( void );
	void log( const QString& msg );
	void turnLogOn( void );

private:
    Ui::MainWindow *ui;
	QTimer tim;
	QString destPath;
	QStringList removableMedias;
	int timeoutVal;
	QLabel statusBarLabel;
	QSystemTrayIcon* ti;
	QMenu* timenu;
	QAction* tirestore;
	QAction* ticlose;
	bool logToFile;
	QFile* logF;
	QTextStream* logStr;
	int syncMaxSize;

protected:
	void changeEvent(QEvent * event);

public slots:
	void timeout( void );
	void aboutWindow( void );
	void selectDest( void );
	void selectRemovables( void );
	void setTimeout( void );
	void aboutToQuit( void );
	void restoreFromTray( void );
	void trayIconClicked(QSystemTrayIcon::ActivationReason reason);
	void setLog( void );

};

#endif // MAINWINDOW_H
