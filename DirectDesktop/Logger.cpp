#include "Logger.h"
#include <string>
#include "framework.h"
using namespace std;


void Logger::StartLogger(const wchar_t* filename) {
	logfile.open(filename, wfstream::binary | wfstream::out | wfstream::trunc | wfstream::app);
	if (!logfile)
		logfile.open(filename, wfstream::binary | wfstream::trunc | wfstream::out);

	logfile << "\n\n";
	if (logfile.good())logfile << "================ New logging started ================\n" << endl;
}

void Logger::WriteLine(wstring line) {
	if (logfile.good())logfile << line << endl;
}

void Logger::WriteLine(wstring line, int exitCode) {
	logfile << line << L" (exit code: " << to_wstring(exitCode) << L")" << endl;
}

Logger::~Logger() {
	logfile.close();
}