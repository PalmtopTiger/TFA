#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "srtwriter.h"
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileDialog>
#include <QDateTime>
#include <qmath.h>

void GetFirstChannel(WavReader::SamplesVector& samples, const int numChannels);
QString ToTimestamp(const uint utime);
QString UrlToPath(const QUrl &url);

const QString DEFAULT_DIR_KEY = "DefaultDir";
const QString THRESHOLD_KEY = "Threshold";
const QString MIN_INTERVAL_KEY = "MinInterval";
const QString MIN_LENGTH_KEY = "MinLength";


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

void MainWindow::openFile(const QString &fileName)
{
    this->_reader.open(fileName);
    if (_reader.hasErrors())
    {
        ui->btSave->setEnabled(false);
        ui->edInfo->clear();
        return;
    }
    _fileInfo.setFile(fileName);

    QDateTime dt;
    dt.setTime_t(static_cast<uint>(_reader.samples().size()) / _reader.format().sampleRate / _reader.format().numChannels + 61200u);
    ui->edInfo->setHtml(QString("<b>Имя файла:</b> %1<br/>"
                                "<b>Формат:</b> %2<br/>"
                                "<b>Продолжительность:</b> %3<br/>"
                                "<b>Битрейт:</b> %4 Кбит/сек<br/>"
                                "<b>Каналы:</b> %5<br/>"
                                "<b>Частота:</b> %6 КГц<br/>"
                                "<b>Битовая глубина:</b> %7 бит")
                        .arg(_fileInfo.fileName())
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
    for (const qint32 s : qAsConst(samples))
    {
        maxVol = qMax(maxVol, qAbs(s));
    }

    const qreal samplesInMsec = _reader.format().sampleRate * 0.001;
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
    const QString path = url.toLocalFile();

    if (!path.isEmpty() && QFileInfo(path).suffix().toLower() == "wav")
    {
        return path;
    }

    return QString::null;
}
