/*
 * This file is part of TFA.
 * Copyright (C) 2013-2025  Andrey Efremov <duxus@yandex.ru>
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

#ifndef SRTWRITER_H
#define SRTWRITER_H

#include <QPair>
#include <QList>
#include <QString>

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
    void addPhrase(const Phrase &phrase);
    bool save(const QString &fileName);
};
}

#endif // SRTWRITER_H
