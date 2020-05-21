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
#include <QBuffer>
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

    // Разбор секции DATA
    if (!subchunks.contains(ID_DATA))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция DATA не найдена.");
        return;
    }

    if (!subchunks[ID_DATA].size())
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция DATA пуста.");
        return;
    }

    QBuffer data(&subchunks[ID_DATA]);
    data.open(QIODevice::ReadOnly);
    switch (this->_format.audioFormat)
    {
    case FMT_INT:
        switch (this->_format.bitsPerSample)
        {
        case 32:
        {
            this->_samples.reserve(static_cast<int>(data.size() / sizeof(qint32)));
            qint32 sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(qint32)) == sizeof(qint32))
            {
                this->_samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint32>::max());
            }
            break;
        }

        case 24:
        {
            const int sampleSize = sizeof(qint32) - 1;
            this->_samples.reserve(static_cast<int>(data.size() / sampleSize));
            qint32 sample = 0; // Зануление необходимо
            char* samplePtr = reinterpret_cast<char*>(&sample) + 1; // Знаковый бит попадает куда нужно
            while (data.read(samplePtr, sampleSize) == sampleSize)
            {
                this->_samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint32>::max());
            }
            break;
        }

        case 16:
        {
            this->_samples.reserve(static_cast<int>(data.size() / sizeof(qint16)));
            qint16 sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(qint16)) == sizeof(qint16))
            {
                this->_samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint16>::max());
            }
            break;
        }

        case 8:
        {
            this->_samples.reserve(static_cast<int>(data.size()));
            quint8 sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(quint8)) == sizeof(quint8))
            {
                // 128 в 8-битных WAV означает 0; qint8 - не ошибка, т.к. приводим к знаковому типу
                this->_samples.append((static_cast<qreal>(sample) - 128.0) / std::numeric_limits<qint8>::max());
            }
            break;
        }

        default:
            data.close();
            QMessageBox::critical(nullptr, "Ошибка", "Неправильный размер сэмпла.");
            return;
        }
        break;

    case FMT_FLOAT:
        switch (this->_format.bitsPerSample)
        {
        case 64:
        {
            this->_samples.reserve(static_cast<int>(data.size() / sizeof(double)));
            double sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(double)) == sizeof(double))
            {
                this->_samples.append(static_cast<qreal>(sample));
            }
            break;
        }

        case 32:
        {
            this->_samples.reserve(static_cast<int>(data.size() / sizeof(float)));
            float sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(float)) == sizeof(float))
            {
                this->_samples.append(static_cast<qreal>(sample));
            }
            break;
        }

        default:
            data.close();
            QMessageBox::critical(nullptr, "Ошибка", "Неправильный размер сэмпла.");
            return;
        }
        break;

    default:
        data.close();
        QMessageBox::critical(nullptr, "Ошибка", "Программа поддерживает только несжатые WAV.");
        return;
    }
    data.close();

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
