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

#ifndef WAVREADER_H
#define WAVREADER_H

#include <QString>
#include <QVector>

namespace WavReader
{
#pragma pack(push, 1)
struct ChunkHeader
{
    quint32 id; // char[4]
    quint32 size;
};
struct FormatChunk
{
    quint16 audioFormat;
    quint16 numChannels;
    quint32 sampleRate;
    quint32 byteRate;
    quint16 blockAlign;
    quint16 bitsPerSample;
};
#pragma pack (pop)

typedef QVector<qreal> SamplesVector;

// Little-endian
const quint32 ID_RIFF   = 0x46464952u, // RIFF
              FMT_WAVE  = 0x45564157u, // WAVE
              ID_FORMAT = 0x20746D66u, // fmt_
              ID_DATA   = 0x61746164u; // data
const quint16 PCM_INT   = 1u,
              PCM_FLOAT = 3u;

class WavReader
{
    FormatChunk _format;
    SamplesVector _samples;
    bool _hasErrors;

public:
    WavReader();
    WavReader(const QString& fileName);
    void clear();
    void open(const QString& fileName);
    bool hasErrors();
    void toMono();
    const FormatChunk& format();
    const SamplesVector& samples();
};
}

#endif // WAVREADER_H
