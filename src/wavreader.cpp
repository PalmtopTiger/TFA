#include "wavreader.h"
#include <QMessageBox>
#include <QFile>
#include <QDataStream>
#include <QMap>
#include <QByteArray>

namespace WavReader
{
WavReader::WavReader()
{
    this->clear();
}

WavReader::WavReader(const QString &fileName)
{
    this->open(fileName);
}

void WavReader::clear()
{
    this->_hasErrors = true;

    qMemSet(&this->_format, 0, sizeof(FormatSubChunk));
    this->_samples.clear();
}

void WavReader::open(const QString &fileName)
{
    this->clear();

    // Открытие файла
    QFile fin(fileName);
    if (!fin.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(NULL, "Ошибка", "Не могу открыть файл для чтения.");
        return;
    }
    if (static_cast<size_t>(fin.size()) < (sizeof(ChunkHeader) + sizeof(FormatSubChunk) + sizeof(SubChunkHeader) * 2))
    {
        QMessageBox::critical(NULL, "Ошибка", "Файл подозрительно мал.");
        fin.close();
        return;
    }
    QDataStream in(&fin);

    // Чтение заголовка первого чанка
    ChunkHeader header;
    if (in.readRawData((char*)&header, sizeof(ChunkHeader)) < 0)
    {
        QMessageBox::critical(NULL, "Ошибка", "Ошибка чтения.");
        fin.close();
        return;
    }

    if (qstrncmp((const char*)&header.ID, RIFF_ID, sizeof(header.ID)) != 0)
    {
        QMessageBox::critical(NULL, "Ошибка", "Не найден заголовок RIFF.");
        fin.close();
        return;
    }
    if (static_cast<size_t>(fin.size()) < (sizeof(SubChunkHeader) + header.size))
    {
        QMessageBox::critical(NULL, "Ошибка", "Реальный размер файла меньше, чем указанный в заголовке.");
        fin.close();
        return;
    }
    if (qstrncmp((const char*)&header.format, WAVE, sizeof(header.format)) != 0)
    {
        QMessageBox::critical(NULL, "Ошибка", "Файл RIFF не является файлом WAV.");
        fin.close();
        return;
    }

    // Чтение сабчанков
    QMap<QString, QByteArray> subchunks;
    QMap<QString, QByteArray>::iterator it;
    SubChunkHeader subheader;
    char temp[sizeof(subheader.ID) + 1] = {0};
    QString key;
    while(!in.atEnd())
    {
        if (in.readRawData((char*)&subheader, sizeof(SubChunkHeader)) < 0)
        {
            QMessageBox::critical(NULL, "Ошибка", "Ошибка чтения.");
            fin.close();
            return;
        }

        qMemCopy(temp, subheader.ID, sizeof(subheader.ID));
        key = temp;

        it = subchunks.find(key);
        if (it == subchunks.end())
        {
            it = subchunks.insert(key, QByteArray());
        }
        else
        {
            QMessageBox::warning(NULL, "Ошибка", "Повторяющаяся структура: \"" + key + "\"");
        }

        it->resize(subheader.size);

        if (in.readRawData(it->data(), subheader.size) < 0)
        {
            QMessageBox::critical(NULL, "Ошибка", "Ошибка чтения.");
            fin.close();
            return;
        }
    }
    fin.close();

    //return;

    // Разбор секции FORMAT
    it = subchunks.find(FORMAT_ID);
    if (it == subchunks.end())
    {
        QMessageBox::critical(NULL, "Ошибка", "Секция FORMAT не найдена.");
        return;
    }
    if (static_cast<size_t>(it->size()) < sizeof(FormatSubChunk))
    {
        QMessageBox::critical(NULL, "Ошибка", "Секция FORMAT неверного размера.");
        return;
    }

    qMemCopy(&(this->_format), it->data(), sizeof(FormatSubChunk));

    if (1 != this->_format.audioFormat)
    {
        QMessageBox::critical(NULL, "Ошибка", "Программа поддерживает только несжатые WAV.");
        return;
    }

    // Разбор секции DATA
    it = subchunks.find(DATA_ID);
    if (it == subchunks.end())
    {
        QMessageBox::critical(NULL, "Ошибка", "Секция DATA не найдена.");
        return;
    }
    if (it->size() < 1)
    {
        QMessageBox::critical(NULL, "Ошибка", "Секция DATA пуста.");
        return;
    }

    if (16 == this->_format.bitsPerSample)
    {
        this->_samples.resize(it->size() / sizeof(quint16));

        qint16 tempShort;
        char* byte1 = (char*)&tempShort;
        char* byte2 = byte1 + 1;
        for (size_t i = 0, j = 0, len = it->size(); i < len; i += sizeof(quint16), ++j)
        {
            *byte1 = it->at(i);
            *byte2 = it->at(i + 1);
            this->_samples[j] = qAbs(tempShort);
        }
    }
    else if (8 == this->_format.bitsPerSample)
    {
        this->_samples.resize(it->size());

        qint16 tempShort;
        for (size_t i = 0, len = it->size(); i < len; ++i)
        {
            tempShort = static_cast<uchar>(it->at(i));
            this->_samples[i] = qAbs(tempShort - 128) << 8;
        }
    }
    else
    {
        QMessageBox::critical(NULL, "Ошибка", "Программа поддерживает только 8 и 16-бититные WAV.");
        return;
    }

this->_hasErrors = false;
}

bool WavReader::hasErrors()
{
    return this->_hasErrors;
}

const FormatSubChunk& WavReader::format()
{
    return this->_format;
}

const SamplesVector &WavReader::samples()
{
    return this->_samples;
}
}
