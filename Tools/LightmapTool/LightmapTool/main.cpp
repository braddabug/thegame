#include <cstdio>
#include <cassert>
#include <cstdlib>
#include "SDL.h"
#include "glew/GL/glew.h"
#include "lightmapper.h"
#include "GeoLoader.h"
#include "FileSystem.h"
#include "Graphics/stb_image.h"

#include <cstring>
#include "iniparse.h"

#include "Common.h"

//#define STBVOX_CONFIG_MODE 0
//#include "stb_voxel_render.h"

const int LC = 4;

typedef struct {
	float p[3];
	float t[2];
} vertex_t;

struct SceneMesh
{
	unsigned int indexCount, startIndex;
};

struct Lightmap
{
	float* Data;
	uint32 Width;
	uint32 Height;
	GLuint GLHandle;
};

struct SceneModel
{
	GLuint vao, vbo, ibo;
	vertex_t *vertices;
	unsigned short *indices;
	unsigned int indexCount;
	unsigned int vertexCount;
	
	SceneMesh* Meshes;
	unsigned int meshCount;

	bool isEmissive;
	float emissive[3];

	float position[3];

	char LightmapName[64];

	Lightmap LightmapData;
};

enum class LightType
{
	Point,
	Directional
};

struct SceneLight
{
	LightType Type;

	SceneModel Model;
};



typedef struct
{
	GLuint program;
	GLint u_lightmap;
	GLint u_projection;
	GLint u_view;

	static const int MAX_MODELS = 10;
	SceneModel models[MAX_MODELS];
	unsigned int modelCount;

	SceneLight lights[MAX_MODELS];
	unsigned int lightCount;
} scene_t;

int initScene(scene_t *scene, const char* filename);
void drawScene(scene_t *scene, float *view, float *projection);
void destroyScene(scene_t *scene);
int bake(scene_t *scene, int pass);
GLuint loadProgram(const char *vp, const char *fp, const char **attributes, int attributeCount);
void fpsCameraViewMatrix(SDL_Window *window, float *view);
void perspectiveMatrix(float *out, float fovy, float aspect, float zNear, float zFar);
void multiplyMatrices(float *out, float *a, float *b);
void translationMatrix(float *out, float x, float y, float z);
int loadSimpleObjFile(const char *filename, vertex_t **vertices, unsigned int *vertexCount, unsigned short **indices, unsigned int *indexCount);

int main(int argc, char* argv[])
{
	const int w = 640;
	const int h = 480;
	int pass = 0;

	if (argc != 2)
	{
		printf("Usage: LightmapTool [scene file]\n");
		return -1;
	}

	auto window = SDL_CreateWindow("Lightmap Tool", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_OPENGL);
	if (window == nullptr)
	{
		printf("Unable to create window\n");
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	auto context = SDL_GL_CreateContext(window);
	if (context == nullptr)
	{
		printf("Unable to create OpenGL context\n");
		return -1;
	}

	glewInit();

	scene_t scene = { 0 };
	if (!initScene(&scene, argv[1]))
	{
		fprintf(stderr, "Could not initialize scene.\n");
		return -1;
	}

	printf("Ambient Occlusion Baking Example.\n");
	printf("Use your mouse and the W, A, S, D, E, Q keys to navigate.\n");
	printf("Press SPACE to start baking one light bounce!\n");
	printf("This will take a few seconds and bake a lightmap illuminated by:\n");
	printf("1. The mesh itself (initially black)\n");
	printf("2. A white sky (1.0f, 1.0f, 1.0f)\n");


	while (true)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				goto end;
			case SDL_KEYUP:
				if (e.key.keysym.sym == SDLK_SPACE)
				{
					bake(&scene, pass++);
					glViewport(0, 0, w, h);
				}
				break;
			}
		}

		float view[16], projection[16];
		fpsCameraViewMatrix(window, view);
		perspectiveMatrix(projection, 45.0f, (float)w / (float)h, 0.01f, 100.0f);

		// draw to screen with a blueish sky
		glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_CULL_FACE);

		drawScene(&scene, view, projection);

		SDL_GL_SwapWindow(window);
	}

end:

	destroyScene(&scene);

	return 0;
}

void genSphereGeometry(ModelGeometry* result);

int initModel(ModelGeometry* geometry, const char* lightmapName, float* emissive, SceneModel* result);

int initModel(const char* name, const char* lightmapName, bool isGeo, float* emissive, SceneModel* result)
{
	// load mesh
	ModelGeometry geo = {};

	if (isGeo)
	{
		if (LoadModelGeo(name, &geo) == false)
		{
			return 0;
		}
	}
	else
	{
		if (LoadModelObj(name, &geo) == false)
		{
			return 0;
		}
	}

	return initModel(&geo, lightmapName, emissive, result);
}

int initModel(ModelGeometry* geometry, const char* lightmapName, float* emissive, SceneModel* result)
{
	ModelGeometry& geo = *geometry;

	SceneModel model;
	if (emissive == nullptr)
	{
		model.isEmissive = false;
		model.emissive[0] = model.emissive[1] = model.emissive[2] = 0;
	}
	else
	{
		model.isEmissive = emissive != nullptr;
		model.emissive[0] = emissive[0];
		model.emissive[1] = emissive[1];
		model.emissive[2] = emissive[2];
	}

	model.position[0] = model.position[1] = model.position[2] = 0;
	model.Meshes = new SceneMesh[geo.Meshes.Length];
	model.indices = new uint16[geo.Indices.Length];
	model.vertices = new vertex_t[geo.Positions.Length / 3];

	for (size_t i = 0; i < geo.Indices.Length; i++)
	{
		model.indices[i] = geo.Indices.A[i];
	}

	for (size_t i = 0; i < geo.Positions.Length / 3; i++)
	{
		model.vertices[i].p[0] = geo.Positions.A[i * 3 + 0];
		model.vertices[i].p[1] = geo.Positions.A[i * 3 + 1];
		model.vertices[i].p[2] = geo.Positions.A[i * 3 + 2];

		model.vertices[i].t[0] = geo.TextureCoords.A[i * 2 + 0];
		model.vertices[i].t[1] = geo.TextureCoords.A[i * 2 + 1];
	}

	for (size_t i = 0; i < geo.Meshes.Length; i++)
	{
		model.Meshes[i].startIndex = geo.Meshes.A[i].StartIndex;
		model.Meshes[i].indexCount = geo.Meshes.A[i].NumTriangles * 3;
	}

	model.meshCount = geo.Meshes.Length;
	model.vertexCount = geo.Positions.Length / 3;
	model.indexCount = geo.Indices.Length;

	glGenVertexArrays(1, &model.vao);
	glBindVertexArray(model.vao);

	glGenBuffers(1, &model.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
	glBufferData(GL_ARRAY_BUFFER, model.vertexCount * sizeof(vertex_t), model.vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &model.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indexCount * sizeof(unsigned short), model.indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, p));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, t));

	// create lightmap texture
	model.LightmapData.Width = 512;
	model.LightmapData.Height = 512;
	glGenTextures(1, &model.LightmapData.GLHandle);
	glBindTexture(GL_TEXTURE_2D, model.LightmapData.GLHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (lightmapName)
	{
		int w, h, d;
		auto pixels = stbi_load(lightmapName, &w, &h, &d, LC);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		stbi_image_free(pixels);
		
		strcpy_s(model.LightmapName, lightmapName);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, emissive);

		model.LightmapName[0] = 0;
	}

	model.LightmapData.Data = new float[model.LightmapData.Width * model.LightmapData.Height * LC];
	if (emissive == nullptr)
		memset(model.LightmapData.Data, 0, sizeof(float) * model.LightmapData.Width * model.LightmapData.Height * LC);
	else
	{
		for (int i = 0; i < model.LightmapData.Width * model.LightmapData.Height; i++)
		{
			memcpy(model.LightmapData.Data + i * LC, emissive, sizeof(float) * 3);
			if (LC == 4)
				model.LightmapData.Data[i * LC + 3] = 0;
		}
	}

	*result = model;

	return 1;
}

void prepModelForBake(SceneModel* model)
{
	glBindTexture(GL_TEXTURE_2D, model->LightmapData.GLHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, model->LightmapData.Width, model->LightmapData.Height, 0, GL_RGBA, GL_FLOAT, model->LightmapData.Data);
}

static int initScene(scene_t *scene, const char* filename)
{
	File f;
	if (FileSystem::Open(filename, &f) == false || FileSystem::MapFile(&f) == nullptr)
		return 0;

	const char* txt = (char*)f.Memory;

	ini_context ctx;
	ini_init(&ctx, txt, txt + f.FileSize);

	ini_item item;

parse:
	if (ini_next(&ctx, &item) == ini_result_success)
	{
		if (item.type == ini_itemtype::section)
		{
			if (ini_section_equals(&ctx, &item, "light"))
			{
				float x = 0, y = 0, z = 0;
				float r = 1.0f, g = 1.0f, b = 1.0f;
				LightType type = LightType::Point;

				while (ini_next_within_section(&ctx, &item) == ini_result_success)
				{
					if (ini_key_equals(&ctx, &item, "type"))
					{
						if (ini_value_equals(&ctx, &item, "directional"))
							type = LightType::Directional;
					}
					if (ini_key_equals(&ctx, &item, "x"))
						ini_value_float(&ctx, &item, &x);
					else if (ini_key_equals(&ctx, &item, "y"))
						ini_value_float(&ctx, &item, &y);
					else if (ini_key_equals(&ctx, &item, "z"))
						ini_value_float(&ctx, &item, &z);

					else if (ini_key_equals(&ctx, &item, "r"))
						ini_value_float(&ctx, &item, &r);
					else if (ini_key_equals(&ctx, &item, "g"))
						ini_value_float(&ctx, &item, &g);
					else if (ini_key_equals(&ctx, &item, "b"))
						ini_value_float(&ctx, &item, &b);
				}

				float glowing[] = { r, g, b, 1.0f };

				//if (initModel("sphere.obj", nullptr, false, glowing, &scene->lights[scene->lightCount].Model) == 0)
				//	return 0;
				ModelGeometry sphere;
				genSphereGeometry(&sphere);
				if (initModel(&sphere, nullptr, glowing, &scene->lights[scene->lightCount].Model) == 0)
					return 0;

				scene->lights[scene->lightCount].Type = type;
				if (type == LightType::Directional)
				{
					scene->lights[scene->lightCount].Model.position[0] = -x;
					scene->lights[scene->lightCount].Model.position[1] = -y;
					scene->lights[scene->lightCount].Model.position[2] = -z;
				}
				else
				{
					scene->lights[scene->lightCount].Model.position[0] = x;
					scene->lights[scene->lightCount].Model.position[1] = y;
					scene->lights[scene->lightCount].Model.position[2] = z;
				}

				scene->lightCount++;

				goto parse;
			}
			else if (ini_section_equals(&ctx, &item, "model"))
			{
				float x = 0, y = 0, z = 0;
				float r = 0, g = 0, b = 0;
				bool isEmissive = false;
				char name[256]; name[0] = 0;
				char lightmap[64]; lightmap[0] = 0;

				while (ini_next_within_section(&ctx, &item) == ini_result_success)
				{
					if (ini_key_equals(&ctx, &item, "file"))
					{
						int len = item.keyvalue.value_end - item.keyvalue.value_start;
#ifdef _WIN32
						strncpy_s(name, txt + item.keyvalue.value_start, len < 256 ? len : 256);
#else
						strncpy(name, txt + item.keyvalue.value_start, len < 256 ? len : 256);
#endif
						name[255] = 0;
					}
					else if (ini_key_equals(&ctx, &item, "lightmap"))
					{
						int len = item.keyvalue.value_end - item.keyvalue.value_start;
#ifdef _WIN32
						strncpy_s(lightmap, txt + item.keyvalue.value_start, len < 64 ? len : 64);
#else
						strncpy(liightmap, txt + item.keyvalue.value_start, len < 64 ? len : 64);
#endif
						lightmap[63] = 0;
					}
					else if (ini_key_equals(&ctx, &item, "x"))
						ini_value_float(&ctx, &item, &x);
					else if (ini_key_equals(&ctx, &item, "y"))
						ini_value_float(&ctx, &item, &y);
					else if (ini_key_equals(&ctx, &item, "z"))
						ini_value_float(&ctx, &item, &z);

					else if (ini_key_equals(&ctx, &item, "r"))
					{
						isEmissive = true;
						ini_value_float(&ctx, &item, &r);
					}
					else if (ini_key_equals(&ctx, &item, "g"))
					{
						isEmissive = true;
						ini_value_float(&ctx, &item, &g);
					}
					else if (ini_key_equals(&ctx, &item, "b"))
					{
						isEmissive = true;
						ini_value_float(&ctx, &item, &b);
					}
				}

				if (name[0] != 0)
				{
					bool isGeo = false;
					if (name[strlen(name) - 1] == 'o')
						isGeo = true;

					
					float glowing[] = { r, g, b, 1.0f };
					float* emissive = isEmissive ? glowing : nullptr;

					if (initModel(name, lightmap, isGeo, emissive, &scene->models[scene->modelCount]) == 0)
					{
						printf("Could not load model %s\n", name);
						return 0;
					}

					scene->models[scene->modelCount].position[0] = x;
					scene->models[scene->modelCount].position[1] = y;
					scene->models[scene->modelCount].position[2] = z;

					scene->modelCount++;
				}
				goto parse;
			}
		}
	}

	FileSystem::Close(&f);

	// load shader
	const char *vp =
		"#version 150 core\n"
		"in vec3 a_position;\n"
		"in vec2 a_texcoord;\n"
		"uniform mat4 u_view;\n"
		"uniform mat4 u_projection;\n"
		"out vec2 v_texcoord;\n"

		"void main()\n"
		"{\n"
		"gl_Position = u_projection * (u_view * vec4(a_position, 1.0));\n"
		"v_texcoord = a_texcoord;\n"
		"}\n";

	const char *fp =
		"#version 150 core\n"
		"in vec2 v_texcoord;\n"
		"uniform sampler2D u_lightmap;\n"
		"out vec4 o_color;\n"

		"void main()\n"
		"{\n"
		"o_color = vec4(texture(u_lightmap, v_texcoord).rgb, gl_FrontFacing ? 1.0 : 0.0);\n"
		"}\n";

	const char *attribs[] =
	{
		"a_position",
		"a_texcoord"
	};

	scene->program = loadProgram(vp, fp, attribs, 2);
	if (!scene->program)
	{
		fprintf(stderr, "Error loading shader\n");
		return 0;
	}
	scene->u_view = glGetUniformLocation(scene->program, "u_view");
	scene->u_projection = glGetUniformLocation(scene->program, "u_projection");
	scene->u_lightmap = glGetUniformLocation(scene->program, "u_lightmap");

	return 1;
}

void drawModel(scene_t* scene, SceneModel* model, float* view)
{
	float transform[16];
	translationMatrix(transform, model->position[0], model->position[1], model->position[2]);

	float out[16];
	multiplyMatrices(out, view, transform);

	glUniformMatrix4fv(scene->u_view, 1, GL_FALSE, out);

	glBindTexture(GL_TEXTURE_2D, model->LightmapData.GLHandle);

	glBindVertexArray(model->vao);

	for (unsigned int i = 0; i < model->meshCount; i++)
	{
		glDrawElements(GL_TRIANGLES, model->Meshes[i].indexCount, GL_UNSIGNED_SHORT, (void*)(model->Meshes[i].startIndex * sizeof(uint16)));
	}
}

void drawScene(scene_t *scene, float *view, float *projection)
{
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	glUseProgram(scene->program);
	glUniform1i(scene->u_lightmap, 0);
	glUniformMatrix4fv(scene->u_projection, 1, GL_FALSE, projection);
	
	
	for (unsigned int mi = 0; mi < scene->modelCount; mi++)
	{
		auto model = &scene->models[mi];

		drawModel(scene, model, view);
	}

	for (unsigned int mi = 0; mi < scene->lightCount; mi++)
	{
		auto model = &scene->lights[mi].Model;

		if (scene->lights[mi].Type == LightType::Point)
			drawModel(scene, model, view);
		else if (scene->lights[mi].Type == LightType::Directional)
		{
			float lightView[16];
			memcpy(lightView, view, sizeof(float) * 16);
			lightView[12] = lightView[13] = lightView[14] = 0;

			drawModel(scene, model, lightView);
		}
	}
}

static void destroyScene(scene_t *scene)
{
	auto model = &scene->models[0];
	{
		free(model->vertices);
		free(model->indices);
		glDeleteVertexArrays(1, &model->vao);
		glDeleteBuffers(1, &model->vbo);
		glDeleteBuffers(1, &model->ibo);
		glDeleteTextures(1, &model->LightmapData.GLHandle);
	}
	glDeleteProgram(scene->program);
}

static int loadSimpleObjFile(const char *filename, vertex_t **vertices, unsigned int *vertexCount, unsigned short **indices, unsigned int *indexCount)
{
#ifdef _WIN32
	FILE* file;
	if (fopen_s(&file, filename, "rt") != 0)
		return 0;
#else
	FILE *file = fopen(filename, "rt");
	if (!file)
		return 0;
#endif
	
	char line[1024];

	// first pass
	unsigned int np = 0, nn = 0, nt = 0, nf = 0;
	while (!feof(file))
	{
		fgets(line, 1024, file);
		if (line[0] == '#') continue;
		if (line[0] == 'v')
		{
			if (line[1] == ' ') { np++; continue; }
			if (line[1] == 'n') { nn++; continue; }
			if (line[1] == 't') { nt++; continue; }
			assert(!"unknown vertex attribute");
		}
		if (line[0] == 'f') { nf++; continue; }
		assert(!"unknown identifier");
	}
	assert(np && np == nn && np == nt && nf); // only supports obj files without separately indexed vertex attributes

											  // allocate memory
	*vertexCount = np;
	*vertices = (vertex_t*)calloc(np, sizeof(vertex_t));
	*indexCount = nf * 3;
	*indices = (unsigned short*)calloc(nf * 3, sizeof(unsigned short));

	// second pass
	fseek(file, 0, SEEK_SET);
	unsigned int cp = 0, cn = 0, ct = 0, cf = 0;
	while (!feof(file))
	{
		fgets(line, 1024, file);
		if (line[0] == '#') continue;
		if (line[0] == 'v')
		{
			if (line[1] == ' ') { float *p = (*vertices)[cp++].p; char *e1, *e2; p[0] = (float)strtod(line + 2, &e1); p[1] = (float)strtod(e1, &e2); p[2] = (float)strtod(e2, 0); continue; }
			if (line[1] == 'n') { /*float *n = (*vertices)[cn++].n; char *e1, *e2; n[0] = (float)strtod(line + 3, &e1); n[1] = (float)strtod(e1, &e2); n[2] = (float)strtod(e2, 0);*/ continue; } // no normals needed
			if (line[1] == 't') { float *t = (*vertices)[ct++].t; char *e1;      t[0] = (float)strtod(line + 3, &e1); t[1] = (float)strtod(e1, 0);                                continue; }
			assert(!"unknown vertex attribute");
		}
		if (line[0] == 'f')
		{
			unsigned short *tri = (*indices) + cf;
			cf += 3;
			char *e1, *e2, *e3 = line + 1;
			for (int i = 0; i < 3; i++)
			{
				unsigned long pi = strtoul(e3 + 1, &e1, 10);
				assert(e1[0] == '/');
				unsigned long ti = strtoul(e1 + 1, &e2, 10);
				assert(e2[0] == '/');
				unsigned long ni = strtoul(e2 + 1, &e3, 10);
				assert(pi == ti && pi == ni);
				tri[i] = (unsigned short)(pi - 1);
			}
			continue;
		}
		assert(!"unknown identifier");
	}

	fclose(file);
	return 1;
}

int bake(scene_t *scene, int pass)
{
	glDisable(GL_CULL_FACE);

	float ambientRed, ambientGreen, ambientBlue;
	if (pass == 0 && false)
	{
		ambientRed = ambientGreen = ambientBlue = 1.0f;
	}
	else
	{
		ambientRed = ambientGreen = ambientBlue = 0;
	}

	lm_context *ctx = lmCreate(
		64,               // hemisphere resolution (power of two, max=512)
		0.01f, 200.0f,   // zNear, zFar of hemisphere cameras
		ambientRed, ambientGreen, ambientBlue, // background color (white for ambient occlusion)
		0, 0.00f);        // lightmap interpolation threshold (small differences are interpolated rather than sampled)
						  // check debug_interpolation.tga for an overview of sampled (red) vs interpolated (green) pixels.
	if (!ctx)
	{
		fprintf(stderr, "Error: Could not initialize lightmapper.\n");
		return 0;
	}
	
	for (unsigned int mi = 0; mi < scene->modelCount; mi++)
	{
		prepModelForBake(&scene->models[mi]);
	}

	for (unsigned int mi = 0; mi < scene->modelCount; mi++)
	{
		auto model = &scene->models[mi];

		printf("Baking model #%u\n", mi);

		int w = model->LightmapData.Width, h = model->LightmapData.Height;
		float *data = model->LightmapData.Data;

		lmSetTargetLightmap(ctx, data, w, h, LC);

		lmSetGeometry(ctx, NULL,
			LM_FLOAT, (unsigned char*)model->vertices + offsetof(vertex_t, p), sizeof(vertex_t),
			LM_FLOAT, (unsigned char*)model->vertices + offsetof(vertex_t, t), sizeof(vertex_t),
			model->indexCount, LM_UNSIGNED_SHORT, model->indices);

		int vp[4];
		float view[16], projection[16];
		double lastUpdateTime = 0.0;
		while (lmBegin(ctx, vp, view, projection))
		{
			// render to lightmapper framebuffer
			glViewport(vp[0], vp[1], vp[2], vp[3]);
			drawScene(scene, view, projection);

			// display progress every second (printf is expensive)
			double time = SDL_GetTicks() / 1000.0f;
			if (time - lastUpdateTime > 1.0)
			{
				lastUpdateTime = time;
				printf("\r%6.2f%%", lmProgress(ctx) * 100.0f);
				fflush(stdout);
			}

			lmEnd(ctx);
		}
		printf("\rFinished baking %d triangles.\n", model->indexCount / 3);
	}
	
	for (unsigned int mi = 0; mi < scene->modelCount; mi++)
	{
		auto model = &scene->models[mi];
		auto w = model->LightmapData.Width;
		auto h = model->LightmapData.Height;

		auto data = model->LightmapData.Data;
		float *temp = (float*)calloc(w * h * LC, sizeof(float));

		//lmImageSmooth(data, temp, model->LightmapData.Width, model->LightmapData.Height, LC);
		//lmImageDilate(temp, data, model->LightmapData.Width, model->LightmapData.Height, LC);
		for (int i = 0; i < 4; i++)
		{
			lmImageDilate(data, temp, model->LightmapData.Width, model->LightmapData.Height, LC);
			lmImageDilate(temp, data, model->LightmapData.Width, model->LightmapData.Height, LC);
		}

		memcpy(model->LightmapData.Data, data, w * h * LC * sizeof(float));

		// if the model is emissive then add that back in
		if (model->isEmissive)
		{
			for (int i = 0; i < w * h; i++)
			{
				data[i * 3 + 0] = model->emissive[0];
				data[i * 3 + 1] = model->emissive[1];
				data[i * 3 + 2] = model->emissive[2];
			}
		}


		// postprocess texture
		
		//lmImageSmooth(data, temp, w, h, 3);
		//lmImageDilate(temp, data, w, h, 3);
		for (int i = 0; i < 16; i++)
		{
			lmImageDilate(data, temp, w, h, LC);
			lmImageDilate(temp, data, w, h, LC);
		}
		lmImagePower(data, w, h, LC, 1.0f / 2.2f, 0x7); // gamma correct color channels
		
		// save result to a file
		if (model->LightmapName[0] != 0)
		{
			// flip the image
			for (int i = 0; i < h; i++)
			{
				memcpy(temp + i * w * LC, data + (h - i - 1) * w * LC, w * sizeof(float) * LC);
			}

			if (lmImageSaveTGAf(model->LightmapName, temp, w, h, LC, 1.0f))
				printf("Saved %s\n", model->LightmapName);
		}
		free(temp);

		// upload result
		glBindTexture(GL_TEXTURE_2D, model->LightmapData.GLHandle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, data);
	}

	lmDestroy(ctx);

	return 1;
}

static GLuint loadShader(GLenum type, const char *source)
{
	GLuint shader = glCreateShader(type);
	if (shader == 0)
	{
		fprintf(stderr, "Could not create shader!\n");
		return 0;
	}
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		fprintf(stderr, "Could not compile shader!\n");
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen)
		{
			char* infoLog = (char*)malloc(infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "%s\n", infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint loadProgram(const char *vp, const char *fp, const char **attributes, int attributeCount)
{
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vp);
	if (!vertexShader)
		return 0;
	GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fp);
	if (!fragmentShader)
	{
		glDeleteShader(vertexShader);
		return 0;
	}

	GLuint program = glCreateProgram();
	if (program == 0)
	{
		fprintf(stderr, "Could not create program!\n");
		return 0;
	}
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	for (int i = 0; i < attributeCount; i++)
		glBindAttribLocation(program, i, attributes[i]);

	glLinkProgram(program);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		fprintf(stderr, "Could not link program!\n");
		GLint infoLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen)
		{
			char* infoLog = (char*)malloc(sizeof(char) * infoLen);
			glGetProgramInfoLog(program, infoLen, NULL, infoLog);
			fprintf(stderr, "%s\n", infoLog);
			free(infoLog);
		}
		glDeleteProgram(program);
		return 0;
	}
	return program;
}


static void genSphereGeometry(ModelGeometry* result)
{
	const uint32 numLongitude = 10;
	const uint32 numLatitude = 8;
	const float radius = 1.0f;
	const uint32 numVertices = numLongitude * numLatitude;
	const uint32 numIndices = numLongitude  * numLatitude * 6;

	const float pi = M_PI;
	const float piOver2 = pi * 0.5f;

	float invLat = 1.0f / (float)(numLatitude - 1);
	float invLong = 1.0f / (float)(numLongitude - 1);

	float vertices[numVertices * 3];
	uint32 indices[numIndices];

	float* v = vertices;
	uint32* i = indices;
	for (uint32 lat = 0; lat < numLatitude; lat++)
	{
		for (uint32 lng = 0; lng < numLongitude; lng++)
		{
			float const y = sin(-piOver2 + pi * lat * invLat);
			float const x = cos(2 * pi * lng * invLong) * sin(pi * lat * invLat);
			float const z = sin(2 * pi * lng * invLong) * sin(pi * lat * invLat);

			*v = x * radius; v++;
			*v = y * radius; v++;
			*v = z * radius; v++;

			assert(v <= vertices + numVertices * 3);
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

	result->Indices = HeapArray<uint32>::Init(numIndices);
	result->Positions = HeapArray<float>::Init(numVertices * 3);
	result->TextureCoords = HeapArray<float>::Init(numVertices * 2);
	result->Meshes = HeapArray<ModelGeometryMesh>::Init(1);

	memcpy(result->Indices.A, indices, sizeof(uint32) * numIndices);
	memcpy(result->Positions.A, vertices, sizeof(float) * numVertices * 3);
	memset(result->TextureCoords.A, 0, sizeof(float) * result->TextureCoords.Length);
	result->Meshes.A[0].NumTriangles = numIndices / 3;
	result->Meshes.A[0].StartIndex = 0;
	result->Meshes.A[0].TextureIndex = -1;
}

static void multiplyMatrices(float *out, float *a, float *b)
{
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++)
			out[y * 4 + x] = a[x] * b[y * 4] + a[4 + x] * b[y * 4 + 1] + a[8 + x] * b[y * 4 + 2] + a[12 + x] * b[y * 4 + 3];
}
static void translationMatrix(float *out, float x, float y, float z)
{
	out[0] = 1.0f; out[1] = 0.0f; out[2] = 0.0f; out[3] = 0.0f;
	out[4] = 0.0f; out[5] = 1.0f; out[6] = 0.0f; out[7] = 0.0f;
	out[8] = 0.0f; out[9] = 0.0f; out[10] = 1.0f; out[11] = 0.0f;
	out[12] = x;    out[13] = y;    out[14] = z;    out[15] = 1.0f;
}
static void rotationMatrix(float *out, float angle, float x, float y, float z)
{
	angle *= (float)M_PI / 180.0f;
	float c = cosf(angle), s = sinf(angle), c2 = 1.0f - c;
	out[0] = x*x*c2 + c;   out[1] = y*x*c2 + z*s; out[2] = x*z*c2 - y*s; out[3] = 0.0f;
	out[4] = x*y*c2 - z*s; out[5] = y*y*c2 + c;   out[6] = y*z*c2 + x*s; out[7] = 0.0f;
	out[8] = x*z*c2 + y*s; out[9] = y*z*c2 - x*s; out[10] = z*z*c2 + c;   out[11] = 0.0f;
	out[12] = 0.0f;         out[13] = 0.0f;         out[14] = 0.0f;         out[15] = 1.0f;
}
static void transformPosition(float *out, float *m, float *p)
{
	float d = 1.0f / (m[3] * p[0] + m[7] * p[1] + m[11] * p[2] + m[15]);
	out[2] = d * (m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14]);
	out[1] = d * (m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13]);
	out[0] = d * (m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12]);
}
static void transposeMatrix(float *out, float *m)
{
	out[0] = m[0]; out[1] = m[4]; out[2] = m[8]; out[3] = m[12];
	out[4] = m[1]; out[5] = m[5]; out[6] = m[9]; out[7] = m[13];
	out[8] = m[2]; out[9] = m[6]; out[10] = m[10]; out[11] = m[14];
	out[12] = m[3]; out[13] = m[7]; out[14] = m[11]; out[15] = m[15];
}
static void perspectiveMatrix(float *out, float fovy, float aspect, float zNear, float zFar)
{
	float f = 1.0f / tanf(fovy * (float)M_PI / 360.0f);
	float izFN = 1.0f / (zNear - zFar);
	out[0] = f / aspect; out[1] = 0.0f; out[2] = 0.0f;                       out[3] = 0.0f;
	out[4] = 0.0f;       out[5] = f;    out[6] = 0.0f;                       out[7] = 0.0f;
	out[8] = 0.0f;       out[9] = 0.0f; out[10] = (zFar + zNear) * izFN;      out[11] = -1.0f;
	out[12] = 0.0f;       out[13] = 0.0f; out[14] = 2.0f * zFar * zNear * izFN; out[15] = 0.0f;
}

static void fpsCameraViewMatrix(SDL_Window *window, float *view)
{
	// initial camera config
	static float position[] = { 0.0f, 0.3f, 1.5f };
	static float rotation[] = { 0.0f, 0.0f };

	// mouse look
	static int lastMouse[] = { 0, 0 };
	int mouse[2];
	auto buttons = SDL_GetMouseState(&mouse[0], &mouse[1]);
	if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		rotation[0] += (float)(mouse[1] - lastMouse[1]) * -0.2f;
		rotation[1] += (float)(mouse[0] - lastMouse[0]) * -0.2f;
	}
	lastMouse[0] = mouse[0];
	lastMouse[1] = mouse[1];

	float rotationY[16], rotationX[16], rotationYX[16];
	rotationMatrix(rotationX, rotation[0], 1.0f, 0.0f, 0.0f);
	rotationMatrix(rotationY, rotation[1], 0.0f, 1.0f, 0.0f);
	multiplyMatrices(rotationYX, rotationY, rotationX);

	// keyboard movement (WSADEQ)
	auto keys = SDL_GetKeyboardState(nullptr);

	float speed = keys[SDL_SCANCODE_LSHIFT] ? 0.1f : 0.01f;
	float movement[3] = { 0 };
	if (keys[SDL_SCANCODE_W]) movement[2] -= speed;
	if (keys[SDL_SCANCODE_S]) movement[2] += speed;
	if (keys[SDL_SCANCODE_A]) movement[0] -= speed;
	if (keys[SDL_SCANCODE_D]) movement[0] += speed;
	if (keys[SDL_SCANCODE_E]) movement[1] -= speed;
	if (keys[SDL_SCANCODE_Q]) movement[1] += speed;

	float worldMovement[3];
	transformPosition(worldMovement, rotationYX, movement);
	position[0] += worldMovement[0];
	position[1] += worldMovement[1];
	position[2] += worldMovement[2];

	// construct view matrix
	float inverseRotation[16], inverseTranslation[16];
	transposeMatrix(inverseRotation, rotationYX);
	translationMatrix(inverseTranslation, -position[0], -position[1], -position[2]);
	multiplyMatrices(view, inverseRotation, inverseTranslation); // = inverse(translation(position) * rotationYX);
}

#define LIGHTMAPPER_IMPLEMENTATION
#define LM_DEBUG_INTERPOLATION
#include "lightmapper.h"

#define INIPARSE_IMPLEMENTATION
#include "iniparse.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Graphics/stb_image.h"