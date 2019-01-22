#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "wavreader.h"
#include <QMainWindow>
#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btOpen_clicked();
    void on_btSave_clicked();

private:
    Ui::MainWindow *ui;
    QSettings _settings;
    QString _fileName;
    WavReader::WavReader _reader;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void openFile(const QString &fileName);
    void saveFile(const QString &fileName);
};

#endif // MAINWINDOW_H
