#include "wavreader.h"
#include <QMessageBox>
#include <QFile>
#include <QDataStream>
#include <QMap>
#include <QByteArray>
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

        std::memcpy(temp, subheader.ID, sizeof(subheader.ID));
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

    std::memcpy(&(this->_format), it->data(), sizeof(FormatSubChunk));

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

    qint32 sample;
    char* byte0 = (char*)&sample;
    char* byte1 = byte0 + 1;
    char* byte2 = byte0 + 2;
    char* byte3 = byte0 + 3;
    const char signBit = 0b10000000, maxChar = 0b11111111;
    switch (this->_format.bitsPerSample)
    {
    case 32:
        this->_samples.resize(it->size() / 4);
        for (size_t i = 0, j = 0, len = it->size(); i < len; i += 4, ++j)
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
        for (size_t i = 0, j = 0, len = it->size(); i < len; i += 3, ++j)
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
        for (size_t i = 0, j = 0, len = it->size(); i < len; i += 2, ++j)
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
        for (size_t i = 0, len = it->size(); i < len; ++i)
        {
            *byte0 = it->at(i);
            *byte1 = *byte2 = *byte3 = 0;

            sample -= 128; // Центрируем

            this->_samples[i] = sample << 24; // Приводим к 32 битам
        }
        break;

    default:
        QMessageBox::critical(NULL, "Ошибка", "Программа поддерживает только целочисленные WAV.");
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
