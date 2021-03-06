#ifndef GUI_CONSOLE_H
#define GUI_CONSOLE_H

#include "../Common.h"

class SpriteBatchHelper;
struct LogData;
struct ConsoleCommand;

namespace Gui
{
	struct ConsoleData;

	class Console
	{
		static ConsoleData* m_data;
		static const uint32 MaxLinesToDraw = 20;

	public:
		static void SetGlobalData(ConsoleData** data);
		static void Shutdown();

		static void AddCommands(ConsoleCommand* commands, uint32 numCommands);

		static bool IsVisible();
		static void IsVisible(bool visible);

		static void HandleInput(Nxna::Input::InputState* input);

		static void Draw(SpriteBatchHelper* sb, LogData* data);

	private:
		static void cmd_help(const char* args);
	};
}

#endif // GUI_CONSOLE_H
