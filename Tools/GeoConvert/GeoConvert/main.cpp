#include "tiny_obj_loader.h"
#include "optionparser.h"
#include "Geometry.h"
#include "SdkMeshLoader.h"
#include <iostream>

typedef unsigned int uint32;

int main(int argc, char* argv[])
{
	argc -= (argc > 0); argv += (argc > 0); // skip program name argv[0] if present

	enum  optionIndex { UNKNOWN, HELP, TEXTURE1, TEXTURE2, OUTPUT };
	const option::Descriptor usage[] =
	{
		{ UNKNOWN, 0,"" , ""    , option::Arg::None, "USAGE: ObjToGeo [options]\n\n"
													 "Options:" },
		{ HELP,    0,"" , "help", option::Arg::None, "  --help  \tPrint usage and exit." },
		{ TEXTURE1,    0,"", "texture1",option::Arg::Optional, "  --texture1 \tSpecify the mesh input file for texture stream 1" },
		{ TEXTURE2,   0, "", "texture2", option::Arg::Optional, "  --texture2 \tSpecify the mesh input file for texture stream 2" },
		{ OUTPUT, 0, "", "output", option::Arg::Optional, "  --output \tSpecify the output filename" },
		{ UNKNOWN, 0,"" ,  ""   , option::Arg::None, "\nExamples:\n"
													 "  example --ObjToGeo --texture2 blah.sdkmesh --texture1 blah_texture.sdkmesh -o blah.geo\n" },
		{ 0,0,0,0,0,0 }
	};

	option::Stats  stats(usage, argc, argv);
	option::Option options[200], buffer[260];
	option::Parser parse(usage, argc, argv, options, buffer);
	
	if (parse.error())
		return -1;
	
	if (options[HELP] || argc == 0 || !options[TEXTURE1] )
	{
		option::printUsage(std::cout, usage);
		return 0;
	}

	const char* defaultPath = "output.geo";
	const char* path = defaultPath;
	if (options[OUTPUT])
		path = options[OUTPUT].arg;

	printf("Input file 1: %s\n", options[TEXTURE1].arg);
	if (options[TEXTURE2])
		printf("Input file 2: %s\n", options[TEXTURE2].arg);
	printf("Output file: %s\n\n", path);

	Geo::Info texture1Mesh;
	Geo::Info texture2Mesh;
	Geo::Info finalResult;

	if (SdkMeshLoader::Load(options[TEXTURE1].arg, &texture1Mesh) != 0)
	{
		printf("Error while loading %s\n", options[TEXTURE1].arg);
		return -1;
	}

	if (options[TEXTURE2] && SdkMeshLoader::Load(options[TEXTURE2].arg, &texture2Mesh) != 0)
	{
		printf("Error while loading %s\n", options[TEXTURE2].arg);
		return -1;
	}

	if (options[TEXTURE2])
	{
		if (Geo::Merge(&texture1Mesh, &texture2Mesh, &finalResult) != 0)
		{
			printf("Error while merging %s and %s\n", options[TEXTURE1].arg, options[TEXTURE2].arg);
			return -1;
		}
	}
	else
	{
		finalResult = texture1Mesh;
	}

	if (Geo::Write(path, &finalResult) == false)
	{
		printf("Error while writing %s\n", path);
		return -1;
	}

	return 0;

#if 0

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, argv[1]);
	if (ret != true)
		return false;

	const uint32 stride = 5;

	std::vector<Geo::InputMesh> meshes;

	struct Vertex
	{
		float X, Y, Z;
		float TU, TV;
	};

	std::vector<Vertex> vertices;
	std::vector<unsigned short> indices;

	for (size_t s = 0; s < shapes.size(); s++)
	{
		Geo::InputMesh m = {};
		m.NumIndices = (uint32)shapes[s].mesh.num_face_vertices.size() * 3;
		m.NumVertices = (uint32)shapes[s].mesh.num_face_vertices.size() * 3;

		if (shapes[s].mesh.material_ids.empty() == false)
			m.MaterialIndex = shapes[s].mesh.material_ids[0];
		else
			m.MaterialIndex = 0;

		m.StartIndex = indices.size();
		m.StartVertex = vertices.size();

		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			auto fv = shapes[s].mesh.num_face_vertices[f];

			if (fv != 3)
			{
				// OBJ must contain only triangles
				return false;
			}

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++)
			{
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				Vertex vertex;
				
				vertex.X = attrib.vertices[3 * idx.vertex_index + 0];
				vertex.Y = attrib.vertices[3 * idx.vertex_index + 1];
				vertex.Z = attrib.vertices[3 * idx.vertex_index + 2];
				/*vertex.NX = attrib.normals[3 * idx.normal_index + 0];
				vertex.NY = attrib.normals[3 * idx.normal_index + 1];
				vertex.NZ = attrib.normals[3 * idx.normal_index + 2];*/
				if (idx.texcoord_index >= 0)
				{
					vertex.TU = attrib.texcoords[2 * idx.texcoord_index + 0];
					vertex.TV = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				else
				{
					vertex.TU = 0;
					vertex.TV = 0;
				}

				vertices.push_back(vertex);
			}

			indices.push_back(indices.size());
			indices.push_back(indices.size());
			indices.push_back(indices.size());

			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}

		meshes.push_back(m);
	}

	const char** textureNames = nullptr;
	if (materials.empty() == false)
	{
		textureNames = new const char*[materials.size()];
		for (uint32 i = 0; i < materials.size(); i++)
		{
			textureNames[i] = materials[i].diffuse_texname.c_str();
		}
	}

	Geo::Info info = {};
	info.Meshes = meshes.data();
	info.NumMeshes = meshes.size();
	info.NumTextures = materials.size();
	info.DiffuseTextureNames = textureNames;
	info.Vertices = vertices.data();
	info.NumVertices = vertices.size();
	info.Indices = indices.data();
	info.IndexSize = 2;
	info.NumIndices = indices.size();

	info.NumVertexElements = 2;
	Geo::InputElement elements[] = {

		{ 0, Geo::VertexElementFormat::Vector3, Geo::VertexElementUsage::Position, 0, 0},
		{ 12, Geo::VertexElementFormat::Vector2, Geo::VertexElementUsage::TextureCoordinate, 0, 0}
	};
	info.VertexElements = elements;
	info.VertexStride = sizeof(float) * (3 + 2);

	Geo::Write("output.geo", &info);

	delete[] textureNames;

#endif

	return 0;
}

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
