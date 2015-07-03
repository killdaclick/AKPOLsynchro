#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Utility.h"
#include <QRegExp>
#include <QInputDialog>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
	destPath("")
{
    ui->setupUi(this);

	bool t = connect( &tim, SIGNAL(timeout()), this, SLOT(timeout()) );
	t = connect( ui->actionAbout, SIGNAL(triggered()), this, SLOT(aboutWindow()) );
	t = connect( ui->actionExit, SIGNAL(triggered()), qApp, SLOT(quit()) );
	t = connect( ui->actionSelectDest, SIGNAL(triggered()), this, SLOT(selectDest()) );
	t = connect( ui->actionSelectRemovables, SIGNAL(triggered()), this, SLOT(selectRemovables()) );
	t = connect( ui->actionSetTimeout, SIGNAL(triggered()), this, SLOT(setTimeout()) );
	t = connect( ui->scanAndCopy, SIGNAL(clicked()), this, SLOT(timeout()) );
	t = connect( qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()) );

	statusBarLabel.setText("");
	ui->statusBar->addWidget( &statusBarLabel );
	timeoutVal = SCAN_DEFAULT_TIMEOUT;
	tim.setInterval( timeoutVal * 1000 );
	tim.start();

	//noRemovableMedias.setText("Nie wykryto nośników wymiennych typu pendrive!");
	//removableMediaFoundStr = "Wykryto napędy wymienne: ";
	//removableMediaFound.setText(removableMediaFoundStr);

	if( !readConfig(QDir::currentPath() + "/" + CONFIG_FILE_NAME) )
		writeConfig(QDir::currentPath() + "/" + CONFIG_FILE_NAME);

	if( destPath == "" )
	{
		QMessageBox::information( this, "Katalog docelowy", "Nie wybrano katalogu docelowego, proszę go wybrać.", QMessageBox::Ok );
		selectDest();
	}
	
	timeout();
}

MainWindow::~MainWindow()
{
    disconnect( &tim, SIGNAL(timeout()), this, SLOT(timeout()) );
	disconnect( ui->actionAbout, SIGNAL(triggered()), this, SLOT(aboutWindow()) );
	disconnect( ui->actionExit, SIGNAL(triggered()), qApp, SLOT(quit()) );
	disconnect( ui->actionSelectDest, SIGNAL(triggered()), this, SLOT(selectDest()) );
	disconnect( ui->actionSelectRemovables, SIGNAL(triggered()), this, SLOT(selectRemovables()) );
	disconnect( ui->actionSetTimeout, SIGNAL(triggered()), this, SLOT(setTimeout()) );
	disconnect( ui->scanAndCopy, SIGNAL(clicked()), this, SLOT(timeout()) );
	disconnect( qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()) );
	
	delete ui;
}

void MainWindow::scanMedia( void )
{
	// dodajemy napedy wymienialne do listy
	removableMedias = Utility::getRemovableMedia();
	if( removableMedias.count() == 0 )
	{
		statusBarLabel.setText( NO_REMOVABLE_MEDIA );
		ui->removableMedias->setText( "" );
		ui->scanAndCopy->setEnabled(false);
	}
	else
	{
		QString drives;
		for( QStringList::iterator itr = removableMedias.begin(); itr != removableMedias.end(); ++itr )
		{
			drives += (*itr).replace("/","\\");
			if( itr + 1 != removableMedias.end() )
				drives += " ;";
		}
		statusBarLabel.setText( REMOVABLE_MEDIA_FOUND + drives );
		ui->scanAndCopy->setEnabled(true);
		
		ui->removableMedias->setText( drives );
	}
}

void MainWindow::copyFiles( const QString& path )
{
	QDir dir(path);
	auto files = dir.entryInfoList( QDir::Files | QDir::NoDot | QDir::NoDotAndDotDot | QDir::NoDotDot );
	for( QFileInfoList::iterator itr = files.begin(); itr != files.end(); ++itr )
	{
		QFileInfo dstF( destPath + "/" + itr->fileName() );

		auto t1  = dstF.size();
		auto t2 = itr->size();
		auto t3 = dstF.lastModified().time().minute();
		auto t4 = itr->lastModified().time().minute();
		auto t5 = itr->absoluteFilePath();

		if( itr->suffix() == "" && /*itr->size() != dstF.size() &&*/ itr->lastModified() != dstF.lastModified() )
		{
			QFile::copy( itr->absoluteFilePath(), destPath + "/" + itr->fileName() );
		}
	}
}

void MainWindow::timeout( void )
{
	scanMedia();
	if( destPath != "" )
	{
		for( QStringList::iterator itr = removableMedias.begin(); itr != removableMedias.end(); ++itr )
			copyFiles( *itr );
	}
}

void MainWindow::aboutWindow( void )
{

}

void MainWindow::selectDest( void )
{
	bool ok;
	auto dir = QFileDialog::getExistingDirectory( this, "Wybierz katalog docelowy", QDir::currentPath() );
	if( dir == "" )
		ui->statusBar->showMessage( "Nie wybrano katalogu docelowego...", STATUSBAR_TEMP_MSG_TIMEOUT );
	else
	{
		for( QStringList::iterator itr = removableMedias.begin(); itr != removableMedias.end(); ++itr )
		{
			if( *itr == dir )
			{
				ui->statusBar->showMessage( "Katalog docelowy nie może wskazywać na napęd wymienny...", STATUSBAR_TEMP_MSG_TIMEOUT );
				return;
			}
		}
		destPath = dir;
		ui->statusBar->showMessage( "Wybrano nowy katalog docelowy: " + destPath.replace("/","\\"), STATUSBAR_TEMP_MSG_TIMEOUT );
	}
}

void MainWindow::selectRemovables( void )
{

}

void MainWindow::setTimeout( void )
{
	bool ok;
	int timeout = QInputDialog::getInt( this, "Okres skanowania", "Podaj co ile sekund skanować napędy wymienne:", 
		CONFIG_TIMEOUT_DEF, CONFIG_TIMEOUT_MIN, CONFIG_TIMEOUT_MAX, 1, &ok );
	if( ok )
	{
		timeout = timeout * 1000;
		tim.setInterval( timeout );
		tim.stop();
		tim.start();
		ui->statusBar->showMessage( "Okres skanowania zmieniono na: " + QString::number(timeout) + " sekund...", STATUSBAR_TEMP_MSG_TIMEOUT );
	}
	else
		ui->statusBar->showMessage( "Nie przestawiono okresu skanowania...", STATUSBAR_TEMP_MSG_TIMEOUT );
}

bool MainWindow::readConfig( const QString& path )
{
	QFile f( path );
	if( !f.open( QIODevice::ReadOnly) )
	{
		ui->statusBar->showMessage( "Błąd odczytu pliku konfiguracyjnego - przywracam ustawienia domyślne...", STATUSBAR_TEMP_MSG_TIMEOUT );
		return false;
	}

	QByteArray data = f.readLine();
	while( !data.isEmpty() )
	{
		if( data.contains( CONFIG_DSTPATH ) )
		{
			destPath = data.replace(CONFIG_DSTPATH,"").replace("\r\n","");
			QDir destDir(destPath);
			if( !destDir.exists() )
				ui->statusBar->showMessage( "Błąd odczytu pola 'katalog docelowy' w pliku konfiguracyjnym - przywracam ustawienia domyślne...", STATUSBAR_TEMP_MSG_TIMEOUT );
		}
		else if( data.contains( CONFIG_TIMEOUT ) )
		{
			timeoutVal = data.replace( CONFIG_TIMEOUT, "").replace("\r\n","").toInt();
			if( timeoutVal > CONFIG_TIMEOUT_MAX || timeoutVal < CONFIG_TIMEOUT_MIN )
			{
				timeoutVal = CONFIG_TIMEOUT_DEF;
				ui->statusBar->showMessage( "Błąd odczytu pola 'okres skanowania' w pliku konfiguracyjnym - przywracam ustawienia domyślne...", STATUSBAR_TEMP_MSG_TIMEOUT );
			}
		}
		data = f.readLine();
	}
	f.close();

	return true;
}

bool MainWindow::writeConfig( const QString& path )
{
	QFile f( path );
	if( !f.open( QIODevice::WriteOnly ) )
	{
		ui->statusBar->showMessage( "Błąd tworzenia pliku konfiguracyjnego - ustawienia nie zostaną zapisane!", STATUSBAR_TEMP_MSG_TIMEOUT );
		return false;
	}

	QByteArray data;
	data.push_back(CONFIG_DSTPATH + destPath.toLatin1() + "\r\n");
	data.push_back(CONFIG_TIMEOUT + QString::number(timeoutVal).toLatin1() + "\r\n");
	f.write(data);
	f.close();

	return true;
}

void MainWindow::aboutToQuit( void )
{
	timeout();
	writeConfig(QDir::currentPath() + "/" + CONFIG_FILE_NAME);
}
