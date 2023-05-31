/*
 * This file is part of TFA.
 * Copyright (C) 2013-2023  Andrey Efremov <duxus@yandex.ru>
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
    if (!fout.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "Ошибка", "Не могу открыть файл для записи.");
        return;
    }
    QTextStream out(&fout);

    out.setGenerateByteOrderMark(true);

    uint num = 1;
    for (const Phrase& p : qAsConst(_phrases)) {
        out << QString("%1\n%2 --> %3\n%4\n\n").arg(num).arg(ToTimestamp(p.time.first), ToTimestamp(p.time.second), p.text);
        ++num;
    }

    fout.close();
}
}
