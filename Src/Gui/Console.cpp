#include "Console.h"
#include "TextPrinter.h"
#include "../Logging.h"

namespace Gui
{
	struct ConsoleData
	{
		char InputBuffer[256];
		int InputBufferSize;
		int InputBufferCursor;
		int FirstVisibleLine;
		bool Visible;
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

	bool Console::IsVisible()
	{
		return m_data->Visible;
	}

	void Console::IsVisible(bool visible)
	{
		m_data->Visible = visible;
	}

	void Console::HandleInput(Nxna::Input::InputState* input)
	{
		// handle unicdoe input
		for (uint32 i = 0; i < input->NumBufferedKeys; i++)
		{
			auto key = input->BufferedKeys[i];
			if (key >= 32)
			{
				if (m_data->InputBufferSize < 255)
				{
					if (m_data->InputBufferCursor != m_data->InputBufferSize)
					{
						// the cursor is in the middle, so we have to rearrange things
						memmove(m_data->InputBuffer + m_data->InputBufferCursor + 1, m_data->InputBuffer + m_data->InputBufferCursor, m_data->InputBufferSize - m_data->InputBufferCursor);
					}

					m_data->InputBuffer[m_data->InputBufferCursor] = key;
					m_data->InputBuffer[m_data->InputBufferSize + 1] = 0;
					m_data->InputBufferSize++;
					m_data->InputBufferCursor++;
				}
			}
		}

		// handle arrows
		{
			uint32 leftTransition = NXNA_BUTTON_TRANSITION_COUNT(input->KeyboardKeysData[(int)Nxna::Input::Key::Left]);
			uint32 leftDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::Left]) > 0 ? 1 : 0;
			uint32 rightTransition = NXNA_BUTTON_TRANSITION_COUNT(input->KeyboardKeysData[(int)Nxna::Input::Key::Right]);
			uint32 rightDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::Right]) > 0 ? 1 : 0;

			m_data->InputBufferCursor -= (leftTransition + leftDown) / 2;
			m_data->InputBufferCursor += (rightTransition + rightDown) / 2;

			if (m_data->InputBufferCursor < 0) m_data->InputBufferCursor = 0;
			if (m_data->InputBufferCursor > m_data->InputBufferSize) m_data->InputBufferCursor = m_data->InputBufferSize;
		}

		// handle backspace
		{
			int32 backspaceTransition = NXNA_BUTTON_TRANSITION_COUNT(input->KeyboardKeysData[(int)Nxna::Input::Key::Back]);
			int32 backspaceDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::Back]) > 0 ? 1 : 0;
			int32 backspaceCount = (backspaceTransition + backspaceDown) / 2;

			if (backspaceCount > m_data->InputBufferCursor)
				backspaceCount = m_data->InputBufferCursor;

			if (backspaceCount > 0)
			{
				m_data->InputBufferSize -= backspaceCount;
				m_data->InputBufferCursor -= backspaceCount;
				if (m_data->InputBufferSize < 0) m_data->InputBufferSize = 0;
				if (m_data->InputBufferCursor < 0) m_data->InputBufferCursor = 0;

				if (m_data->InputBufferCursor != m_data->InputBufferSize)
				{
					memmove(m_data->InputBuffer + m_data->InputBufferCursor, m_data->InputBuffer + m_data->InputBufferCursor + backspaceCount, m_data->InputBufferSize - m_data->InputBufferCursor);
				}
				m_data->InputBuffer[m_data->InputBufferSize] = 0;
			}
		}

		// handle delete
		{
			int32 deleteTransition = NXNA_BUTTON_TRANSITION_COUNT(input->KeyboardKeysData[(int)Nxna::Input::Key::Delete]);
			int32 deleteDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::Delete]) > 0 ? 1 : 0;
			int32 deleteCount = (deleteTransition + deleteDown) / 2;

			if (deleteCount > m_data->InputBufferSize - m_data->InputBufferCursor)
				deleteCount = m_data->InputBufferSize - m_data->InputBufferCursor;

			if (deleteCount > 0)
			{
				m_data->InputBufferSize -= deleteCount;
				if (m_data->InputBufferSize < 0) m_data->InputBufferSize = 0;

				if (m_data->InputBufferCursor != m_data->InputBufferSize)
				{
					memmove(m_data->InputBuffer + m_data->InputBufferCursor, m_data->InputBuffer + m_data->InputBufferCursor + deleteCount, m_data->InputBufferSize - m_data->InputBufferCursor);
				}
				m_data->InputBuffer[m_data->InputBufferSize] = 0;
			}
		}

		// handle enter
		uint32 enterTransition = NXNA_BUTTON_TRANSITION_COUNT(input->KeyboardKeysData[(int)Nxna::Input::Key::Enter]);
		uint32 enterDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::Enter]) > 0 ? 1 : 0;
		if (enterDown && enterTransition == 1)
		{
			WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleInput, m_data->InputBuffer);
			m_data->InputBuffer[0] = 0;
			m_data->InputBufferSize = 0;
			m_data->InputBufferCursor = 0;
		}
	}

	void Console::Draw(SpriteBatchHelper* sb, LogData* log)
	{
		const int maxLinesToDraw = 20;

		if (m_data->Visible == false || log->NumLines == 0) return;

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

		if (m_data->InputBufferSize > 0)
		{
			TextPrinter::PrintScreen(sb, 0, linesToDraw * consoleFont->LineHeight, consoleFont, m_data->InputBuffer);
		}

		// draw the cursor
		auto size = TextPrinter::MeasureString(consoleFont, m_data->InputBuffer, m_data->InputBuffer + m_data->InputBufferCursor);
		TextPrinter::PrintScreen(sb, size.X, linesToDraw * consoleFont->LineHeight, consoleFont, "_");
	}
}
