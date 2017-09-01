#ifndef GUI_CONSOLE_H
#define GUI_CONSOLE_H

class SpriteBatchHelper;
struct LogData;
struct ConsoleCommand;

namespace Gui
{
	struct ConsoleData;

	class Console
	{
		static ConsoleData* m_data;

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
