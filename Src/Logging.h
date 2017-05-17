#ifndef LOGGING_H
#define LOGGING_H

#include "Common.h"
#include <cstdarg>
#include <atomic>

enum class LogSeverityType
{
	Debug,
	Info,
	Normal,
	Warning,
	Error
};

enum class LogChannelType
{
	Unknown,
	ConsoleInput,
	ConsoleOutput,

	Content,
	FileSystem,
	Graphics
};

struct LogEntry
{
	LogSeverityType Severity;
	LogChannelType Channel;
	uint32 TextPageIndex;
	const char* TextStart;
};

struct LogData
{
	static const uint32 LineDataSize = 10000;
	static const uint32 NumLinePages = 3;
	char* LineDataPages[NumLinePages];
	uint32 CurrentLinePageIndex;
	uint32 CurrentLinePageSize;

	static const uint32 MaxLines = 1000;
	LogEntry Lines[MaxLines];

	uint32 NumLines;
	uint32 FirstLineIndex;

	std::atomic_flag Lock;
};

#define LOG_DEBUG(f, ...) WriteLog(LogSeverityType::Debug, LogChannelType::Unknown, f, __VA_ARGS__)
#define LOG_ERROR(f, ...) WriteLog(LogSeverityType::Error, LogChannelType::Unknown, f, __VA_ARGS__)
#define LOG(f, ...) WriteLog(LogSeverityType::Normal, LogChannelType::Unknown, f, __VA_ARGS__)

void WriteLog(LogSeverityType severity, LogChannelType channel, const char* format, ...);
void WriteLog(LogData* log, LogSeverityType severity, LogChannelType channel, const char* format, ...);

#endif // LOGGING_H
