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

    std::memset(&_format, 0, sizeof(FormatChunk));
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

    // Чтение заголовка
    ChunkHeader header;
    if (fin.read(reinterpret_cast<char*>(&header), sizeof(ChunkHeader)) != sizeof(ChunkHeader))
    {
        fin.close();
        QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
        return;
    }

    if (ID_RIFF != header.id)
    {
        fin.close();
        QMessageBox::critical(nullptr, "Ошибка", "Не найден заголовок RIFF.");
        return;
    }
    const qint64 fileSize = sizeof(ChunkHeader) + header.size;
    if (fin.size() < fileSize)
    {
        fin.close();
        QMessageBox::critical(nullptr, "Ошибка", "Реальный размер файла меньше, чем указанный в заголовке.");
        return;
    }

    quint32 fileFormat;
    if (fin.read(reinterpret_cast<char*>(&fileFormat), sizeof(quint32)) != sizeof(quint32))
    {
        fin.close();
        QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
        return;
    }
    if (FMT_WAVE != fileFormat)
    {
        fin.close();
        QMessageBox::critical(nullptr, "Ошибка", "Файл RIFF не является файлом WAV.");
        return;
    }

    // Чтение секций
    bool hasFormat = false, hasData = false;
    qint64 chunkEnd = 0;
    while(!fin.atEnd() && fin.pos() < fileSize)
    {
        if (fin.read(reinterpret_cast<char*>(&header), sizeof(ChunkHeader)) != sizeof(ChunkHeader))
        {
            fin.close();
            QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
            return;
        }
        if (fin.bytesAvailable() < header.size)
        {
            fin.close();
            QMessageBox::critical(nullptr, "Ошибка", "Указанный размер секции больше, чем осталось до конца файла.");
            return;
        }
        chunkEnd = fin.pos() + header.size;

        // Разбор секций
        switch (header.id)
        {
        case ID_FORMAT:
            if (hasFormat)
            {
                QMessageBox::warning(nullptr, "Предупреждение", "Повторяющаяся секция FORMAT");
                break;
            }

            if (fin.read(reinterpret_cast<char*>(&_format), sizeof(FormatChunk)) != sizeof(FormatChunk))
            {
                fin.close();
                QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
                return;
            }
            hasFormat = true;
            break;

        case ID_DATA:
            if (hasData)
            {
                QMessageBox::warning(nullptr, "Предупреждение", "Повторяющаяся секция DATA");
                break;
            }

            if (!hasFormat)
            {
                fin.close();
                QMessageBox::critical(nullptr, "Ошибка", "Секция FORMAT не найдена.");
                return;
            }

            switch (_format.audioFormat)
            {
            case PCM_INT:
                switch (_format.bitsPerSample)
                {
                case 32:
                {
                    const qint64 sampleSize = sizeof(qint32);
                    _samples.reserve(fin.size() / sampleSize);
                    qint32 sample;
                    while (chunkEnd - fin.pos() >= sampleSize &&
                           fin.read(reinterpret_cast<char*>(&sample), sampleSize) == sampleSize)
                    {
                        _samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint32>::max());
                    }
                    break;
                }

                case 24:
                {
                    const qint64 sampleSize = sizeof(qint32) - 1;
                    _samples.reserve(fin.size() / sampleSize);
                    qint32 sample = 0; // Зануление необходимо
                    char* const samplePtr = reinterpret_cast<char*>(&sample) + 1; // Знаковый бит попадает куда нужно
                    while (chunkEnd - fin.pos() >= sampleSize &&
                           fin.read(samplePtr, sampleSize) == sampleSize)
                    {
                        _samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint32>::max());
                    }
                    break;
                }

                case 16:
                {
                    const qint64 sampleSize = sizeof(qint16);
                    _samples.reserve(fin.size() / sampleSize);
                    qint16 sample;
                    while (chunkEnd - fin.pos() >= sampleSize &&
                           fin.read(reinterpret_cast<char*>(&sample), sampleSize) == sampleSize)
                    {
                        _samples.append(static_cast<qreal>(sample) / std::numeric_limits<qint16>::max());
                    }
                    break;
                }

                case 8:
                {
                    const qint64 sampleSize = sizeof(quint8);
                    _samples.reserve(fin.size() / sampleSize);
                    quint8 sample;
                    while (chunkEnd - fin.pos() >= sampleSize &&
                           fin.read(reinterpret_cast<char*>(&sample), sampleSize) == sampleSize)
                    {
                        // 128 в 8-битных WAV означает 0; qint8 - не ошибка, т.к. приводим к знаковому типу
                        _samples.append((static_cast<qreal>(sample) - 128.0) / std::numeric_limits<qint8>::max());
                    }
                    break;
                }

                default:
                    fin.close();
                    QMessageBox::critical(nullptr, "Ошибка", "Неправильный размер сэмпла.");
                    return;
                }
                break;

            case PCM_FLOAT:
                switch (_format.bitsPerSample)
                {
                case 64:
                {
                    const qint64 sampleSize = sizeof(double);
                    _samples.reserve(fin.size() / sampleSize);
                    double sample;
                    while (chunkEnd - fin.pos() >= sampleSize &&
                           fin.read(reinterpret_cast<char*>(&sample), sampleSize) == sampleSize)
                    {
                        _samples.append(static_cast<qreal>(sample));
                    }
                    break;
                }

                case 32:
                {
                    const qint64 sampleSize = sizeof(float);
                    _samples.reserve(fin.size() / sampleSize);
                    float sample;
                    while (chunkEnd - fin.pos() >= sampleSize &&
                           fin.read(reinterpret_cast<char*>(&sample), sampleSize) == sampleSize)
                    {
                        _samples.append(static_cast<qreal>(sample));
                    }
                    break;
                }

                default:
                    fin.close();
                    QMessageBox::critical(nullptr, "Ошибка", "Неправильный размер сэмпла.");
                    return;
                }
                break;

            default:
                fin.close();
                QMessageBox::critical(nullptr, "Ошибка", "Программа поддерживает только несжатые WAV.");
                return;
            }
            hasData = true;
            break;
        }

        // Переходим на конец секции (например, FORMAT_EX)
        if (!fin.seek(chunkEnd))
        {
            fin.close();
            QMessageBox::critical(nullptr, "Ошибка", "Ошибка чтения.");
            return;
        }
    }
    fin.close();

    if (!hasData)
    {
        QMessageBox::critical(nullptr, "Ошибка", "Секция DATA не найдена.");
        return;
    }

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

const FormatChunk &WavReader::format()
{
    return _format;
}

const SamplesVector &WavReader::samples()
{
    return _samples;
}
}
