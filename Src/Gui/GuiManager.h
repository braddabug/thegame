#ifndef GUI_GUIMANAGER_H
#define GUI_GUIMANAGER_H

struct PlatformInfo;

namespace Content
{
	struct ContentLoaderParams;
}

namespace Gui
{
	struct GuiManagerData;

#define DEFINE_CURSOR_TYPE(t) t,
#define DEFINE_CURSOR_TYPES \
	DEFINE_CURSOR_TYPE(Pointer) \
	DEFINE_CURSOR_TYPE(PointerHighlight1) \
	DEFINE_CURSOR_TYPE(PointerHighlight2) \
	DEFINE_CURSOR_TYPE(Wait)

	enum class CursorType
	{
		DEFINE_CURSOR_TYPES

		LAST
	};

	class GuiManager
	{
		static GuiManagerData* m_data;
		static PlatformInfo* m_platform;

	public:
		static bool SetGlobalData(GuiManagerData** data, PlatformInfo* platform);
		static void Shutdown();

		static bool LoadCursor(Content::ContentLoaderParams* params);

		static void SetCursor(CursorType cursor);
	};
}

#endif // GUI_GUIMANAGER_H
