#include "Console.h"
#include "TextPrinter.h"
#include "../Logging.h"

namespace Gui
{
	void Console::Draw(SpriteBatchHelper* sb, LogData* log)
	{
		const int maxLinesToDraw = 10;

		if (log->NumLines == 0) return;

		int linesToDraw = log->NumLines;
		if (linesToDraw > maxLinesToDraw) linesToDraw = maxLinesToDraw;
		
		int startLine = (log->FirstLineIndex + log->NumLines - linesToDraw) % LogData::MaxLines;

		Font* consoleFont = TextPrinter::GetFont(FontType::Console);

		for (int i = startLine; i < linesToDraw; i++)
		{
			int lineIndex = i % LogData::MaxLines;
			
			TextPrinter::PrintScreen(sb, 0, i * consoleFont->LineHeight, consoleFont, log->Lines[i].TextStart);
		}
	}
}
