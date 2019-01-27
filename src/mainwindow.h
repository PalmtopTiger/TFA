#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "wavreader.h"
#include <QMainWindow>
#include <QSettings>
#include <QFileInfo>

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
    void on_tbInfo_customContextMenuRequested(const QPoint& pos);
    void on_tbInfo_customHeaderContextMenuRequested(const QPoint& pos);
    void copyInfo();

private:
    Ui::MainWindow *ui;
    QSettings _settings;
    QFileInfo _fileInfo;
    WavReader::WavReader _reader;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    QMenu* createContextMenu();
    void openFile(const QString &fileName);
    void saveFile(const QString &fileName);
};

#endif // MAINWINDOW_H
