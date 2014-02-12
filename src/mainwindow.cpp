#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "srtwriter.h"
#include <QMessageBox>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QUrl>
#include <QSettings>
#include <QFileDialog>
#include <QDateTime>
#include <qmath.h>

void GetFirstChannel(WavReader::SamplesVector& samples, size_t numChannels);
QString ToTimestamp(const uint utime);
QString UrlToPath(const QUrl &url);

const QString DEFAULT_DIR_KEY = "DefaultDir";


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->move(QApplication::desktop()->screenGeometry().center() - this->rect().center());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
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
    QSettings settings;
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Выберите аудио",
                                                    settings.value(DEFAULT_DIR_KEY).toString(),
                                                    "Microsoft WAV (*.wav)");

    if (!fileName.isEmpty())
    {
        settings.setValue(DEFAULT_DIR_KEY, QFileInfo(fileName).absolutePath());
        this->openFile(fileName);
    }
}

void MainWindow::on_btSave_clicked()
{
    if (_reader.hasErrors()) return;

    QString fileName = _fileName.replace(QRegExp("\\.[^.]*$"), "").append(".srt");
    fileName = QFileDialog::getSaveFileName(this,
                                            "Выберите выходной файл",
                                            fileName,
                                            "SubRip (*.srt)");

    if (!fileName.isEmpty())
    {
        this->saveFile(fileName);
    }
}

void MainWindow::openFile(const QString &fileName)
{
    this->_reader.open(fileName);
    if (_reader.hasErrors())
    {
        ui->btSave->setEnabled(false);
        ui->edInfo->clear();
        _fileName.clear();
        return;
    }
    _fileName = fileName;

    QDateTime dt;
    dt.setTime_t(_reader.samples().size() / _reader.format().sampleRate / _reader.format().numChannels + 61200);
    ui->edInfo->setHtml(QString("<b>Имя файла:</b> %1<br/>"
                                "<b>Формат:</b> %2<br/>"
                                "<b>Продолжительность:</b> %3<br/>"
                                "<b>Битрейт:</b> %4 Кбит/сек<br/>"
                                "<b>Каналы:</b> %5<br/>"
                                "<b>Частота:</b> %6 КГц<br/>"
                                "<b>Битовая глубина:</b> %7 бит")
                        .arg(QFileInfo(_fileName).fileName())
                        .arg(_reader.format().audioFormat == 1 ? QString("PCM") : QString("неизвестен"))
                        .arg(dt.toString("HH:mm:ss"))
                        .arg(_reader.format().byteRate * 0.008)
                        .arg(_reader.format().numChannels)
                        .arg(_reader.format().sampleRate * 0.001)
                        .arg(_reader.format().bitsPerSample));

    ui->btSave->setEnabled(true);
}

void MainWindow::saveFile(const QString &fileName)
{
    WavReader::SamplesVector samples = _reader.samples();
    if (_reader.format().numChannels > 1)
    {
        GetFirstChannel(samples, _reader.format().numChannels);
    }

    qint32 maxVol = 0;
    foreach (qint32 s, samples)
    {
        maxVol = qMax(maxVol, qAbs(s));
    }

    const qreal samplesInMsec = _reader.format().sampleRate * 0.001;
    const qint32 threshold = qFloor(maxVol * ui->spinThreshold->value() * 0.01);
    const int trustInterval = qRound(ui->spinTrustInterval->value() * samplesInMsec);
    const int minLength = ui->spinMinLength->value();
    bool inPhrase = false;
    int countdown = trustInterval;
    size_t lastSeenTime = 0, num = 1;
    SrtWriter::Phrase phrase;
    SrtWriter::SrtWriter writer;
    for (size_t i = 0, len = samples.size(); i < len; ++i)
    {
        if (qAbs(samples.at(i)) >= threshold)
        {
            lastSeenTime = qRound(i / samplesInMsec);
            countdown = trustInterval;

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


void GetFirstChannel(WavReader::SamplesVector& samples, size_t numChannels)
{
    for (size_t i = 0, j = 0, len = samples.size(); i < len; i += numChannels, ++j)
    {
        samples[j] = samples[i];
    }
    samples.resize(samples.size() / numChannels);
}

QString UrlToPath(const QUrl &url)
{
    QString path = url.toLocalFile();

    if (!path.isEmpty() && QFileInfo(path).suffix().toLower() == "wav")
    {
        return path;
    }

    return QString::null;
}
