#include "GuiManager.h"
#include "../Graphics/Bitmap.h"
#include "../MemoryManager.h"
#include "../GlobalData.h"
#include "../FileFinder.h"
#include "../Logging.h"
#include "../Content/ContentManager.h"
#include "../VirtualResolution.h"
#include "../HashStringManager.h"
#include "../iniparse.h"


namespace Gui
{
	struct GuiManagerData
	{
		CursorInfo Cursors[(int)CursorType::LAST];
		bool CursorsActive[(int)CursorType::LAST];

		SpriteBatchHelper Sprites;
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
			new (&(*data)->Sprites) SpriteBatchHelper();
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
		m_data->Sprites.~SpriteBatchHelper();
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	SpriteBatchHelper* GuiManager::GetSprites()
	{
		return &m_data->Sprites;
	}



	void GuiManager::DrawSpeech(Nxna::Vector2 virtualPosition, const char* text)
	{
		auto screenPosition = VirtualResolution::ConvertVirtualToScreen(virtualPosition);
		DrawSpeechScreen(screenPosition, text);
	}

	void GuiManager::DrawSpeechScreen(Nxna::Vector2 screenPosition, const char* text)
	{
		if (text == nullptr) return;

		auto font = TextPrinter::GetFont(FontType::Default);
		auto screenSize = TextPrinter::MeasureString(font, text);

		screenPosition.X -= screenSize.X * 0.5f;
		screenPosition.Y -= screenSize.Y * 0.5f;

		screenPosition.X = roundf(screenPosition.X);
		screenPosition.Y = roundf(screenPosition.Y);

		const float shadowOffset = 1.0f;

		TextPrinter::PrintScreen(&m_data->Sprites, screenPosition.X + shadowOffset, screenPosition.Y + shadowOffset, font, text, NXNA_GET_PACKED_COLOR_RGB_BYTES(0,0,0));
		TextPrinter::PrintScreen(&m_data->Sprites, screenPosition.X, screenPosition.Y, font, text);
	}

	void GuiManager::Render()
	{
		m_data->Sprites.Render();
		m_data->Sprites.Reset();
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
			auto filename = HashStringManager::Get(params->FilenameHash, HashStringManager::HashStringType::File);
			if (filename == nullptr)
			{
				LOG_ERROR("Unable to get filename for texture with hash %u", params->FilenameHash);
				return false;
			}

			FoundFile f;
			if (FileFinder::OpenAndMap(filename, &f) == false)
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

						loadData->Cursors[cursorCount].ImageFileHash = HashStringManager::Set(HashStringManager::HashStringType::File, imageFileBuffer);

						cursorCount++;
					}
				}

				goto parse;
			}

			FileFinder::Close(&f);

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

				if (g_platform->CreateCursor(bmp->Width, bmp->Height, (uint32)(loadData->Cursors[i].HotX * bmp->Width), (uint32)(loadData->Cursors[i].HotY * bmp->Height), bmp->Pixels, cursor) == false)
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