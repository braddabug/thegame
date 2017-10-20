#include <cstdlib>
#include "TextPrinter.h"

#include "../Common.h"
#include "../FileSystem.h"
#include "../SpriteBatchHelper.h"
#include "../MemoryManager.h"

#include "../utf8.h"
#include "../MyNxna2.h"

#ifdef ENABLE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#else
#include "stb_truetype.h"
#endif

#include "stb_rect_pack.h"

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
		return createFont(device, "Content/Fonts/DroidSans.ttf", 20, 32, 127, '?', &m_data->DefaultFont) &&
			createFont(device, "Content/Fonts/Inconsolata-Regular.ttf", 13, 32, 255, '?', &m_data->ConsoleFont);
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

	inline int findCharacter(Font* font, int character)
	{
		// binary search to find the character index
		int characterIndex = -1;
		{
			int first = 0;
			int last =  font->NumCharacters - 1;
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

		return characterIndex;
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
			auto character = (int)((const uint8*)text)[i];
			auto characterIndex = findCharacter(font, character);
			if (characterIndex == -1)
				characterIndex = font->DefaultCharacterInfoIndex;
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
		auto pfont = font;
		if (pfont == nullptr) return;

		uint32 numCharacters = (uint32)utf8len(text);

		Nxna::Graphics::SpriteBatchSprite* sprites = sb->AddSprites(numCharacters);
		memset(sprites, 0, sizeof(Nxna::Graphics::SpriteBatchSprite) * numCharacters);

		float cursorX = x;

		int character, i = 0;
		while((text = (const char*)utf8codepoint(text, &character)) && character != 0)
		{
			auto characterIndex = findCharacter(pfont, character);
			if (characterIndex == -1)
				characterIndex = pfont->DefaultCharacterInfoIndex;
			auto characterInfo = pfont->Characters[characterIndex];

			sprites[i].Source[0] = characterInfo.SrcX;
			sprites[i].Source[1] = characterInfo.SrcY;
			sprites[i].Source[2] = characterInfo.SrcW;
			sprites[i].Source[3] = characterInfo.SrcH;

			sprites[i].Destination[0] = roundf(cursorX + characterInfo.XOffset);
			sprites[i].Destination[1] = roundf(y + characterInfo.YOffset);
			sprites[i].Destination[2] = characterInfo.ScreenW;
			sprites[i].Destination[3] = characterInfo.ScreenH;

			sprites[i].SpriteColor = color;

			sprites[i].Texture = pfont->Texture;
			sprites[i].TextureWidth = 256;
			sprites[i].TextureHeight = 256;

			cursorX += characterInfo.XAdvance;
			i++;
		}
		assert(i == numCharacters);
	}

	bool TextPrinter::createFont(Nxna::Graphics::GraphicsDevice* device, const char* path, float size, int firstCharacter, int lastCharacter, int defaultCharacter, Font** result)
	{
		const int numCharacters = lastCharacter - firstCharacter + 1;

		const uint32 textureSize = 256;

		File f;
		if (FileSystem::OpenAndMap(path, &f) == nullptr)
			return false;

#ifdef ENABLE_FREETYPE
		const int maxChars = 500;
		stbrp_rect rects[maxChars];

		uint8 rgbaPixels[textureSize * textureSize * 4];
		uint8 glyphPixels[textureSize * textureSize];

		if (maxChars < numCharacters)
			return false;

		FT_Library ft;
		if (FT_Init_FreeType(&ft))
		{
			FileSystem::Close(&f);
			return false;
		}

		uint8* memory = nullptr;

		FT_Face face;
		if (FT_New_Memory_Face(ft, (const FT_Byte*)f.Memory, f.FileSize, 0, &face))
			goto error;

		if (FT_Set_Pixel_Sizes(face, 0, (uint32)size - 1))
			goto error;

		stbrp_context c;
		stbrp_node nodes[textureSize];
		stbrp_init_target(&c, textureSize, textureSize, nodes, textureSize);

		memory = (uint8*)g_memory->AllocTrack(sizeof(Font) + sizeof(Font::CharInfo) * numCharacters + sizeof(int) * numCharacters, __FILE__, __LINE__);
		*result = (Font*)memory;
		(*result)->CharacterMap = (int*)(memory + sizeof(Font));
		(*result)->Characters = (Font::CharInfo*)(memory + sizeof(Font) + sizeof(int) * numCharacters);
	
		auto ascender = face->size->metrics.ascender >> 6;

		auto glyphPixelCursor = glyphPixels;
		for (int i = firstCharacter; i <= lastCharacter; i++)
		{
			if (FT_Load_Char(face, i, FT_LOAD_DEFAULT))
				goto error;

			if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
				goto error;

			int ci = i - firstCharacter;

			(*result)->Characters[ci].SrcX = (float)face->glyph->bitmap_left;
			(*result)->Characters[ci].SrcY = (float)face->glyph->bitmap_top;
			(*result)->Characters[ci].SrcW = (float)face->glyph->bitmap.width;
			(*result)->Characters[ci].SrcH = (float)face->glyph->bitmap.rows;
			(*result)->Characters[ci].ScreenW = (float)face->glyph->bitmap.width;
			(*result)->Characters[ci].ScreenH = (float)face->glyph->bitmap.rows;
			(*result)->Characters[ci].XAdvance = (float)(face->glyph->advance.x >> 6);
			(*result)->Characters[ci].XOffset = (float)face->glyph->bitmap_left;
			(*result)->Characters[ci].YOffset = (float)-face->glyph->bitmap_top;

			(*result)->CharacterMap[ci] = i;

			rects[ci].x = 0;
			rects[ci].y = 0;
			rects[ci].w = face->glyph->bitmap.width;
			rects[ci].h = face->glyph->bitmap.rows;

			memcpy(glyphPixelCursor, face->glyph->bitmap.buffer, face->glyph->bitmap.width * face->glyph->bitmap.rows);
			glyphPixelCursor += face->glyph->bitmap.width * face->glyph->bitmap.rows;
		}

		if (stbrp_pack_rects(&c, rects, numCharacters) == 0)
			goto error;

		glyphPixelCursor = glyphPixels;
		for (int i = firstCharacter; i <= lastCharacter; i++)
		{
			int ci = i - firstCharacter;

			for (int row = 0; row < rects[ci].h; row++)
			{
				for (int column = 0; column < rects[ci].w; column++)
				{
					auto pixel = &rgbaPixels[((rects[ci].y + row) * textureSize + rects[ci].x + column) * 4];
					assert(pixel + 4 < rgbaPixels + textureSize * textureSize * 4);
					int value = glyphPixelCursor[row * rects[ci].w + column];

					// TODO: convert value from sRGB to linear

					pixel[0] = value;
					pixel[1] = value;
					pixel[2] = value;
					pixel[3] = value;
				}
			}

			(*result)->Characters[ci].SrcX = rects[ci].x;
			(*result)->Characters[ci].SrcY = rects[ci].y;

			glyphPixelCursor += rects[ci].w * rects[ci].h;
		}

		// gen texture
		Nxna::Graphics::Texture2D texture;
		{
			Nxna::Graphics::TextureCreationDesc desc = {};
			desc.ArraySize = 1;
			desc.MipLevels = 1;
			desc.Width = textureSize;
			desc.Height = textureSize;
			Nxna::Graphics::SubresourceData srdata = {};
			srdata.Data = rgbaPixels;
			srdata.DataPitch = textureSize * 4;
			if (device->CreateTexture2D(&desc, &srdata, &texture) != Nxna::NxnaResult::Success)
				goto error;
		}

		(*result)->Texture = texture;

		(*result)->LineHeight = size;
		(*result)->NumCharacters = numCharacters;
		(*result)->DefaultCharacterInfoIndex = findCharacter((*result), defaultCharacter);
		if ((*result)->DefaultCharacterInfoIndex == -1)
		{
			goto error;
		}

		FT_Done_Face(face);
		FT_Done_FreeType(ft);

		FileSystem::Close(&f);

		return true;

	error:
		if (memory) g_memory->FreeTrack(memory, __FILE__, __LINE__);

		FileSystem::Close(&f);

		FT_Done_Face(face);
		FT_Done_FreeType(ft);

		return false;
#else
		uint8 pixels[textureSize * textureSize];

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
		g_memory->FreeTrack(r.chardata_for_range, __FILE__, __LINE__);

		(*result)->LineHeight = size;
		(*result)->NumCharacters = numCharacters;
		(*result)->DefaultCharacterInfoIndex = findCharacter((*result), defaultCharacter);
		if ((*result)->DefaultCharacterInfoIndex == -1)
			return false;
#endif

		return true;
	}
}


#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#ifdef ENABLE_FREETYPE
#ifdef _MSC_VER
#pragma comment(lib, "freetype281d.lib")
#endif
#endif