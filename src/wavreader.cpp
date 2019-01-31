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

#include "wavreader.h"
#include <QMessageBox>
#include <QFile>
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
        QMessageBox::critical(nullptr, "Ошибка", "Не могу открыть файл для чтения.");
        return;
    }
    if (static_cast<size_t>(fin.size()) < (sizeof(ChunkHeader) + sizeof(FormatSubChunk) + sizeof(SubChunkHeader) * 2))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Файл подозрительно мал.");
        fin.close();
        return;
    }

    // Чтение заголовка первого чанка
    ChunkHeader header;
    if (fin.read(reinterpret_cast<char*>(&header), sizeof(ChunkHeader)) != sizeof(ChunkHeader))
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
    SubChunkHeader subheader;
    while(!fin.atEnd())
    {
        if (fin.read(reinterpret_cast<char*>(&subheader), sizeof(SubChunkHeader)) != sizeof(SubChunkHeader))
        {
            QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
            fin.close();
            return;
        }

        if (subchunks.contains(subheader.id))
        {
            QMessageBox::warning(nullptr, "Предупреждение", QString("Повторяющаяся структура: %1").arg(subheader.id, 0, 16));
        }

        QByteArray &chunk = subchunks[subheader.id] = fin.read(subheader.size);
        if (chunk.size() != static_cast<int>(subheader.size))
        {
            QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
            fin.close();
            return;
        }
    }
    fin.close();

    // Разбор секции FORMAT
    if (!subchunks.contains(ID_FORMAT))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция FORMAT не найдена.");
        return;
    }

    QByteArray &format = subchunks[ID_FORMAT];
    if (static_cast<size_t>(format.size()) < sizeof(FormatSubChunk))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция FORMAT неверного размера.");
        return;
    }

    std::memcpy(&(this->_format), format.constData(), sizeof(FormatSubChunk));

    if (1 != this->_format.audioFormat)
    {
        QMessageBox::critical(nullptr, "Ошибка", "Программа поддерживает только несжатые WAV.");
        return;
    }

    // Разбор секции DATA
    if (!subchunks.contains(ID_DATA))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция DATA не найдена.");
        return;
    }

    QByteArray &data = subchunks[ID_DATA];
    if (!data.size())
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
        this->_samples.resize(data.size() / 4);
        for (int i = 0, j = 0, len = data.size(); i < len; i += 4, ++j)
        {
            *byte0 = data.at(i);
            *byte1 = data.at(i + 1);
            *byte2 = data.at(i + 2);
            *byte3 = data.at(i + 3);

            this->_samples[j] = sample;
        }
        break;

    case 24:
        this->_samples.resize(data.size() / 3);
        for (int i = 0, j = 0, len = data.size(); i < len; i += 3, ++j)
        {
            *byte0 = data.at(i);
            *byte1 = data.at(i + 1);
            *byte2 = data.at(i + 2);

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
        this->_samples.resize(data.size() / 2);
        for (int i = 0, j = 0, len = data.size(); i < len; i += 2, ++j)
        {
            *byte0 = data.at(i);
            *byte1 = data.at(i + 1);

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
        this->_samples.resize(data.size());
        for (int i = 0, len = data.size(); i < len; ++i)
        {
            *byte0 = data.at(i);
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
