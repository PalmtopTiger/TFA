#ifndef WAVREADER_H
#define WAVREADER_H

#include <QVector>

namespace WavReader
{
// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
#pragma pack(push, 1)
struct ChunkHeader
{
    quint32 id; // char[4]
    quint32 size;
    quint32 format; // char[4]
};
struct SubChunkHeader
{
    quint32 id; // char[4]
    quint32 size;
};
struct FormatSubChunk
{
    quint16 audioFormat;
    quint16 numChannels;
    quint32 sampleRate;
    quint32 byteRate;
    quint16 blockAlign;
    quint16 bitsPerSample;
};
#pragma pack (pop)

typedef QVector<qint32> SamplesVector;

const quint32 ID_RIFF   = 0x46464952u, // RIFF
              FMT_WAVE  = 0x45564157u, // WAVE
              ID_FORMAT = 0x20746D66u, // fmt_
              ID_DATA   = 0x61746164u; // data

class WavReader
{
    FormatSubChunk _format;
    SamplesVector _samples;
    bool _hasErrors;

public:
    WavReader();
    WavReader(const QString& fileName);
    void clear();
    void open(const QString& fileName);
    bool hasErrors();
    const FormatSubChunk& format();
    const SamplesVector& samples();
};
}

#endif // WAVREADER_H
