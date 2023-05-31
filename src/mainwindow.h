/*
 * This file is part of TFA.
 * Copyright (C) 2013-2019  Andrey Efremov <duxus@yandex.ru>
 *
 * TFA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TFA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TFA.  If not, see <https://www.gnu.org/licenses/>.
 */

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
    const QString DEFAULT_DIR_KEY  = "DefaultDir",
                  THRESHOLD_KEY    = "Threshold",
                  MIN_INTERVAL_KEY = "MinInterval",
                  MIN_LENGTH_KEY   = "MinLength";

    Ui::MainWindow *ui;
    QSettings _settings;
    QFileInfo _fileInfo;
    WavReader::WavReader _reader;

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    QMenu* createContextMenu();
    void openFile(const QString &fileName);
    void saveFile(const QString &fileName);
    QString urlToPath(const QUrl &url);
};

#endif // MAINWINDOW_H
