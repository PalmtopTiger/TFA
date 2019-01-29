#include "wavreader.h"
#include <QMessageBox>
#include <QFile>
#include <QMap>
#include <cstring>

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

    std::memset(&this->_format, 0, sizeof(FormatSubChunk));
    this->_samples.clear();
}

void WavReader::open(const QString &fileName)
{
    this->clear();

    // Открытие файла
    QFile fin(fileName);
    if (!fin.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Не могу открыть файл для чтения.");
        return;
    }
    if (static_cast<size_t>(fin.size()) < (sizeof(ChunkHeader) + sizeof(FormatSubChunk) + sizeof(SubChunkHeader) * 2))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Файл подозрительно мал.");
        fin.close();
        return;
    }
    QDataStream in(&fin);

    // Чтение заголовка первого чанка
    ChunkHeader header;
    if (in.readRawData(reinterpret_cast<char*>(&header), sizeof(ChunkHeader)) < 0)
    {
        QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
        fin.close();
        return;
    }

    if (ID_RIFF != header.id)
    {
        QMessageBox::critical(nullptr, "Ошибка", "Не найден заголовок RIFF.");
        fin.close();
        return;
    }
    if (static_cast<size_t>(fin.size()) < (sizeof(SubChunkHeader) + header.size))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Реальный размер файла меньше, чем указанный в заголовке.");
        fin.close();
        return;
    }
    if (FMT_WAVE != header.format)
    {
        QMessageBox::critical(nullptr, "Ошибка", "Файл RIFF не является файлом WAV.");
        fin.close();
        return;
    }

    // Чтение сабчанков
    QMap<quint32, QByteArray> subchunks;
    QMap<quint32, QByteArray>::iterator it;
    SubChunkHeader subheader;
    while(!in.atEnd())
    {
        if (in.readRawData(reinterpret_cast<char*>(&subheader), sizeof(SubChunkHeader)) < 0)
        {
            QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
            fin.close();
            return;
        }

        it = subchunks.find(subheader.id);
        if (it == subchunks.end())
        {
            it = subchunks.insert(subheader.id, QByteArray());
        }
        else
        {
            QMessageBox::warning(nullptr, "Ошибка", QString("Повторяющаяся структура: %1").arg(subheader.id, 0, 16));
        }

        it->resize(static_cast<int>(subheader.size));

        if (in.readRawData(it->data(), static_cast<int>(subheader.size)) < 0)
        {
            QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
            fin.close();
            return;
        }
    }
    fin.close();

    // Разбор секции FORMAT
    it = subchunks.find(ID_FORMAT);
    if (it == subchunks.end())
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция FORMAT не найдена.");
        return;
    }
    if (static_cast<size_t>(it->size()) < sizeof(FormatSubChunk))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция FORMAT неверного размера.");
        return;
    }

    std::memcpy(&(this->_format), it->data(), sizeof(FormatSubChunk));

    if (1 != this->_format.audioFormat)
    {
        QMessageBox::critical(nullptr, "Ошибка", "Программа поддерживает только несжатые WAV.");
        return;
    }

    // Разбор секции DATA
    it = subchunks.find(ID_DATA);
    if (it == subchunks.end())
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция DATA не найдена.");
        return;
    }
    if (it->size() < 1)
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция DATA пуста.");
        return;
    }

    qint32 sample;
    char* byte0 = reinterpret_cast<char*>(&sample);
    char* byte1 = byte0 + 1;
    char* byte2 = byte0 + 2;
    char* byte3 = byte0 + 3;
    const char signBit = '\x80'; // 0b10000000
    const char maxChar = '\xFF'; // 0b11111111
    switch (this->_format.bitsPerSample)
    {
    case 32:
        this->_samples.resize(it->size() / 4);
        for (int i = 0, j = 0, len = it->size(); i < len; i += 4, ++j)
        {
            *byte0 = it->at(i);
            *byte1 = it->at(i + 1);
            *byte2 = it->at(i + 2);
            *byte3 = it->at(i + 3);

            this->_samples[j] = sample;
        }
        break;

    case 24:
        this->_samples.resize(it->size() / 3);
        for (int i = 0, j = 0, len = it->size(); i < len; i += 3, ++j)
        {
            *byte0 = it->at(i);
            *byte1 = it->at(i + 1);
            *byte2 = it->at(i + 2);

            if (*byte2 & signBit)
            {
                *byte3 = maxChar;
            }
            else
            {
                *byte3 = 0;
            }

            this->_samples[j] = sample << 8; // Приводим к 32 битам
        }
        break;

    case 16:
        this->_samples.resize(it->size() / 2);
        for (int i = 0, j = 0, len = it->size(); i < len; i += 2, ++j)
        {
            *byte0 = it->at(i);
            *byte1 = it->at(i + 1);

            if (*byte1 & signBit)
            {
                *byte2 = *byte3 = maxChar;
            }
            else
            {
                *byte2 = *byte3 = 0;
            }

            this->_samples[j] = sample << 16; // Приводим к 32 битам
        }
        break;

    case 8:
        this->_samples.resize(it->size());
        for (int i = 0, len = it->size(); i < len; ++i)
        {
            *byte0 = it->at(i);
            *byte1 = *byte2 = *byte3 = 0;

            sample -= 128; // Центрируем

            this->_samples[i] = sample << 24; // Приводим к 32 битам
        }
        break;

    default:
        QMessageBox::critical(nullptr, "Ошибка", "Программа поддерживает только целочисленные WAV.");
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
