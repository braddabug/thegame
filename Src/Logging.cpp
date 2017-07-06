#include "Logging.h"
#include <cstring>
#include <cstdio>
#include <cassert>

LogData* g_log;

void WriteLogRaw(LogData* log, LogSeverityType severity, LogChannelType channel, const char* text)
{
	uint32 len = (uint32)strlen(text); // TODO: support UTF8

	while (log->Lock.test_and_set());

	if (log->CurrentLinePageSize + len + 1 > LogData::LineDataSize)
	{
		uint32 pageToRetire = (log->CurrentLinePageIndex + 1) % LogData::NumLinePages;

		// retire the old page and use that
		while(log->NumLines > 0)
		{
			if (log->Lines[log->FirstLineIndex].TextPageIndex == pageToRetire)
			{
				log->FirstLineIndex = (log->FirstLineIndex + 1) % LogData::MaxLines;
				log->NumLines--;
			}
			else
			{
				break;
			}
		}

		log->CurrentLinePageIndex = (log->CurrentLinePageIndex + 1) % LogData::NumLinePages;
		log->CurrentLinePageSize = 0;
	}

	// figure out which line to put this
	uint32 newLineIndex;
	if (log->NumLines == LogData::MaxLines)
	{
		newLineIndex = log->FirstLineIndex;
		log->FirstLineIndex++;
		if (log->FirstLineIndex >= LogData::MaxLines)
			log->FirstLineIndex = 0;
	}
	else
	{
		newLineIndex = (log->FirstLineIndex + log->NumLines) % LogData::MaxLines;
		log->NumLines++;
	}

	log->Lines[newLineIndex].Severity = severity;
	log->Lines[newLineIndex].Channel = channel;
	log->Lines[newLineIndex].TextPageIndex = log->CurrentLinePageIndex;
	log->Lines[newLineIndex].TextStart = log->LineDataPages[log->CurrentLinePageIndex] + log->CurrentLinePageSize;

	memcpy(log->LineDataPages[log->CurrentLinePageIndex] + log->CurrentLinePageSize, text, len + 1);
	log->CurrentLinePageSize += len + 1;

	log->Lock.clear();

	printf(text);
	printf("\n");
}

void WriteLog(LogSeverityType severity, LogChannelType channel, const char* format, ...)
{
	assert(g_log != nullptr && "g_log cannot be null");

	// build the string
	va_list l;
	va_start(l, format);

	char buffer[512];
#ifdef NXNA_PLATFORM_WIN32
	vsnprintf_s(buffer, 512, format, l);
#else
	vsnprintf(buffer, 512, format, l);
#endif
	va_end(l);

	buffer[511] = 0;

	WriteLogRaw(g_log, severity, channel, buffer);
}

void WriteLog(LogData* log, LogSeverityType severity, LogChannelType channel, const char* format, ...)
{
	// build the string
	va_list l;
	va_start(l, format);

	char buffer[512];
#ifdef NXNA_PLATFORM_WIN32
	vsnprintf_s(buffer, 512, format, l);
#else
	vsnprintf(buffer, 512, format, l);
#endif
	va_end(l);

	buffer[511] = 0;

	WriteLogRaw(log, severity, channel, buffer);
}