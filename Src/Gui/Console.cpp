#include "../Common.h"
#include "Console.h"
#include "TextPrinter.h"
#include "../ConsoleCommand.h"
#include "../Logging.h"
#include "../GlobalData.h"
#include "../utf8.h"

extern PlatformInfo* g_platform;

namespace Gui
{
	struct ConsoleData
	{
		static const int MaxInputBufferSize = 256;
		char InputBuffer[MaxInputBufferSize];
		int InputBufferSize;
		int InputBufferCursor;

		static const int MaxHistorySize = 5;
		char History[MaxHistorySize][MaxInputBufferSize];
		int HistorySize;
		int HistoryCursor;
		int HistoryNext;

		int FirstVisibleLine;
		bool Visible;

		static const int MaxCommands = 100;
		static const int MaxCommandLength = 32;
		uint32 NumCommands;
		char Commands[MaxCommands][MaxCommandLength];
		CommandCallback CommandCallbacks[MaxCommands];
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

			// add the default "help" command
			(*data)->NumCommands = 1;
#ifdef _MSC_VER
			strcpy_s((*data)->Commands[0], "help");
#else
			strcpy((*data)->Commands[0], "help");
#endif
			(*data)->CommandCallbacks[0] = cmd_help;
		}

		m_data = *data;
	}

	void Console::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
		m_data = nullptr;
	}

	void Console::AddCommands(ConsoleCommand* commands, uint32 numCommands)
	{
		assert(m_data->NumCommands + numCommands < ConsoleData::MaxCommands);

		for (uint32 i = 0; i < numCommands; i++)
		{
#ifdef _MSC_VER
			strcpy_s(m_data->Commands[m_data->NumCommands], commands[i].Command);
#else
			strcpy(m_data->Commands[m_data->NumCommands], commands[i].Command);
#endif
			m_data->CommandCallbacks[m_data->NumCommands] = commands[i].Callback;
			m_data->NumCommands++;
		}
	}

	bool Console::IsVisible()
	{
		return m_data->Visible;
	}

	void Console::IsVisible(bool visible)
	{
		m_data->Visible = visible;
	}

	void injectText(ConsoleData* data, const char* text)
	{
		uint32 len = (uint32)strlen(text); // get byte length, not character length
		uint32 toCopy = ConsoleData::MaxInputBufferSize - 1 - data->InputBufferSize;
		if (toCopy > len) toCopy = len;

		if (toCopy > 0)
		{
			if (data->InputBufferCursor != data->InputBufferSize)
			{
				// the cursor is in the middle, so we have to rearrange things
				memmove(data->InputBuffer + data->InputBufferCursor + toCopy, data->InputBuffer + data->InputBufferCursor, data->InputBufferSize - data->InputBufferCursor);
			}

			memcpy(data->InputBuffer + data->InputBufferCursor, text, toCopy);
			data->InputBufferSize += toCopy;
			data->InputBufferCursor += toCopy;
			data->InputBuffer[data->InputBufferSize] = 0;
		}

		assert(data->InputBufferSize < ConsoleData::MaxInputBufferSize);
	}

	void Console::HandleInput(Nxna::Input::InputState* input)
	{
		// handle unicdoe input
		for (uint32 i = 0; i < input->NumBufferedKeys; i++)
		{
			auto key = input->BufferedKeys[i];
			if (key >= 32)
			{
				char buffer[8]; buffer[0] = 0;
				auto end = (char*)utf8catcodepoint(buffer, key, 8);
				if (end != nullptr)
				{
					end[0] = 0;
					injectText(m_data, buffer);
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

			if (m_data->HistorySize > 0)
			{
				uint32 upTransition = NXNA_BUTTON_TRANSITION_COUNT(input->KeyboardKeysData[(int)Nxna::Input::Key::Up]);
				uint32 upDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::Up]) > 0 ? 1 : 0;
				uint32 downTransition = NXNA_BUTTON_TRANSITION_COUNT(input->KeyboardKeysData[(int)Nxna::Input::Key::Down]);
				uint32 downDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::Down]) > 0 ? 1 : 0;

				int oldCursor = m_data->HistoryCursor;
				m_data->HistoryCursor += (upTransition + upDown) / 2;
				m_data->HistoryCursor -= (downTransition + downDown) / 2;
				m_data->HistoryCursor = Utils::Wrap(m_data->HistoryCursor, m_data->HistorySize);

				if (oldCursor != m_data->HistoryCursor)
				{
					int history = Utils::Wrap(m_data->HistoryNext - m_data->HistoryCursor, m_data->HistorySize);
#ifdef _WIN32
					strcpy_s(m_data->InputBuffer, m_data->History[history]);
#else
					strcpy(m_data->InputBuffer, m_data->History[history]);
#endif
					m_data->InputBufferCursor = m_data->InputBufferSize = (int)strlen(m_data->InputBuffer);
				}
			}
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
			WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleInput, ">%s", m_data->InputBuffer);
			
			m_data->HistorySize++;
			
			if (m_data->HistorySize > m_data->MaxHistorySize)
				m_data->HistorySize = m_data->MaxHistorySize;

#ifdef _WIN32
			strcpy_s(m_data->History[m_data->HistoryNext], m_data->InputBuffer);
#else
			strcpy(m_data->History[m_data->HistoryNext], m_data->InputBuffer);
#endif

			m_data->HistoryCursor = 0;
			m_data->HistoryNext++;
			if (m_data->HistoryNext >= m_data->MaxHistorySize)
				m_data->HistoryNext = 0;

			// find and execute the command
			{
				const char* commandEnd = m_data->InputBuffer;
				while (commandEnd[0] != 0 && commandEnd[0] != ' ')
					commandEnd++;

				for (uint32 i = 0; i < m_data->NumCommands; i++)
				{
					if (Utils::CompareI(m_data->InputBuffer, (uint32)(commandEnd - m_data->InputBuffer), m_data->Commands[i]) == 0)
					{
						const char* argsStart = commandEnd;
						while (argsStart[0] == ' ') argsStart++;

						m_data->CommandCallbacks[i](argsStart);

						goto after;
					}
				}

				// didn't find the command :(
				WriteLog(LogSeverityType::Error, LogChannelType::ConsoleOutput, "Unknown command: %s", m_data->InputBuffer);
			}
			after:
						
			m_data->InputBuffer[0] = 0;
			m_data->InputBufferSize = 0;
			m_data->InputBufferCursor = 0;
		}

		// handle clipboard
		{
			uint32 insertTransition = NXNA_BUTTON_TRANSITION_COUNT(input->KeyboardKeysData[(int)Nxna::Input::Key::Insert]);
			uint32 insertDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::Insert]) > 0 ? 1 : 0;
			uint32 shiftDown = NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::LeftShift]) > 0 ||
				NXNA_BUTTON_STATE(input->KeyboardKeysData[(int)Nxna::Input::Key::RightShift]) > 0 ? 1 : 0;

			if (insertDown == 1 && insertTransition > 0 && shiftDown == 1)
			{
				const char* clipboard = g_platform->GetClipboardText();
				if (clipboard != nullptr)
				{
					injectText(m_data, clipboard);
					g_platform->FreeClipboardText(clipboard);
				}
			}
		}

		// handle scroll
		{
			int firstVisibleLine = m_data->FirstVisibleLine;
			if (firstVisibleLine == -1)
				firstVisibleLine = g_log->NumLines - MaxLinesToDraw;

			int delta = -input->RelWheel * 3;

			m_data->FirstVisibleLine = firstVisibleLine + delta;

			if (m_data->FirstVisibleLine < 0) m_data->FirstVisibleLine = 0;
			if ((uint32)m_data->FirstVisibleLine >= g_log->NumLines - MaxLinesToDraw) m_data->FirstVisibleLine = -1;
		}
	}

	void Console::Draw(SpriteBatchHelper* sb, LogData* log)
	{
		if (m_data->Visible == false || log->NumLines == 0) return;

		int linesToDraw = log->NumLines;
		if (linesToDraw > MaxLinesToDraw) linesToDraw = MaxLinesToDraw;

		int firstVisible = m_data->FirstVisibleLine;
		if (firstVisible < 0 || firstVisible > (int)log->NumLines - linesToDraw)
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

	void Console::cmd_help(const char* args)
	{
		WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleOutput, "List of available commands:");

		for (uint32 i = 0; i < m_data->NumCommands; i++)
		{
			WriteLog(LogSeverityType::Normal, LogChannelType::ConsoleOutput, "   %s", m_data->Commands[i]);
		}
	}
}
