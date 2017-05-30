#ifndef GRAPHICS_DRAWUTILS_H
#define GRAPHICS_DRAWUTILS_H

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

	private:
		static bool init()
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

			m_data->Initialized = true;

			return true;
		}
	};
}

#endif // GRAPHICS_DRAWUTILS_H
