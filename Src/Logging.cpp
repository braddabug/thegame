#include "Logging.h"
#include <cstring>
#include <cstdio>

LogData* g_log;

void WriteLog(LogData* log, LogSeverityType severity, LogChannelType channel, const char* format, ...)
{
	if (log == nullptr)
		log = g_log;

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

	uint32 len = (uint32)strlen(buffer); // TODO: support UTF8

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

	memcpy(log->LineDataPages[log->CurrentLinePageIndex] + log->CurrentLinePageSize, buffer, len + 1);
	log->CurrentLinePageSize += len + 1;

	printf(buffer);
	printf("\n");
}