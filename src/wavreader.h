#ifndef WAVREADER_H
#define WAVREADER_H

#include <QString>
#include <QVector>

namespace WavReader
{
// https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
#pragma pack(push, 1)
struct ChunkHeader
{
    char ID[4];
    quint32 size;
    char format[4];
};
struct SubChunkHeader
{
    char ID[4];
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

const char RIFF_ID[] = "RIFF";
const char WAVE[] = "WAVE";
const QString FORMAT_ID = "fmt ";
const QString DATA_ID = "data";

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
