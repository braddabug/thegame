#include "Model.h"
#include "tiny_obj_loader.h"
#include "../StringManager.h"
#include "DrawUtils.h"
#include <sstream>

namespace Graphics
{
	ModelData* Model::m_data = nullptr;

	void Model::SetGlobalData(ModelData** data)
	{
		if (*data == nullptr)
		{
			*data = NewObject<ModelData>(__FILE__, __LINE__);
			(*data)->Initialized = false;
		}

		m_data = *data;
	}

	void Model::Shutdown()
	{
		g_memory->FreeTrack(m_data, __FILE__, __LINE__);
	}

	struct ModelLoaderStorage
	{
		float* Vertices;
		uint16* Indices;
	};

	bool Model::LoadObj(Content::ContentLoaderParams* params)
	{
		static_assert(sizeof(ModelLoaderStorage) < sizeof(Content::ContentLoaderParams::LocalDataStorage), "ModelLoaderStorage is too big");

		Model* result = (Model*)params->Destination;

		File f;
		if (FileSystem::OpenAndMap(params->Filename, &f) == nullptr)
		{
			StringManager::Release(params->Filename);
			params->Filename = nullptr;

			params->State = Content::ContentState::NotFound;
			return false;
		}
		StringManager::Release(params->Filename);
		params->Filename = nullptr;

		std::string str((char*)f.Memory, f.FileSize);
		std::stringstream ss(str);

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		tinyobj::MaterialFileReader mr("Content/Models/");

		std::string err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &ss, &mr);

		if (!err.empty()) { // `err` may contain warning message.
			// TODO: log the error
		}

		if (!ret) {
			params->State = Content::ContentState::UnknownError;
			return false;
		}

		// count the total number of vertices
		uint32 numVertices = 0;
		for (size_t s = 0; s < shapes.size(); s++)
		{
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				int fv = shapes[s].mesh.num_face_vertices[f];
				if (fv != 3)
				{
					params->State = Content::ContentState::InvalidFormat;
					return false; // model must be triangulated
				}

				numVertices += fv;
			}
		}

		struct Vertex
		{
			float X, Y, Z;
			float U, V;
		};
		Vertex* vertices = new Vertex[numVertices];
		uint16* indices = new uint16[numVertices];

		result->NumMeshes = (uint32)shapes.size();
		result->Meshes = new ModelMesh[shapes.size()];

		float minv[] = { 1e10, 1e10, 1e10 };
		float maxv[] = { -1e10, -1e10, -1e10 };

		// Loop over shapes
		uint32 vertexCursor = 0;
		for (size_t s = 0; s < shapes.size(); s++)
		{
			result->Meshes[s].FirstIndex = vertexCursor;

			// Loop over faces(polygon)
			uint32 index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				// Loop over vertices in the face.
				for (size_t v = 0; v < 3; v++)
				{
					Vertex vtx;

					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					vtx.X = attrib.vertices[3 * idx.vertex_index + 0];
					vtx.Y = attrib.vertices[3 * idx.vertex_index + 1];
					vtx.Z = attrib.vertices[3 * idx.vertex_index + 2];
					//float nx = attrib.normals[3*idx.normal_index+0];
					//float ny = attrib.normals[3*idx.normal_index+1];
					//float nz = attrib.normals[3*idx.normal_index+2];
					vtx.U = attrib.texcoords[2 * idx.texcoord_index + 0];
					//vtx.V = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
					vtx.V = attrib.texcoords[2 * idx.texcoord_index + 1];

					vertices[vertexCursor++] = vtx;

					if (vtx.X > maxv[0]) maxv[0] = vtx.X;
					if (vtx.Y > maxv[1]) maxv[1] = vtx.Y;
					if (vtx.Z > maxv[2]) maxv[2] = vtx.Z;
					if (vtx.X < minv[0]) minv[0] = vtx.X;
					if (vtx.Y < minv[1]) minv[1] = vtx.Y;
					if (vtx.Z < minv[2]) minv[2] = vtx.Z;
				}

				indices[index_offset] = index_offset; index_offset++;
				indices[index_offset] = index_offset; index_offset++;
				indices[index_offset] = index_offset; index_offset++;

				// per-face material
				result->Meshes[s].TextureIndex = shapes[s].mesh.material_ids[f];
			}

			result->Meshes[s].NumTriangles = (uint32)shapes[s].mesh.num_face_vertices.size();
		}

		result->BoundingBox[0] = minv[0];
		result->BoundingBox[1] = minv[1];
		result->BoundingBox[2] = minv[2];
		result->BoundingBox[3] = maxv[0];
		result->BoundingBox[4] = maxv[1];
		result->BoundingBox[5] = maxv[2];

		ModelLoaderStorage* storage = (ModelLoaderStorage*)params->LocalDataStorage;
		storage->Vertices = (float*)vertices;
		result->NumVertices = numVertices;

		storage->Indices = indices;
		result->NumIndices = numVertices;

		// load the textures
		result->NumTextures = (uint32)materials.size();
		result->Textures = new Nxna::Graphics::Texture2D[result->NumTextures];
		for (size_t i = 0; i < materials.size(); i++)
		{
			char buffer[256];
#ifdef _WIN32
			strcpy_s(buffer, "Content/Models/");
			strncat_s(buffer, materials[i].diffuse_texname.c_str(), 256);
#else
			strcpy(buffer, "Content/Models/");
			strncat(buffer, materials[i].diffuse_texname.c_str(), 256);
#endif
			buffer[255] = 0;

			Content::ContentState r;
			if (params->Asynchronous)
			{
				PersistantString s = StringManager::Create(buffer);
				if (s == nullptr) continue;

				r = Content::ContentLoader::BeginLoad(s, Content::LoaderType::Texture2D, &result->Textures[i], nullptr, params->Job);
			}
			else
			{
				r = Content::ContentLoader::Load(buffer, Content::LoaderType::Texture2D, &result->Textures[i]);
			}

			if (r != Content::ContentState::Loaded && r != Content::ContentState::Incomplete)
			{
				printf("Error while loading %s\n", materials[i].diffuse_texname.c_str());
				result->Textures[i] = TextureLoader::GetErrorTexture(true);
			}
		}
		

		return true;
	}

	bool Model::FinalizeLoadObj(Content::ContentLoaderParams* params)
	{
		Nxna::Graphics::GraphicsDevice* gd = (Nxna::Graphics::GraphicsDevice*)params->LoaderParam;
		Model* result = (Model*)params->Destination;

		if (m_data->Initialized == false)
		{
			Nxna::Graphics::ConstantBufferDesc cbDesc = {};
			cbDesc.InitialData = nullptr;
			cbDesc.ByteCount = sizeof(float) * 16;
			if (gd->CreateConstantBuffer(&cbDesc, &m_data->Constants) != Nxna::NxnaResult::Success)
			{
				printf("Unable to create constant buffer\n");
				params->State = Content::ContentState::UnknownError;
				return false;
			}

			Nxna::Graphics::SamplerStateDesc ssDesc = NXNA_SAMPLERSTATEDESC_LINEARWRAP;
			if (gd->CreateSamplerState(&ssDesc, &m_data->SamplerState) != Nxna::NxnaResult::Success)
			{
				printf("Unable to create sampler state\n");
				params->State = Content::ContentState::UnknownError;
				return false;
			}
		}

		ModelLoaderStorage* storage = (ModelLoaderStorage*)params->LocalDataStorage;

		Nxna::Graphics::VertexBufferDesc vbDesc = {};
		vbDesc.ByteLength = result->NumVertices * sizeof(float) * 5;
		vbDesc.InitialData = storage->Vertices;
		vbDesc.InitialDataByteCount = result->NumVertices * sizeof(float) * 5;
		if (gd->CreateVertexBuffer(&vbDesc, &result->Vertices) != Nxna::NxnaResult::Success)
		{
			delete[] storage->Vertices;
			delete[] storage->Indices;
			delete[] result->Meshes;
			params->State = Content::ContentState::UnknownError;
			return false;
		}
		delete[] storage->Vertices;

		Nxna::Graphics::IndexBufferDesc ibDesc = {};
		ibDesc.ElementSize = Nxna::Graphics::IndexElementSize::SixteenBits;
		ibDesc.NumElements = result->NumIndices;
		ibDesc.InitialDataByteCount = sizeof(uint16) * result->NumIndices;
		ibDesc.InitialData = storage->Indices;
		if (gd->CreateIndexBuffer(&ibDesc, &result->Indices) != Nxna::NxnaResult::Success)
		{
			delete[] storage->Vertices;
			delete[] storage->Indices;
			delete[] result->Meshes;
			params->State = Content::ContentState::UnknownError;
			return false;
		}
		delete[] storage->Indices;

		result->VertexStride = sizeof(float) * 5;

		Nxna::Graphics::RasterizerStateDesc rsDesc = NXNA_RASTERIZERSTATEDESC_DEFAULT;
		rsDesc.FrontCounterClockwise = true;
		if (gd->CreateRasterizerState(&rsDesc, &result->RasterState) != Nxna::NxnaResult::Success)
		{
			printf("Unable to create rasterizer state\n");
			delete[] result->Meshes;
			return false;
		}

		return true;
	}

	void Model::BeginRender(Nxna::Graphics::GraphicsDevice* device)
	{
		device->SetBlendState(nullptr);
		device->SetDepthStencilState(nullptr);
		device->SetSamplerState(0, &m_data->SamplerState);

		device->SetConstantBuffer(m_data->Constants, 0);
	}

	void Model::Render(Nxna::Graphics::GraphicsDevice* device, Nxna::Matrix* transform, Model* model)
	{
		device->UpdateConstantBuffer(m_data->Constants, transform->C, 16 * sizeof(float));

		device->SetRasterizerState(&model->RasterState);
		device->SetVertexBuffer(&model->Vertices, 0, model->VertexStride);
		device->SetIndices(model->Indices);

		auto pipeline = ShaderLibrary::GetShader(ShaderType::BasicTextured);
		device->SetShaderPipeline(pipeline);

		for (uint32 j = 0; j < model->NumMeshes; j++)
		{
			device->BindTexture(&model->Textures[model->Meshes[j].TextureIndex], 0);
			device->DrawIndexed(Nxna::Graphics::PrimitiveType::TriangleList, 0, 0, model->NumVertices, model->Meshes[j].FirstIndex, model->Meshes[j].NumTriangles * 3);
		}
	}
}
