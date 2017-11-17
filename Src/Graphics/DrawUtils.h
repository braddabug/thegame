#ifndef GRAPHICS_DRAWUTILS_H
#define GRAPHICS_DRAWUTILS_H

#include <cmath>
#include "../MyNxna2.h"
#include "../Common.h"
#include "../MemoryManager.h"
#include "ShaderLibrary.h"

namespace Graphics
{
	struct DrawUtilsData
	{
		Nxna::Graphics::GraphicsDevice* Device;

		static const uint32 NumVertices = 8;
		static const uint32 NumIndices = 16;
		Nxna::Graphics::VertexBuffer BBoxVertices;
		Nxna::Graphics::IndexBuffer BBoxIndices;
		Nxna::Graphics::ConstantBuffer Transform;
		Nxna::Graphics::ConstantBuffer Color;
		Nxna::Graphics::Texture2D SolidWhite;

		static const uint32 NumSphereVertices = 10 * 8;
		static const uint32 NumSphereIndices = 10 * 8 * 6;
		Nxna::Graphics::VertexBuffer SphereVertices;
		Nxna::Graphics::IndexBuffer SphereIndices;

		static const uint32 NumPlaneVertices = 4;
		static const uint32 NumPlaneIndices = 6;
		Nxna::Graphics::VertexBuffer PlaneVertices;
		Nxna::Graphics::IndexBuffer PlaneIndices;

		bool Initialized;
	};

	class DrawUtils
	{
		static DrawUtilsData* m_data;

	public:
		static bool SetGlobalData(DrawUtilsData** data, Nxna::Graphics::GraphicsDevice* device)
		{
			if (*data == nullptr)
			{
				*data = NewObject<DrawUtilsData>(__FILE__, __LINE__);
				(*data)->Initialized = false;
				(*data)->Device = device;
			}

			m_data = *data;

			return true;
		}
		
		static void DrawBoundingBox(float box[6], Nxna::Matrix* modelviewprojection, Nxna::Color color = { 255, 255, 255, 255 })
		{
			if (m_data->Initialized == false) init();

			float minx = box[0];
			float miny = box[1];
			float minz = box[2];

			float maxx = box[3];
			float maxy = box[4];
			float maxz = box[5];

			Nxna::Matrix scalem, positionm, transform;

			// calc scale transform
			float scale[3] = { maxx - minx, maxy - miny, maxz - minz };
			Nxna::Matrix::CreateScale(scale[0], scale[1], scale[2], scalem);
			
			// calc translation transform
			float center[3] = { (maxx + minx) * 0.5f, (maxy + miny) * 0.5f, (maxz + minz) * 0.5f };
			Nxna::Matrix::CreateTranslation(center[0], center[1], center[2], positionm);
			
			transform = scalem * positionm * *modelviewprojection;

			m_data->Device->UpdateConstantBuffer(m_data->Transform, transform.C, sizeof(Nxna::Matrix));
			m_data->Device->SetConstantBuffer(m_data->Transform, 0);

			float colorf[] = { color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f };
			m_data->Device->UpdateConstantBuffer(m_data->Color, colorf, sizeof(float) * 4);
			m_data->Device->SetConstantBuffer(m_data->Color, 1);

			// draw
			m_data->Device->SetVertexBuffer(&m_data->BBoxVertices, 0, sizeof(float) * 4);
			m_data->Device->SetIndices(m_data->BBoxIndices);

			auto shader = ShaderLibrary::GetShader(ShaderType::BasicWhite);
			m_data->Device->SetShaderPipeline(shader);

			m_data->Device->DrawIndexed(Nxna::Graphics::PrimitiveType::LineStrip, 0, 0, DrawUtilsData::NumVertices, 0, 18);
			m_data->Device->DrawIndexed(Nxna::Graphics::PrimitiveType::LineList, 0, 0, DrawUtilsData::NumVertices, 18, 8);
		}

		static void DrawSphere(float position[3], float radius, Nxna::Matrix* modelviewprojection, float color[4])
		{
			if (m_data->Initialized == false) init();

			Nxna::Matrix scalem, positionm, transform;
			scalem = Nxna::Matrix::CreateScale(radius);

			Nxna::Matrix::CreateTranslation(position[0], position[1], position[2], positionm);

			transform = scalem * positionm * *modelviewprojection;

			m_data->Device->UpdateConstantBuffer(m_data->Transform, transform.C, sizeof(Nxna::Matrix));
			m_data->Device->SetConstantBuffer(m_data->Transform, 0);

			m_data->Device->UpdateConstantBuffer(m_data->Color, color, sizeof(float) * 4);
			m_data->Device->SetConstantBuffer(m_data->Color, 1);

			m_data->Device->SetVertexBuffer(&m_data->SphereVertices, 0, sizeof(float) * 4);
			m_data->Device->SetIndices(m_data->SphereIndices);

			auto shader = ShaderLibrary::GetShader(ShaderType::BasicWhite);
			m_data->Device->SetShaderPipeline(shader);

			m_data->Device->DrawIndexed(Nxna::Graphics::PrimitiveType::TriangleList, 0, 0, DrawUtilsData::NumSphereVertices, 0, DrawUtilsData::NumSphereIndices);
		}

		static void Draw2DRect(SpriteBatchHelper* sbh, float x, float y, float w, float h, Nxna::Color color)
		{
			sbh->Draw(&m_data->SolidWhite, 1, 1, x, y, w, h, color);
		}

		static void DrawQuadY(float center[3], float xSize, float zSize, Nxna::Graphics::Texture2D* texture, Nxna::Matrix* modelviewprojection)
		{
			if (m_data->Initialized == false) init();

			Nxna::Matrix scalem, positionm, rotationm, transform;
			scalem = Nxna::Matrix::Identity;
			Nxna::Matrix::CreateScale(xSize, zSize, 1.0f, scalem);
			Nxna::Matrix::CreateRotationX(1.5708f, rotationm);
			Nxna::Matrix::CreateTranslation(center[0], center[1], center[2], positionm);

			transform = scalem * rotationm * positionm * *modelviewprojection;

			m_data->Device->UpdateConstantBuffer(m_data->Transform, transform.C, sizeof(Nxna::Matrix));
			m_data->Device->SetConstantBuffer(m_data->Transform, 0);

			float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			m_data->Device->UpdateConstantBuffer(m_data->Color, color, sizeof(float) * 4);
			m_data->Device->SetConstantBuffer(m_data->Color, 1);

			m_data->Device->SetVertexBuffer(&m_data->PlaneVertices, 0, sizeof(float) * 5);
			m_data->Device->SetIndices(m_data->PlaneIndices);

			Nxna::Graphics::ShaderPipeline* shader;
			if (texture == nullptr)
				shader = ShaderLibrary::GetShader(ShaderType::BasicWhite);
			else
			{
				shader = ShaderLibrary::GetShader(ShaderType::BasicTextured);
				m_data->Device->BindTexture(texture, 0);
			}

			m_data->Device->SetShaderPipeline(shader);

			m_data->Device->DrawIndexed(Nxna::Graphics::PrimitiveType::TriangleList, 0, 0, DrawUtilsData::NumPlaneVertices, 0, DrawUtilsData::NumPlaneIndices);

		}

	private:
		static bool init()
		{
			// setup solid white
			{
				uint8 pixels[] = { 255, 255, 255, 255 };

				Nxna::Graphics::TextureCreationDesc td = {};
				td.ArraySize = 1;
				td.Width = 1;
				td.Height = 1;
				Nxna::Graphics::SubresourceData sd = {};
				sd.Data = pixels;
				sd.DataPitch = 4;
				if (m_data->Device->CreateTexture2D(&td, &sd, &m_data->SolidWhite) != Nxna::NxnaResult::Success)
					return false;
			}

			// setup bounding box
			{
				float vertices[] = {
					-0.5, -0.5, -0.5, 1.0,
					0.5, -0.5, -0.5, 1.0,
					0.5,  0.5, -0.5, 1.0,
					-0.5,  0.5, -0.5, 1.0,
					-0.5, -0.5,  0.5, 1.0,
					0.5, -0.5,  0.5, 1.0,
					0.5,  0.5,  0.5, 1.0,
					-0.5,  0.5,  0.5, 1.0,
				};

				uint16 indices[] = {
					0, 1, 1, 2, 2, 3, 3, 0, 0, 4,
					4, 5, 5, 6, 6, 7, 7, 4,
					0, 4, 1, 5, 2, 6, 3, 7
				};

				Nxna::Graphics::VertexBufferDesc vbd = {};
				vbd.ByteLength = sizeof(vertices);
				vbd.InitialDataByteCount = sizeof(vertices);
				vbd.InitialData = vertices;
				if (m_data->Device->CreateVertexBuffer(&vbd, &m_data->BBoxVertices) != Nxna::NxnaResult::Success)
					return false;

				Nxna::Graphics::IndexBufferDesc ibd = {};
				ibd.ElementSize = Nxna::Graphics::IndexElementSize::SixteenBits;
				ibd.NumElements = sizeof(indices) / 2;
				ibd.InitialData = indices;
				ibd.InitialDataByteCount = sizeof(indices);
				if (m_data->Device->CreateIndexBuffer(&ibd, &m_data->BBoxIndices) != Nxna::NxnaResult::Success)
					return false;

				Nxna::Graphics::ConstantBufferDesc cbd = {};
				cbd.ByteCount = sizeof(float) * 16;
				if (m_data->Device->CreateConstantBuffer(&cbd, &m_data->Transform) != Nxna::NxnaResult::Success)
					return false;

				cbd.ByteCount = sizeof(float) * 4;
				if (m_data->Device->CreateConstantBuffer(&cbd, &m_data->Color) != Nxna::NxnaResult::Success)
					return false;
			}

			// setup sphere
			{
				const uint32 numLongitude = 10;
				const uint32 numLatitude = 8;
				const float radius = 1.0f;
				const uint32 numVertices = numLongitude * numLatitude;
				const uint32 numIndices = numLongitude  * numLatitude * 6;

				static_assert(numVertices == DrawUtilsData::NumSphereVertices, "NumSphereVertices is incorrect");
				static_assert(numIndices == DrawUtilsData::NumSphereIndices, "NumSphereIndices is incorrect");

				float invLat = 1.0f / (float)(numLatitude - 1);
				float invLong = 1.0f / (float)(numLongitude - 1);

				float vertices[numVertices * 4];
				uint16 indices[numIndices];

				float* v = vertices;
				uint16* i = indices;
				for (uint32 lat = 0; lat < numLatitude; lat++)
				{
					for (uint32 lng = 0; lng < numLongitude; lng++)
					{
						float const y = sin(-Nxna::PiOver2 + Nxna::Pi * lat * invLat);
						float const x = cos(2 * Nxna::Pi * lng * invLong) * sin(Nxna::Pi * lat * invLat);
						float const z = sin(2 * Nxna::Pi * lng * invLong) * sin(Nxna::Pi * lat * invLat);

						*v = x * radius; v++;
						*v = y * radius; v++;
						*v = z * radius; v++;
						*v = 1.0f; v++;

						assert(v <= vertices + numVertices * 4);
					}
				}

				for (uint32 lat = 0; lat < numLatitude - 1; lat++)
				{
					for (uint32 lng = 0; lng < numLongitude - 1; lng++)
					{
						
						*i = lat * numLongitude + lng; i++;
						*i = (lat + 1) * numLongitude + lng + 1; i++;
						*i = lat * numLongitude + lng + 1; i++;
						

						*i = lat * numLongitude + lng; i++;
						*i = (lat + 1) * numLongitude + lng; i++;
						*i = (lat + 1) * numLongitude + lng + 1; i++;
						
						
						assert(i <= indices + numIndices);
					}
				}

				Nxna::Graphics::VertexBufferDesc vbd = {};
				vbd.ByteLength = sizeof(vertices);
				vbd.InitialDataByteCount = sizeof(vertices);
				vbd.InitialData = vertices;
				if (m_data->Device->CreateVertexBuffer(&vbd, &m_data->SphereVertices) != Nxna::NxnaResult::Success)
					return false;

				Nxna::Graphics::IndexBufferDesc ibd = {};
				ibd.ElementSize = Nxna::Graphics::IndexElementSize::SixteenBits;
				ibd.NumElements = sizeof(indices) / 2;
				ibd.InitialData = indices;
				ibd.InitialDataByteCount = sizeof(indices);
				if (m_data->Device->CreateIndexBuffer(&ibd, &m_data->SphereIndices) != Nxna::NxnaResult::Success)
					return false;
			}

			// setup plane
			{
				float vertices[] = {
					-1.0f, -1.0f, 0,  0, 0,
					1.0f, -1.0f, 0,  1, 0,
					1.0f, 1.0f, 0, 1, 1,
					-1.0f, 1.0f, 0, 0, 1
				};

				uint16 indices[] = {
					0, 1, 2,
					0, 2, 3
				};

				Nxna::Graphics::VertexBufferDesc vbd = {};
				vbd.ByteLength = sizeof(vertices);
				vbd.InitialDataByteCount = sizeof(vertices);
				vbd.InitialData = vertices;
				if (m_data->Device->CreateVertexBuffer(&vbd, &m_data->PlaneVertices) != Nxna::NxnaResult::Success)
					return false;

				Nxna::Graphics::IndexBufferDesc ibd = {};
				ibd.ElementSize = Nxna::Graphics::IndexElementSize::SixteenBits;
				ibd.NumElements = sizeof(indices) / 2;
				ibd.InitialData = indices;
				ibd.InitialDataByteCount = sizeof(indices);
				if (m_data->Device->CreateIndexBuffer(&ibd, &m_data->PlaneIndices) != Nxna::NxnaResult::Success)
					return false;
			}

			m_data->Initialized = true;

			return true;
		}
	};
}

#endif // GRAPHICS_DRAWUTILS_H
