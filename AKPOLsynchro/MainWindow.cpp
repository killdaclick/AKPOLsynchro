#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Utility.h"
#include <QRegExp>
#include <QInputDialog>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
	destPath("")
{
    ui->setupUi(this);

	connect( &tim, SIGNAL(timeout()), this, SLOT(timeout()) );
	connect( ui->actionAbout, SIGNAL(triggered()), this, SLOT(aboutWindow()) );
	connect( ui->actionExit, SIGNAL(triggered()), qApp, SLOT(quit()) );
	connect( ui->actionSelectDest, SIGNAL(triggered()), this, SLOT(selectDest()) );
	connect( ui->actionSelectRemovables, SIGNAL(triggered()), this, SLOT(selectRemovables()) );
	connect( ui->actionSetTimeout, SIGNAL(triggered()), this, SLOT(setTimeout()) );
	connect( ui->scanAndCopy, SIGNAL(clicked()), this, SLOT(timeout()) );

	ui->statusBar->addWidget( &statusBarLabel );
	tim.setInterval( SCAN_DEFAULT_TIMEOUT * 1000 );
	tim.start();

	//noRemovableMedias.setText("Nie wykryto nośników wymiennych typu pendrive!");
	//removableMediaFoundStr = "Wykryto napędy wymienne: ";
	//removableMediaFound.setText(removableMediaFoundStr);

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
		if( itr->suffix() == "" )
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
	int timeout = QInputDialog::getInt( this, "Okres skanowania", "Podaj co ile sekund skanować napędy wymienne:", 60, 15, 3600, 1, &ok );
	if( ok )
	{
		timeoutMs = timeout * 1000;
		tim.setInterval( timeoutMs );
		tim.stop();
		tim.start();
		ui->statusBar->showMessage( "Okres skanowania zmieniono na: " + QString::number(timeout) + " sekund...", STATUSBAR_TEMP_MSG_TIMEOUT );
	}
	else
		ui->statusBar->showMessage( "Nie przestawiono okresu skanowania...", STATUSBAR_TEMP_MSG_TIMEOUT );
}
