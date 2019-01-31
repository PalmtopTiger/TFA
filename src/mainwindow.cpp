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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "srtwriter.h"
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileDialog>
#include <QDateTime>
#include <QMenu>
#include <QClipboard>
#include <QtMath>

void GetFirstChannel(WavReader::SamplesVector& samples, const int numChannels);
QString UrlToPath(const QUrl &url);

const QString DEFAULT_DIR_KEY  = "DefaultDir",
              THRESHOLD_KEY    = "Threshold",
              MIN_INTERVAL_KEY = "MinInterval",
              MIN_LENGTH_KEY   = "MinLength";


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tbInfo->setVerticalHeaderLabels({
        "Имя файла",
        "Формат",
        "Продолжительность",
        "Битрейт",
        "Каналы",
        "Частота",
        "Битовая глубина"
    });
    ui->tbInfo->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tbInfo->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tbInfo->verticalHeader(), &QHeaderView::customContextMenuRequested, this, &MainWindow::on_tbInfo_customHeaderContextMenuRequested);

    ui->spinThreshold->setValue(_settings.value(THRESHOLD_KEY, ui->spinThreshold->value()).toDouble());
    ui->spinMinInterval->setValue(_settings.value(MIN_INTERVAL_KEY, ui->spinMinInterval->value()).toInt());
    ui->spinMinLength->setValue(_settings.value(MIN_LENGTH_KEY, ui->spinMinLength->value()).toInt());

    this->move(QApplication::desktop()->screenGeometry().center() - this->rect().center());
}

MainWindow::~MainWindow()
{
    _settings.setValue(THRESHOLD_KEY, ui->spinThreshold->value());
    _settings.setValue(MIN_INTERVAL_KEY, ui->spinMinInterval->value());
    _settings.setValue(MIN_LENGTH_KEY, ui->spinMinLength->value());

    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && !UrlToPath(event->mimeData()->urls().first()).isEmpty())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        QString path = UrlToPath(event->mimeData()->urls().first());
        if (!path.isEmpty()) {
            this->openFile(path);
            event->acceptProposedAction();
        }
    }
}


void MainWindow::on_btOpen_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          "Выберите аудио",
                                                          _settings.value(DEFAULT_DIR_KEY).toString(),
                                                          "Microsoft WAV (*.wav)");
    if (fileName.isEmpty()) return;

    _settings.setValue(DEFAULT_DIR_KEY, QFileInfo(fileName).absolutePath());
    this->openFile(fileName);
}

void MainWindow::on_btSave_clicked()
{
    if (_reader.hasErrors()) return;

    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          "Выберите выходной файл",
                                                          _fileInfo.dir().filePath(_fileInfo.completeBaseName() + ".srt"),
                                                          "SubRip (*.srt)");
    if (fileName.isEmpty()) return;

    this->saveFile(fileName);
}

void MainWindow::on_tbInfo_customContextMenuRequested(const QPoint& pos)
{
    this->createContextMenu()->popup(ui->tbInfo->viewport()->mapToGlobal(pos));
}

void MainWindow::on_tbInfo_customHeaderContextMenuRequested(const QPoint& pos)
{
    this->createContextMenu()->popup(ui->tbInfo->verticalHeader()->viewport()->mapToGlobal(pos));
}

QMenu* MainWindow::createContextMenu()
{
    QMenu* const menu = new QMenu(this);
    QAction* const actCopy = new QAction("Копировать", this);
    connect(actCopy, &QAction::triggered, this, &MainWindow::copyInfo);
    menu->addAction(actCopy);
    return menu;
}

void MainWindow::copyInfo()
{
    QStringList data;
    for (int i = 0; i < ui->tbInfo->rowCount(); ++i) {
        data.append(QString("%1: %2")
                    .arg(ui->tbInfo->verticalHeaderItem(i)->text())
                    .arg(ui->tbInfo->item(i, 0)->text()));
    }

    QApplication::clipboard()->setText(data.join('\n'));
}

void MainWindow::openFile(const QString &fileName)
{
    this->_reader.open(fileName);
    if (_reader.hasErrors())
    {
        ui->tbInfo->setEnabled(false);
        ui->btSave->setEnabled(false);
        ui->tbInfo->clearContents();
        return;
    }
    _fileInfo.setFile(fileName);

    const WavReader::FormatSubChunk& format = _reader.format();
    QDateTime dt;
    dt.setTime_t(static_cast<uint>(_reader.samples().size()) / format.sampleRate / format.numChannels + 61200u);

    ui->tbInfo->setItem(0, 0, new QTableWidgetItem(_fileInfo.fileName()));
    ui->tbInfo->setItem(1, 0, new QTableWidgetItem(format.audioFormat == 1 ? "PCM" : "неизвестен"));
    ui->tbInfo->setItem(2, 0, new QTableWidgetItem(dt.toString("HH:mm:ss")));
    ui->tbInfo->setItem(3, 0, new QTableWidgetItem(QString("%1 Кбит/сек").arg(format.byteRate * 0.008)));
    ui->tbInfo->setItem(4, 0, new QTableWidgetItem(QString::number(format.numChannels)));
    ui->tbInfo->setItem(5, 0, new QTableWidgetItem(QString("%1 КГц").arg(format.sampleRate * 0.001)));
    ui->tbInfo->setItem(6, 0, new QTableWidgetItem(QString("%1 бит").arg(format.bitsPerSample)));

    ui->tbInfo->setEnabled(true);
    ui->btSave->setEnabled(true);
}

void MainWindow::saveFile(const QString &fileName)
{
    const WavReader::FormatSubChunk& format = _reader.format();
    WavReader::SamplesVector samples = _reader.samples();
    if (format.numChannels > 1)
    {
        GetFirstChannel(samples, format.numChannels);
    }

    qint32 maxVol = 0;
    for (const qint32 s : qAsConst(samples))
    {
        maxVol = qMax(maxVol, qAbs(s));
    }

    const qreal samplesInMsec = format.sampleRate * 0.001;
    const qint32 threshold = qFloor(maxVol * ui->spinThreshold->value() * 0.01);
    const int minInterval = qRound(ui->spinMinInterval->value() * samplesInMsec);
    const int minLength = ui->spinMinLength->value();
    bool inPhrase = false;
    int countdown = minInterval;
    uint lastSeenTime = 0, num = 1;
    SrtWriter::Phrase phrase;
    SrtWriter::SrtWriter writer;
    for (int i = 0, len = samples.size(); i < len; ++i)
    {
        if (qAbs(samples.at(i)) >= threshold)
        {
            lastSeenTime = static_cast<uint>(qRound(i / samplesInMsec));
            countdown = minInterval;

            if (!inPhrase)
            {
                inPhrase = true;
                phrase.time.first = lastSeenTime;
                phrase.text = QString::number(num);
            }
        }
        else if (inPhrase)
        {
            if (countdown > 0)
            {
                --countdown;
            }
            else
            {
                inPhrase = false;
                phrase.time.second = lastSeenTime + 1;
                if (static_cast<int>(phrase.time.second) - static_cast<int>(phrase.time.first) >= minLength)
                {
                    writer.addPhrase(phrase);
                    ++num;
                }
            }
        }
    }
    if (inPhrase)
    {
        phrase.time.second = lastSeenTime + 1;
        if (static_cast<int>(phrase.time.second) - static_cast<int>(phrase.time.first) >= minLength)
        {
            writer.addPhrase(phrase);
        }
    }

    writer.save(fileName);
}


void GetFirstChannel(WavReader::SamplesVector& samples, const int numChannels)
{
    for (int i = 0, j = 0, len = samples.size(); i < len; i += numChannels, ++j)
    {
        samples[j] = samples[i];
    }
    samples.resize(samples.size() / numChannels);
}

QString UrlToPath(const QUrl &url)
{
    if (url.isLocalFile()) {
        const QString path = url.toLocalFile();
        if (QFileInfo(path).suffix().toLower() == "wav") {
            return path;
        }
    }
    return QString::null;
}
