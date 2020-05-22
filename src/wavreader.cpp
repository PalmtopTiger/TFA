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
    _hasErrors = true;

    std::memset(&_format, 0, sizeof(FormatSubChunk));
    _samples.clear();
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

    std::memcpy(&(_format), format.constData(), sizeof(FormatSubChunk));

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
    switch (_format.audioFormat)
    {
    case PCM_INT:
        switch (_format.bitsPerSample)
        {
        case 32:
        {
            _samples.reserve(static_cast<int>(data.size() / sizeof(qint32)));
            qint32 sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(qint32)) == sizeof(qint32))
            {
                _samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint32>::max());
            }
            break;
        }

        case 24:
        {
            const int sampleSize = sizeof(qint32) - 1;
            _samples.reserve(static_cast<int>(data.size() / sampleSize));
            qint32 sample = 0; // Зануление необходимо
            char* samplePtr = reinterpret_cast<char*>(&sample) + 1; // Знаковый бит попадает куда нужно
            while (data.read(samplePtr, sampleSize) == sampleSize)
            {
                _samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint32>::max());
            }
            break;
        }

        case 16:
        {
            _samples.reserve(static_cast<int>(data.size() / sizeof(qint16)));
            qint16 sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(qint16)) == sizeof(qint16))
            {
                _samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint16>::max());
            }
            break;
        }

        case 8:
        {
            _samples.reserve(static_cast<int>(data.size()));
            quint8 sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(quint8)) == sizeof(quint8))
            {
                // 128 в 8-битных WAV означает 0; qint8 - не ошибка, т.к. приводим к знаковому типу
                _samples.append((static_cast<qreal>(sample) - 128.0) / std::numeric_limits<qint8>::max());
            }
            break;
        }

        default:
            data.close();
            QMessageBox::critical(nullptr, "Ошибка", "Неправильный размер сэмпла.");
            return;
        }
        break;

    case PCM_FLOAT:
        switch (_format.bitsPerSample)
        {
        case 64:
        {
            _samples.reserve(static_cast<int>(data.size() / sizeof(double)));
            double sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(double)) == sizeof(double))
            {
                _samples.append(static_cast<qreal>(sample));
            }
            break;
        }

        case 32:
        {
            _samples.reserve(static_cast<int>(data.size() / sizeof(float)));
            float sample;
            while (data.read(reinterpret_cast<char*>(&sample), sizeof(float)) == sizeof(float))
            {
                _samples.append(static_cast<qreal>(sample));
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

    _hasErrors = false;
}

bool WavReader::hasErrors()
{
    return _hasErrors;
}

void WavReader::toMono()
{
    if (_format.numChannels < 2)
    {
        return;
    }

    for (int i = 0, j = 0, len = _samples.size(); i < len; i += _format.numChannels, ++j)
    {
        _samples[j] = (_samples[i] + _samples[i + 1]) / 2.0;
    }
    _samples.resize(_samples.size() / _format.numChannels);
    _format.numChannels = 1u;
}

const FormatSubChunk& WavReader::format()
{
    return _format;
}

const SamplesVector &WavReader::samples()
{
    return _samples;
}
}
