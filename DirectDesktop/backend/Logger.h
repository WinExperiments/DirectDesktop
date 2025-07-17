#pragma once

#include <fstream>

using namespace std;

namespace DirectDesktop
{
    class Logger
    {
    public:
        ~Logger();

        void StartLogger(const wchar_t* filename);

        void WriteLine(wstring line);
        void WriteLine(wstring line, int exitCode);

    private:
        wfstream logfile;
    };
}
