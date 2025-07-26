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

#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>


int main(int argc, char *argv[])
{
    QApplication::setStyle("Fusion");

    QApplication app(argc, argv);
    app.setApplicationName("TFA");
    app.setApplicationVersion("1.0.1");
    app.setOrganizationName("Unlimited Web Works");
    app.setWindowIcon(QIcon(":/main.ico"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Программа для создания тайминга субтитров из аудио");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("audio", "WAV-файл");
    parser.process(app);
    const QStringList &args = parser.positionalArguments();

    MainWindow window;
    window.processCommandLine(args);
    window.show();
    
    return app.exec();
}
