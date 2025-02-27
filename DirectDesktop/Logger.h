#pragma once
#include <fstream>

#include "framework.h"
using namespace std;

class Logger {
private:
	wfstream logfile;
public:
	void StartLogger(const wchar_t* filename);
	void WriteLine(wstring line);
	void WriteLine(wstring line, int exitCode);
	~Logger();
};