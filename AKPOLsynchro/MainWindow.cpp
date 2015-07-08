#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Utility.h"
#include <QRegExp>
#include <QInputDialog>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QTextCodec>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
	destPath(""),
	ti(nullptr),
	timenu(nullptr),
	tirestore(nullptr),
	ticlose(nullptr),
	logToFile(false),
	logF(nullptr),
	logStr(nullptr),
	syncMaxSize(SYNCHRO_MAXFILE_SIZE)
{
    ui->setupUi(this);

	//currPath = qApp->applicationDirPath();
	setWindowFlags( ( (this->windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint) );
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

	// tray icon
	if( QSystemTrayIcon::isSystemTrayAvailable() )
	{
		ti = new QSystemTrayIcon(this);
		connect( ti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)) );
		timenu = new QMenu("AKPOL synchronizator", this);
		tirestore = new QAction("Przywróć", timenu);
		timenu->addAction(tirestore);
		connect( tirestore, SIGNAL(triggered()), this, SLOT(restoreFromTray()) );
		ticlose = new QAction("Zamknij", timenu);
		connect( ticlose, SIGNAL(triggered()), this, SLOT(close()) );
		timenu->addSeparator();
		timenu->addAction(ticlose);
		ti->setContextMenu(timenu);
		ti->setIcon(QIcon(":/app/icons/app/Treetog-Junior-Sync.ico"));
		ti->show();
	}

	bool t = connect( &tim, SIGNAL(timeout()), this, SLOT(timeout()) );
	t = connect( ui->actionAbout, SIGNAL(triggered()), this, SLOT(aboutWindow()) );
	t = connect( ui->actionExit, SIGNAL(triggered()), qApp, SLOT(quit()) );
	t = connect( ui->actionSelectDest, SIGNAL(triggered()), this, SLOT(selectDest()) );
	t = connect( ui->actionSelectRemovables, SIGNAL(triggered()), this, SLOT(selectRemovables()) );
	t = connect( ui->actionSetTimeout, SIGNAL(triggered()), this, SLOT(setTimeout()) );
	t = connect( ui->scanAndCopy, SIGNAL(clicked()), this, SLOT(timeout()) );
	t = connect( qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()) );
	t = connect( ui->actionSetLog, SIGNAL(triggered()), this, SLOT(setLog()) );

	statusBarLabel.setText("");
	ui->statusBar->addWidget( &statusBarLabel );
	timeoutVal = CONFIG_TIMEOUT_DEF;
	tim.setInterval( timeoutVal * 1000 );
	tim.start();

	//noRemovableMedias.setText("Nie wykryto nośników wymiennych typu pendrive!");
	//removableMediaFoundStr = "Wykryto napędy wymienne: ";
	//removableMediaFound.setText(removableMediaFoundStr);

	if( !readConfig(qApp->applicationDirPath() + "/" + CONFIG_FILE_NAME) )
		writeConfig(qApp->applicationDirPath() + "/" + CONFIG_FILE_NAME);

	if( destPath == "" )
	{
		QMessageBox::information( this, "Katalog docelowy", "Nie wybrano katalogu docelowego, proszę go wybrać.", QMessageBox::Ok );
		log("Nie wybrano katalogu docelowego, proszę go wybrać.");
		selectDest();
	}

	if( logToFile )
	{
		turnLogOn();
		log("Uruchamianie aplikacji z parametrami: \r\n\t" + QString(CONFIG_DSTPATH) + destPath + 
			"\r\n\t" + QString(CONFIG_TIMEOUT) + QString::number(timeoutVal) +
			"\r\n\t" + QString(CONFIG_LOGTOFILE) + (logToFile ? "true" : "false") +
			"\r\n\t" + QString(CONFIG_MAXFILE_SIZE) + QString::number(SYNCHRO_MAXFILE_SIZE)
			);
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
	disconnect( tirestore, SIGNAL(triggered()), this, SLOT(restoreFromTray()) );
	disconnect( ti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)) );
	disconnect( ticlose, SIGNAL(triggered()), this, SLOT(close()) );
	disconnect( ui->actionSetLog, SIGNAL(triggered()), this, SLOT(setLog()) );

	log("Zamykanie aplikacji...\r\n");
	if( logF != nullptr )
	{
		logF->close();
		delete logF;
		if( logStr != nullptr )
			delete logStr;
	}
	
	if( ti != nullptr )
		delete ti;

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
		log(NO_REMOVABLE_MEDIA);
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
		log(REMOVABLE_MEDIA_FOUND + drives);
		
		ui->removableMedias->setText( drives );
	}
}

void MainWindow::copyFiles( const QString& path )
{
	QDir dir(path);
	auto files = dir.entryInfoList( QDir::Files | QDir::NoDot | QDir::NoDotAndDotDot | QDir::NoDotDot );
	QString logFiles;
	for( QFileInfoList::iterator itr = files.begin(); itr != files.end(); ++itr )
	{
		QFileInfo dstF( destPath + "/" + itr->fileName() );
		
		if( itr->suffix() == "" && /*itr->size() != dstF.size() &&*/ itr->lastModified() != dstF.lastModified() )
		{
			QFile f(itr->absoluteFilePath());
			if( f.size() > syncMaxSize )
			{
				ui->statusBar->showMessage("Błąd kopiowania pliku '" + itr->absoluteFilePath() + "' - rozmiar przekracza maksymalną wartość " + QString::number(syncMaxSize) + " bajtów!", STATUSBAR_TEMP_MSG_TIMEOUT );
				log("Błąd kopiowania pliku '" + itr->absoluteFilePath() + "' - rozmiar przekracza maksymalną wartość " + QString::number(syncMaxSize) + " bajtów!");
			}
			else
			{
				if( QFile::copy( itr->absoluteFilePath(), destPath + "/" + itr->fileName() ) )
					log("Skopiowano plik z '" + itr->absoluteFilePath() + "' do '" + destPath + "/" + itr->fileName() + "'");
				else
					log("Błąd kopiowania pliku z '" + itr->absoluteFilePath() + "' do '" + destPath + "/" + itr->fileName() + "'");
			}
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
	AboutWindow* a = new AboutWindow(this);
	a->setAttribute( Qt::WA_DeleteOnClose );
	a->ui.version->setText( a->ui.version->text().replace(APP_VERSION_ABOUT_REPLACE_STR, getVersionString()) );
	a->exec();
}

void MainWindow::selectDest( void )
{
	bool ok;
	auto dir = QFileDialog::getExistingDirectory( this, "Wybierz katalog docelowy", qApp->applicationDirPath() );
	if( dir == "" )
	{
		ui->statusBar->showMessage( "Nie wybrano katalogu docelowego...", STATUSBAR_TEMP_MSG_TIMEOUT );
		log("Nie wybrano katalogu docelowego...");
	}
	else
	{
		for( QStringList::iterator itr = removableMedias.begin(); itr != removableMedias.end(); ++itr )
		{
			if( *itr == dir )
			{
				ui->statusBar->showMessage( "Katalog docelowy nie może wskazywać na napęd wymienny...", STATUSBAR_TEMP_MSG_TIMEOUT );
				log("Katalog docelowy nie może wskazywać na napęd wymienny...");
				return;
			}
		}
		destPath = dir;
		ui->statusBar->showMessage( "Wybrano nowy katalog docelowy: " + destPath.replace("/","\\"), STATUSBAR_TEMP_MSG_TIMEOUT );
		log("Wybrano nowy katalog docelowy: " + destPath.replace("/","\\"));
	}

	writeConfig(qApp->applicationDirPath() + "/" + CONFIG_FILE_NAME);
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
		log("Okres skanowania zmieniono na: " + QString::number(timeout) + " sekund...");
	}
	else
	{
		ui->statusBar->showMessage( "Nie przestawiono okresu skanowania...", STATUSBAR_TEMP_MSG_TIMEOUT );
		log("Nie przestawiono okresu skanowania...");
	}

	writeConfig(qApp->applicationDirPath() + "/" + CONFIG_FILE_NAME);
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
			{
				ui->statusBar->showMessage( "Błąd odczytu pola 'katalog docelowy' w pliku konfiguracyjnym - przywracam ustawienia domyślne...", STATUSBAR_TEMP_MSG_TIMEOUT );
				log("Błąd odczytu pola 'katalog docelowy' w pliku konfiguracyjnym - przywracam ustawienia domyślne...");
			}
		}
		else if( data.contains( CONFIG_TIMEOUT ) )
		{
			timeoutVal = data.replace( CONFIG_TIMEOUT, "").replace("\r\n","").toInt();
			if( timeoutVal > CONFIG_TIMEOUT_MAX || timeoutVal < CONFIG_TIMEOUT_MIN )
			{
				timeoutVal = CONFIG_TIMEOUT_DEF;
				ui->statusBar->showMessage( "Błąd odczytu pola 'okres skanowania' w pliku konfiguracyjnym - przywracam ustawienia domyślne...", STATUSBAR_TEMP_MSG_TIMEOUT );
				log("Błąd odczytu pola 'okres skanowania' w pliku konfiguracyjnym - przywracam ustawienia domyślne...");
			}
		}
		else if( data.contains( CONFIG_LOGTOFILE ) )
		{
			QString tmp = data.replace( CONFIG_LOGTOFILE, "").replace("\r\n","");
			if( tmp == "true" )
				logToFile = true;
			else
				logToFile = false;
		}
		else if( data.contains( CONFIG_MAXFILE_SIZE ) )
		{
			QString tmp = data.replace( CONFIG_MAXFILE_SIZE, "").replace("\r\n","");
			bool ok;
			int tmpI = tmp.toInt(&ok);
			if( !ok )
			{
				ui->statusBar->showMessage( "Błąd oczytu pola 'maksymalny rozmiar pliku do synchronizacji' - przywracam ustawienia domyślne...", STATUSBAR_TEMP_MSG_TIMEOUT );
				log("Błąd oczytu pola 'maksymalny rozmiar pliku do synchronizacji' - przywracam ustawienia domyślne...");
			}
			else
				syncMaxSize = tmpI;
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
	QString logCfg;
	if( logToFile == true )
		logCfg = "true";
	else
		logCfg = "false";
	data.push_back(CONFIG_LOGTOFILE + logCfg.toLatin1() + "\r\n");
	data.push_back(CONFIG_MAXFILE_SIZE + QString::number(syncMaxSize).toLatin1() + "\r\n");
	f.write(data);
	f.close();
	log("Zapisano konfigurację do pliku...");

	return true;
}

void MainWindow::aboutToQuit( void )
{
	timeout();
	writeConfig(qApp->applicationDirPath() + "/" + CONFIG_FILE_NAME);
}

void MainWindow::changeEvent(QEvent * event)
{
	QMainWindow::changeEvent(event);
	if(event->type() == QEvent::WindowStateChange)
	{
		if(isMinimized())
			this->hide();
	}
}

void MainWindow::restoreFromTray( void )
{
	trayIconClicked(QSystemTrayIcon::Trigger);
}

void MainWindow::trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
	if(reason == QSystemTrayIcon::Trigger)
	{
        this->showNormal();
		this->activateWindow();
	}
}

QString MainWindow::getVersionString( void )
{
	return QString("v" + QString::number((appVer&0xFF0000)>>16) + "." + QString::number((appVer&0x00FF00)>>8) + "." + QString::number(appVer&0x0000FF));
}

void MainWindow::log( const QString& msg )
{
	if( !logToFile || logF == nullptr || logStr == nullptr )
		return;

	*logStr << QDateTime::currentDateTime().toString("[dd/MM/yyyy hh:mm:ss] ") << msg << "\r\n";
	logF->flush();
}

void MainWindow::setLog( void )
{
	auto ret = QMessageBox::information(this, "Ustawienia pliku logu", "Jeżeli chcesz włączyć logowanie do pliku naciśnij przycisk 'TAK', w przeciwnym razie naciśnij 'NIE'",
		QMessageBox::Yes, QMessageBox::No );
	if( ret == QMessageBox::Yes )
	{
		logToFile = true;
		turnLogOn();
	}
	else
		logToFile = false;

	writeConfig(qApp->applicationDirPath() + "/" + CONFIG_FILE_NAME);
}

void MainWindow::turnLogOn( void )
{
	if( logF != nullptr )
		return;
	logF = new QFile(qApp->applicationDirPath() + "/" + LOG_FILE_NAME);
	// sprawdzamy rozmiar pliku
	bool lastLogDeleted = false;
	if( logF->size() > LOG_MAX_SIZE )
	{
		logF->remove();
		lastLogDeleted = true;
	}

	if( !logF->open( QIODevice::WriteOnly | QIODevice::Append ) )
	{
		QMessageBox::warning( this, "Plik logu", "Błąd tworzenia pliku logu: '" + QString(LOG_FILE_NAME) + "'!", QMessageBox::Close );
		delete logF;
		logF = nullptr;
		logToFile = false;
	}
	else
	{
		logStr = new QTextStream(logF);
		logStr->setCodec("UTF-8");
		logStr->setGenerateByteOrderMark(true);
		log("Rozpoczynam logowanie do pliku...");
		if( lastLogDeleted )
			log("Stary log został skasowany ponieważ osiągnął maksymalny rozmiar " + QString::number(LOG_MAX_SIZE) + " bajtów.");
	}
}