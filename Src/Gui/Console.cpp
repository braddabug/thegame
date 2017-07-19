#include "Console.h"
#include "TextPrinter.h"
#include "../Logging.h"

namespace Gui
{
	struct ConsoleData
	{
		int FirstVisibleLine;
	};

	ConsoleData* Console::m_data = nullptr;

	void Console::SetGlobalData(ConsoleData** data)
	{
		if (*data == nullptr)
		{
			*data = (ConsoleData*)g_memory->AllocTrack(sizeof(ConsoleData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(ConsoleData));

			// default to locked to the last line
			(*data)->FirstVisibleLine = -1;
		}

		m_data = *data;
	}

	void Console::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}

	void Console::Draw(SpriteBatchHelper* sb, LogData* log)
	{
		const int maxLinesToDraw = 20;

		if (log->NumLines == 0) return;

		int linesToDraw = log->NumLines;
		if (linesToDraw > maxLinesToDraw) linesToDraw = maxLinesToDraw;

		int firstVisible = m_data->FirstVisibleLine;
		if (firstVisible == -1 || firstVisible > (int)log->NumLines - linesToDraw)
			firstVisible = log->NumLines - linesToDraw;
		
		
		int startLine = (log->FirstLineIndex + firstVisible) % LogData::MaxLines;

		Font* consoleFont = TextPrinter::GetFont(FontType::Console);

		for (int i = 0; i < linesToDraw; i++)
		{
			int lineIndex = (startLine + i) % LogData::MaxLines;
			
			Nxna::PackedColor color;
			switch (log->Lines[lineIndex].Severity)
			{
			case LogSeverityType::Error:
				color = NXNA_GET_PACKED_COLOR_RGB_BYTES(255, 0, 0);
				break;
			case LogSeverityType::Warning:
				color = NXNA_GET_PACKED_COLOR_RGB_BYTES(255, 255, 0);
				break;
			default:
				color = NXNA_GET_PACKED_COLOR_RGB_BYTES(255, 255, 255);
				break;
			}

			TextPrinter::PrintScreen(sb, 0, i * consoleFont->LineHeight, consoleFont, log->Lines[lineIndex].TextStart, color);
		}
	}
}
