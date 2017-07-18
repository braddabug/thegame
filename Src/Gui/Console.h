#ifndef GUI_CONSOLE_H
#define GUI_CONSOLE_H

class SpriteBatchHelper;
struct LogData;

namespace Gui
{
	class Console
	{
	public:
		static void Draw(SpriteBatchHelper* sb, LogData* data);
	};
}

#endif // GUI_CONSOLE_H
