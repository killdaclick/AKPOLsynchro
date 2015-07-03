#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>

#define NO_REMOVABLE_MEDIA		"Nie wykryto nośników wymiennych typu pendrive!"
#define REMOVABLE_MEDIA_FOUND	"Wykryto napędy wymienne: "
#define STATUSBAR_TEMP_MSG_TIMEOUT		3000
#define SCAN_DEFAULT_TIMEOUT			60

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

private:
    Ui::MainWindow *ui;
	QTimer tim;
	QString destPath;
	QStringList removableMedias;
	int timeoutMs;
	QLabel statusBarLabel;

public slots:
	void timeout( void );
	void aboutWindow( void );
	void selectDest( void );
	void selectRemovables( void );
	void setTimeout( void );

};

#endif // MAINWINDOW_H
