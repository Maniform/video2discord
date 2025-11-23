#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QtConcurrent>

#include <QFileDialog>
#include <QMessageBox>

#define DISCORD_MAX_VIDEO_SIZE 10000000

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
	, settings("Manicorp", "video2discord", this)
	, ffmpeg("ffmpeg")
	, process(nullptr)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
	ffmpeg = QApplication::applicationDirPath() + "/../Resources/ffmpeg";
#elif defined Q_OS_WIN
	ffmpeg = QApplication::applicationDirPath() + "/ffmpeg.exe";
#endif

	ui->destinationFolderLineEdit->setText(settings.value("DestinationFolder", "").toString());

	ui->label->hide();
	ui->progressBar->hide();

	connect(this, &MainWindow::processFileChanged, ui->label, &QLabel::setText);
	connect(this, &MainWindow::progressIndexChanged, ui->progressBar, &QProgressBar::setValue);
}

MainWindow::~MainWindow()
{
	stop = true;
	if (process)
	{
		process->terminate();
		while (newFiles.isRunning())
		{
			QThread::msleep(10);
		}
	}

    delete ui;
}

QStringList MainWindow::convertFiles(const QStringList& fileNames)
{
	QStringList result;
	const QString destinationFolder = ui->destinationFolderLineEdit->text();
	uint8_t index = 0;
	for (const QString& fileName : fileNames)
	{
		index++;
		emit progressIndexChanged(index);
		const QFileInfo fileInfo(fileName);
		QFileInfo currentFileInfo(fileName);
		int crf = 23;

		if (currentFileInfo.size() <= DISCORD_MAX_VIDEO_SIZE)
		{
			QFile::copy(fileName, destinationFolder + "/" + fileInfo.fileName());
		}

		while (currentFileInfo.size() > DISCORD_MAX_VIDEO_SIZE && crf <= 51 && !stop)
		{
			const QString oldFileName = destinationFolder + "/" + fileInfo.baseName() + "-" + QString::number(crf) + "." + fileInfo.suffix();
			if (QFile::exists(oldFileName))
			{
				QFile::remove(oldFileName);
				crf++;
			}

			emit processFileChanged(QString("%1/%2 %3 (crf %4)").arg(index).arg(fileNames.size()).arg(fileInfo.fileName()).arg(crf));

			const QString newFileName = destinationFolder + "/" + fileInfo.baseName() + "-" + QString::number(crf) + "." + fileInfo.suffix();
			QStringList args;
			args << "-y";
			args << "-i" << fileName;
			args << "-vcodec" << "libx264";
			args << "-crf" << QString::number(crf);
			args << "-acodec" << "copy";
			args << newFileName;

			process = new QProcess;
			process->setProgram(ffmpeg);
			process->setArguments(args);
			process->start();
			process->waitForStarted(-1);
			process->waitForFinished(-1);
			QFile log(QDir::homePath() + "/log.txt");
			if (log.open(QFile::WriteOnly | QFile::Text))
			{
				QTextStream out(&log);
				out << process->readAll() << Qt::endl;
				out << process->exitCode() << " " << process->exitStatus();
				log.close();
			}
			process->deleteLater();
			process = nullptr;

			if (stop)
			{
				QFile::remove(newFileName);
				return result;
			}

			currentFileInfo = QFileInfo(newFileName);
			if (fileInfo.size() <= DISCORD_MAX_VIDEO_SIZE)
			{
				result << newFileName;
			}
		}
	}

	return result;
}

void MainWindow::on_openFilesPushButton_clicked()
{
	const QString videoFormat = "*.mp4 *.mkv *.avi *.mov";
	const QStringList inFileNames = QFileDialog::getOpenFileNames(this, tr("Ouvrir un fichier vidéo"), settings.value("InVideoFolder", QString()).toString(), QString("Fichier vidéo (%1)").arg(videoFormat));
	if (!inFileNames.isEmpty())
	{
		QFileInfo fileInfo(inFileNames[0]);
		settings.setValue("InVideoFolder", fileInfo.absolutePath());

		ui->inFilesListWidget->clear();
		for (const QString& fileName : inFileNames)
		{
			ui->inFilesListWidget->addItem(fileName);
		}
	}
}

void MainWindow::on_openDestinationFolderPushButton_clicked()
{
	const QString destinationFolder = QFileDialog::getExistingDirectory(this, tr("Dossier de destination"), settings.value("DestinationFolder", QString()).toString());
	if (!destinationFolder.isEmpty())
	{
		settings.setValue("DestinationFolder", destinationFolder);
		ui->destinationFolderLineEdit->setText(destinationFolder);
	}
}

void MainWindow::on_convertPushButton_clicked()
{
	QStringList files;
	for (uint8_t i = 0; i < ui->inFilesListWidget->count(); ++i)
	{
		files << ui->inFilesListWidget->item(i)->text();
	}

	if (files.isEmpty())
	{
		QMessageBox::warning(this, tr("Warning"), tr("Aucun fichier spécifié."));
		return;
	}

	ui->convertPushButton->setDisabled(true);
	ui->label->show();
	ui->progressBar->setMaximum(files.size());
	ui->progressBar->setValue(0);
	ui->progressBar->show();

	newFiles = QtConcurrent::run(&MainWindow::convertFiles, this, files);
	newFilesWatcher.setFuture(newFiles);
	connect(&newFilesWatcher, &QFutureWatcher<QStringList>::finished, this, &MainWindow::convert_finished);
}

void MainWindow::convert_finished()
{
	disconnect(&newFilesWatcher, &QFutureWatcher<QStringList>::finished, this, &MainWindow::convert_finished);
	for (const QString& fileName : newFiles.result())
	{
		const QFileInfo newFileInfo(fileName);
		qDebug() << fileName << " : " << newFileInfo.size();
	}

	ui->label->hide();
	ui->progressBar->hide();

	QApplication::beep();
	QMessageBox::information(this, tr("Terminé"), tr("Les fichiers ont été convertis."));
	ui->convertPushButton->setEnabled(true);
}
