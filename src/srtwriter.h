#ifndef SRTWRITER_H
#define SRTWRITER_H

#include <QList>

namespace SrtWriter
{
struct Phrase
{
    QPair<uint, uint> time;
    QString text;
};

typedef QList<Phrase> PhraseList;

class SrtWriter
{
    PhraseList _phrases;

public:
    void addPhrase(const Phrase& phrase);
    void save(const QString& fileName);
};
}

#endif // SRTWRITER_H
