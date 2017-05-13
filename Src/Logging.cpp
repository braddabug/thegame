#include "Logging.h"
#include <cstring>
#include <cstdio>

LogData* g_log;

void WriteLog(LogSeverityType severity, LogChannelType channel, const char* format, ...)
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

	uint32 len = (uint32)strlen(buffer); // TODO: support UTF8

	if (g_log->CurrentLinePageSize + len + 1 > LogData::LineDataSize)
	{
		uint32 pageToRetire = (g_log->CurrentLinePageIndex + 1) % LogData::NumLinePages;

		// retire the old page and use that
		while(g_log->NumLines > 0)
		{
			if (g_log->Lines[g_log->FirstLineIndex].TextPageIndex == pageToRetire)
			{
				g_log->FirstLineIndex = (g_log->FirstLineIndex + 1) % LogData::MaxLines;
				g_log->NumLines--;
			}
			else
			{
				break;
			}
		}

		g_log->CurrentLinePageIndex = (g_log->CurrentLinePageIndex + 1) % LogData::NumLinePages;
		g_log->CurrentLinePageSize = 0;
	}

	// figure out which line to put this
	uint32 newLineIndex;
	if (g_log->NumLines == LogData::MaxLines)
	{
		newLineIndex = g_log->FirstLineIndex;
		g_log->FirstLineIndex++;
		if (g_log->FirstLineIndex >= LogData::MaxLines)
			g_log->FirstLineIndex = 0;
	}
	else
	{
		newLineIndex = (g_log->FirstLineIndex + g_log->NumLines) % LogData::MaxLines;
		g_log->NumLines++;
	}

	g_log->Lines[newLineIndex].Severity = severity;
	g_log->Lines[newLineIndex].Channel = channel;
	g_log->Lines[newLineIndex].TextPageIndex = g_log->CurrentLinePageIndex;
	g_log->Lines[newLineIndex].TextStart = g_log->LineDataPages[g_log->CurrentLinePageIndex] + g_log->CurrentLinePageSize;

	memcpy(g_log->LineDataPages[g_log->CurrentLinePageIndex] + g_log->CurrentLinePageSize, buffer, len + 1);
	g_log->CurrentLinePageSize += len + 1;

	printf(buffer);
	printf("\n");
}