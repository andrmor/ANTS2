#ifndef AMESSAGEOUTPUT_H
#define AMESSAGEOUTPUT_H

#include <QtWidgets/qapplication.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

void AMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    std::string t = localMsg.constData();
    t += "\n   In file: ";
    t += context.file;
    t += " line: ";
    t += std::to_string(context.line);
    //t += "\nFunction:";
    //t += context.function;
    t += "\n";

    switch (type)
    {
    case QtDebugMsg:
        //    std::cout << "\033[1;31mbold red text\033[0m\n";
        std::cout << t << std::flush;
        break;
    case QtInfoMsg:
        std::cout << t << std::flush;
        break;
    case QtWarningMsg:
        std::cout << t << std::flush;
        break;
    case QtCriticalMsg:
        std::cerr << t << std::flush;
        break;
    case QtFatalMsg:
        std::cerr << t << std::flush;
        abort();
    }
}

#endif // AMESSAGEOUTPUT_H
