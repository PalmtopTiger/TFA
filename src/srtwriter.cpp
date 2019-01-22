#include "srtwriter.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

namespace SrtWriter
{
QString ToTimestamp(const uint utime)
{
    QDateTime dt;
    dt.setMSecsSinceEpoch(utime + 61200000);
    return dt.toString("HH:mm:ss,zzz");
}

void SrtWriter::addPhrase(const Phrase& phrase)
{
    _phrases.append(phrase);
}

void SrtWriter::save(const QString& fileName)
{
    QFile fout(fileName);
    if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(nullptr, "Ошибка", "Не могу открыть файл для записи.");
        return;
    }
    QTextStream out(&fout);
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);

    PhraseList::const_iterator it;
    uint num = 1;
    foreach (const Phrase& p, _phrases)
    {
        out << QString("%1\n%2 --> %3\n%4\n\n").arg(num).arg(ToTimestamp(p.time.first)).arg(ToTimestamp(p.time.second)).arg(p.text);
        ++num;
    }

    fout.close();
}
}
