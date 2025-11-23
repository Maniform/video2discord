#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QFutureWatcher>
#include <QProcess>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

	QSettings settings;
	QFuture<QStringList> newFiles;
	QFutureWatcher<QStringList> newFilesWatcher;
	QProcess* process;
	bool stop = false;

	QStringList convertFiles(const QStringList& files);

signals:
	void processFileChanged(const QString& file);
	void progressIndexChanged(int value);

private slots:
	void on_openFilesPushButton_clicked();
	void on_openDestinationFolderPushButton_clicked();
	void on_convertPushButton_clicked();

	void convert_finished();
};
#endif // MAINWINDOW_H
