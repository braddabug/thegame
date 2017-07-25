#include <cstdlib>
#include "TextPrinter.h"
#include "stb_truetype.h"
#include "stb_rect_pack.h"
#include "../Common.h"
#include "../FileSystem.h"
#include "../SpriteBatchHelper.h"
#include "../MemoryManager.h"

#include "../MyNxna2.h"

namespace Gui
{
	TextPrinterData* TextPrinter::m_data = nullptr;

	void TextPrinter::SetGlobalData(TextPrinterData** data)
	{
		if (*data == nullptr)
			*data = NewObject<TextPrinterData>(__FILE__, __LINE__);
		
		m_data = *data;
	}

	bool TextPrinter::Init(Nxna::Graphics::GraphicsDevice* device)
	{
		return createFont(device, "Content/Fonts/DroidSans.ttf", 20, &m_data->DefaultFont) &&
			createFont(device, "Content/Fonts/DroidSans.ttf", 12, &m_data->ConsoleFont);
	}

	void TextPrinter::Shutdown()
	{
		g_memory->FreeTrack(m_data->DefaultFont, __FILE__, __LINE__);

		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	Font* TextPrinter::GetFont(FontType type)
	{
		switch (type)
		{
		case FontType::Console: return m_data->ConsoleFont;
		default: return m_data->DefaultFont;
		}
	}

	Nxna::Vector2 TextPrinter::MeasureString(Font* font, const char* text, const char* end)
	{
		Nxna::Vector2 result;
		uint32 numCharacters;
		if (end == nullptr)
			numCharacters = (uint32)strlen(text);
		else
			numCharacters = (uint32)(end - text);

		for (uint32 i = 0; i < numCharacters; i++)
		{
			auto character = (int)text[i];

			// binary search to find the character index
			int characterIndex = -1;
			{
				int first = 0;
				int last = font->NumCharacters - 1;
				int middle = (first + last) / 2;

				while (first <= last)
				{
					if (font->CharacterMap[middle] < character)
						first = middle + 1;
					else if (font->CharacterMap[middle] == character)
					{
						characterIndex = middle;
						break;
					}
					else
						last = middle - 1;

					middle = (first + last) / 2;
				}
			}

			if (characterIndex == -1)
				continue;

			auto characterInfo = font->Characters[characterIndex];

			result.X += characterInfo.XAdvance;

			auto height = characterInfo.YOffset + characterInfo.ScreenH;
			if (height > result.Y)
				result.Y = height;
		}

		return result;
	}

	void TextPrinter::PrintScreen(SpriteBatchHelper* sb, float x, float y, Font* font, const char* text, Nxna::PackedColor color)
	{
		uint32 numCharacters = (uint32)strlen(text);

		auto pfont = font;
		if (pfont == nullptr) return;

		Nxna::Graphics::SpriteBatchSprite* sprites = sb->AddSprites(numCharacters);
		memset(sprites, 0, sizeof(Nxna::Graphics::SpriteBatchSprite) * numCharacters);

		float cursorX = x;

		for (uint32 i = 0; i < numCharacters; i++)
		{
			auto character = (int)text[i];

			// binary search to find the character index
			int characterIndex = -1;
			{
				int first = 0;
				int last =  pfont->NumCharacters - 1;
				int middle = (first + last) / 2;

				while (first <= last)
				{
					if (pfont->CharacterMap[middle] < character)
						first = middle + 1;
					else if (pfont->CharacterMap[middle] == character)
					{
						characterIndex = middle;
						break;
					}
					else
						last = middle - 1;

					middle = (first + last) / 2;
				}
			}

			if (characterIndex == -1)
				continue;

			auto characterInfo = pfont->Characters[characterIndex];

			sprites[i].Source[0] = characterInfo.SrcX;
			sprites[i].Source[1] = characterInfo.SrcY;
			sprites[i].Source[2] = characterInfo.SrcW;
			sprites[i].Source[3] = characterInfo.SrcH;

			sprites[i].Destination[0] = cursorX + characterInfo.XOffset;
			sprites[i].Destination[1] = y + characterInfo.YOffset;
			sprites[i].Destination[2] = characterInfo.ScreenW;
			sprites[i].Destination[3] = characterInfo.ScreenH;

			sprites[i].SpriteColor = color;

			sprites[i].Texture = pfont->Texture;
			sprites[i].TextureWidth = 256;
			sprites[i].TextureHeight = 256;

			cursorX += characterInfo.XAdvance;
		}
	}

	bool TextPrinter::createFont(Nxna::Graphics::GraphicsDevice* device, const char* path, float size, Font** result)
	{
		const int firstCharacter = 32;
		const int lastCharacter = 127;
		const int numCharacters = lastCharacter - firstCharacter;

		const uint32 textureSize = 256;
		uint8 pixels[textureSize * textureSize];

		File f;
		if (FileSystem::OpenAndMap(path, &f) == nullptr)
			return false;

		stbtt_pack_context c;
		stbtt_PackBegin(&c, pixels, textureSize, textureSize, 0, 1, nullptr);
		stbtt_pack_range r = {};
		r.first_unicode_codepoint_in_range = firstCharacter;
		r.num_chars = numCharacters;
		r.font_size = size;
		r.chardata_for_range = (stbtt_packedchar*)g_memory->AllocTrack(sizeof(stbtt_packedchar) * numCharacters, __FILE__, __LINE__);
		stbtt_PackFontRanges(&c, (uint8*)f.Memory, 0, &r, 1);
		stbtt_PackEnd(&c);

		FileSystem::Close(&f);

		// TODO: this could be done really slick like and do the conversion in-place and only create 1 array
		uint8 rgbaPixels[textureSize * textureSize * 4];

		for (int i = 0; i < textureSize * textureSize; i++)
		{
			auto pixel = pixels[i];

			rgbaPixels[i * 4 + 0] = pixel;
			rgbaPixels[i * 4 + 1] = pixel;
			rgbaPixels[i * 4 + 2] = pixel;
			rgbaPixels[i * 4 + 3] = pixel;
		}

		Nxna::Graphics::Texture2D texture;
		Nxna::Graphics::TextureCreationDesc desc = {};
		desc.ArraySize = 1;
		desc.Width = textureSize;
		desc.Height = textureSize;
		Nxna::Graphics::SubresourceData srdata = {};
		srdata.Data = rgbaPixels;
		srdata.DataPitch = textureSize * 4;
		if (device->CreateTexture2D(&desc, &srdata, &texture) != Nxna::NxnaResult::Success)
		{
			g_memory->FreeTrack(r.chardata_for_range, __FILE__, __LINE__);
			return false;
		}

		auto memory = (uint8*)g_memory->AllocTrack(sizeof(Font) + sizeof(Font::CharInfo) * numCharacters + sizeof(int) * numCharacters, __FILE__, __LINE__);

		*result = (Font*)memory;
		(*result)->Texture = texture;
		(*result)->CharacterMap = (int*)(memory + sizeof(Font));
		(*result)->Characters = (Font::CharInfo*)(memory + sizeof(Font) + sizeof(int) * numCharacters);

		for (int i = 0; i < numCharacters; i++)
		{
			(*result)->Characters[i].SrcX = r.chardata_for_range[i].x0;
			(*result)->Characters[i].SrcY = r.chardata_for_range[i].y0;
			(*result)->Characters[i].SrcW = (float)(r.chardata_for_range[i].x1 - r.chardata_for_range[i].x0);
			(*result)->Characters[i].SrcH = (float)(r.chardata_for_range[i].y1 - r.chardata_for_range[i].y0);
			
			(*result)->Characters[i].ScreenW = (float)(r.chardata_for_range[i].x1 - r.chardata_for_range[i].x0);
			(*result)->Characters[i].ScreenH = (float)(r.chardata_for_range[i].y1 - r.chardata_for_range[i].y0);
			
			(*result)->Characters[i].XAdvance = r.chardata_for_range[i].xadvance;
			(*result)->Characters[i].XOffset = r.chardata_for_range[i].xoff;
			(*result)->Characters[i].YOffset = r.chardata_for_range[i].yoff;
			
			(*result)->CharacterMap[i] = firstCharacter + i;
		}

		(*result)->LineHeight = size;
		(*result)->NumCharacters = numCharacters;

		g_memory->FreeTrack(r.chardata_for_range, __FILE__, __LINE__);
		
		return true;
	}
}


#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
