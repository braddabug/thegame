#include <cstdlib>
#include "TextPrinter.h"
#include "stb_truetype.h"
#include "stb_rect_pack.h"
#include "../Common.h"
#include "../FileSystem.h"
#include "../SpriteBatchHelper.h"

#include "../MyNxna2.h"

namespace Gui
{
	bool TextPrinter::Init(Nxna::Graphics::GraphicsDevice* device, TextPrinterData* result)
	{
		return createFont(device, result, "Content/Fonts/DroidSans.ttf", 36, &result->DefaultFont);
	}

	void TextPrinter::Shutdown(TextPrinterData* data)
	{
		free(data->DefaultFont);
	}

	void TextPrinter::PrintScreen(SpriteBatchHelper* sb, TextPrinterData* data, float x, float y, FontType font, const char* text)
	{
		uint32 numCharacters = (uint32)strlen(text);

		auto pfont = data->DefaultFont;
		if (pfont == nullptr) return;

		Nxna::Graphics::SpriteBatchSprite* sprites = sb->AddSprites(numCharacters);
		memset(sprites, 0, sizeof(Nxna::Graphics::SpriteBatchSprite) * numCharacters);

		const auto white = NXNA_GET_PACKED_COLOR_RGB_BYTES(255, 255, 255);

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

			sprites[i].SpriteColor = white;

			sprites[i].Texture = data->Texture;
			sprites[i].TextureWidth = 256;
			sprites[i].TextureHeight = 256;

			cursorX += characterInfo.XAdvance;
		}
	}

	bool TextPrinter::createFont(Nxna::Graphics::GraphicsDevice* device, TextPrinterData* data, const char* path, float size, Font** result)
	{
		const int firstCharacter = 32;
		const int lastCharacter = 127;
		const int numCharacters = lastCharacter - firstCharacter;

		const uint32 textureSize = 256;
		uint8 pixels[textureSize * textureSize];

		File f;
		if (FileSystem::Open(path, &f) == false)
			return false;

		if (FileSystem::MapFile(&f) == nullptr)
		{
			FileSystem::Close(&f);
			return false;
		}

		stbtt_pack_context c;
		stbtt_PackBegin(&c, pixels, textureSize, textureSize, 0, 1, nullptr);
		stbtt_pack_range r = {};
		r.first_unicode_codepoint_in_range = firstCharacter;
		r.num_chars = numCharacters;
		r.font_size = size;
		r.chardata_for_range = new stbtt_packedchar[numCharacters];
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

		Nxna::Graphics::TextureCreationDesc desc = {};
		desc.ArraySize = 1;
		desc.Width = textureSize;
		desc.Height = textureSize;
		Nxna::Graphics::SubresourceData srdata = {};
		srdata.Data = rgbaPixels;
		srdata.DataPitch = textureSize * 4;
		if (device->CreateTexture2D(&desc, &srdata, &data->Texture) != Nxna::NxnaResult::Success)
		{
			delete[] r.chardata_for_range;
			return false;
		}

		auto memory = (uint8*)malloc(sizeof(Font) + sizeof(Font::CharInfo) * numCharacters + sizeof(int) * numCharacters);

		*result = (Font*)memory;
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

		(*result)->NumCharacters = numCharacters;

		delete[] r.chardata_for_range;
		
		return true;
	}
}


#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
