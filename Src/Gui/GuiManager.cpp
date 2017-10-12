#include "GuiManager.h"
#include "../Graphics/Bitmap.h"
#include "../MemoryManager.h"
#include "../GlobalData.h"
#include "../FileSystem.h"
#include "../Logging.h"
#include "../Content/ContentManager.h"
#include "../iniparse.h"

namespace Gui
{
	struct GuiManagerData
	{
		CursorInfo Cursors[(int)CursorType::LAST];
		bool CursorsActive[(int)CursorType::LAST];
	};

	GuiManagerData* GuiManager::m_data = nullptr;
	PlatformInfo* GuiManager::m_platform = nullptr;

	bool GuiManager::SetGlobalData(GuiManagerData** data, PlatformInfo* platform)
	{
		m_platform = platform;

		if (*data == nullptr)
		{
			*data = (GuiManagerData*)g_memory->AllocTrack(sizeof(GuiManagerData), __FILE__, __LINE__);
			memset(*data, 0, sizeof(GuiManagerData));
			m_data = *data;
		}
		else
		{
			m_data = *data;
		}

		return true;
	}

	void GuiManager::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	struct CursorLoadInfo
	{
		CursorType Type;
		uint32 ImageFileHash;
		float HotX;
		float HotY;
	};

	struct CursorLoaderData
	{
		CursorLoadInfo* Cursors;
		uint32 NumCursors;
	};

	bool GuiManager::LoadCursor(Content::ContentLoaderParams* params)
	{
		if (params->Phase == Content::LoaderPhase::AsyncLoad)
		{
			auto filename = FileSystem::GetFilenameByHash(params->FilenameHash);
			if (filename == nullptr)
			{
				LOG_ERROR("Unable to get filename for texture with hash %u", params->FilenameHash);
				return false;
			}

			File f;
			if (FileSystem::OpenAndMap(filename, &f) == nullptr)
			{
				LOG_ERROR("Unable to open cursor file %s", filename);
				return false;
			}

			ini_context ctx;
			ini_init(&ctx, (const char*)f.Memory, (const char*)f.Memory + f.FileSize);

			ini_item item;

			static_assert(sizeof(CursorLoaderData) <= Content::ContentLoaderParams::LocalDataStorageSize, "CursorLoaderData is too big");
			CursorLoaderData* loadData = (CursorLoaderData*)params->LocalDataStorage;

			// determine the total # of cursors
			uint32 cursorCount = 0;
			while (ini_next(&ctx, &item) == ini_result_success)
			{
				if (item.type == ini_itemtype::section && ini_section_equals(&ctx, &item, "cursor"))
					cursorCount++;
			}

			// create storage for the cursor info
			loadData->Cursors = (CursorLoadInfo*)g_memory->AllocTrack(sizeof(CursorLoadInfo) * cursorCount, __FILE__, __LINE__);
			loadData->NumCursors = cursorCount;

			// reset and parse
			cursorCount = 0;
			ini_init(&ctx, (const char*)f.Memory, (const char*)f.Memory + f.FileSize);

		parse:
			if (ini_next(&ctx, &item) == ini_result_success)
			{
				if (item.type == ini_itemtype::section)
				{
					if (ini_section_equals(&ctx, &item, "cursor"))
					{
						char name[64];
						char image[64];
						float x = 0, y = 0;

						while (ini_next_within_section(&ctx, &item) == ini_result_success)
						{
							if (ini_key_equals(&ctx, &item, "name"))
								ini_value_copy(&ctx, &item, name, 64);
							else if (ini_key_equals(&ctx, &item, "image"))
								ini_value_copy(&ctx, &item, image, 64);
							else if (ini_key_equals(&ctx, &item, "x"))
								ini_value_float(&ctx, &item, &x);
							else if (ini_key_equals(&ctx, &item, "y"))
								ini_value_float(&ctx, &item, &y);
						}


						// map the name to a cursor type
#undef DEFINE_CURSOR_TYPE
#ifdef _MSC_VER
#define DEFINE_CURSOR_TYPE(t) if (_stricmp(name, #t) == 0) { loadData->Cursors[cursorCount].Type = CursorType:: t; } else
#else
#define DEFINE_CURSOR_TYPE(t) if (strcasecmp(name, #t) == 0) { loadData->Cursors[cursorCount].Type = CursorType:: t; } else
#endif
						DEFINE_CURSOR_TYPES
						{
							// else
							WriteLog(LogSeverityType::Warning, LogChannelType::Content, "Unable to map \"%s\" to a specific cursor type", name);
							goto parse;
						}

						loadData->Cursors[cursorCount].HotX = x;
						loadData->Cursors[cursorCount].HotY = y;

						char imageFileBuffer[256];
						VirtualResolution::InjectNearestResolution(image, imageFileBuffer, 256);

						loadData->Cursors[cursorCount].ImageFileHash = Utils::CalcHash(imageFileBuffer);

						cursorCount++;
					}
				}

				goto parse;
			}

			FileSystem::Close(&f);

			return true;
		}
		else if (params->Phase == Content::LoaderPhase::MainThread)
		{
			
		}
		else if (params->Phase == Content::LoaderPhase::Fixup)
		{
			CursorLoaderData* loadData = (CursorLoaderData*)params->LocalDataStorage;

			for (uint32 i = 0; i < loadData->NumCursors; i++)
			{
				Graphics::Bitmap* bmp = (Graphics::Bitmap*)Content::ContentManager::Get(loadData->Cursors[i].ImageFileHash, Content::ResourceType::Bitmap);
				if (bmp == nullptr)
				{
					WriteLog(LogSeverityType::Error, LogChannelType::Content, "Can't get cursor bitmap with hash %u", loadData->Cursors[i].ImageFileHash);
					return false;
				}

				CursorInfo* cursor = &m_data->Cursors[(int)loadData->Cursors[i].Type];

				if (g_platform->CreateCursor(bmp->Width, bmp->Height, loadData->Cursors[i].HotX * bmp->Width, loadData->Cursors[i].HotY * bmp->Height, bmp->Pixels, cursor) == false)
				{
					WriteLog(LogSeverityType::Error, LogChannelType::Content, "Error when trying to create cursor");
					return false;
				}

				m_data->CursorsActive[(int)loadData->Cursors[i].Type] = true;
			}

			return true;
		}

		return true;
	}

	void GuiManager::SetCursor(CursorType cursor)
	{
		if (m_data->CursorsActive[(int)cursor])
		{
			m_platform->SetCursor(&m_data->Cursors[(int)cursor]);
		}
	}
}