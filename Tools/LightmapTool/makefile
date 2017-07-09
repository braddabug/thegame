COMPILER = g++
LINKER = g++
COMPILER_FLAGS = -g `sdl2-config --cflags` --std=c++11 -I../../Src -DGLEW_STATIC -DLIGHTMAPPER
LINKER_FLAGS = 
EXECUTABLE = lt

SOURCES = LightmapTool/main.cpp LightmapTool/GeoLoader.cpp ../../Src/FileSystem.cpp LightmapTool/glew/glew.c
OBJECTS = $(SOURCES:.cpp=.o)

$(EXECUTABLE): $(OBJECTS)
	$(LINKER) $(LINKER_FLAGS) $(OBJECTS) -o $@  -lGL `sdl2-config --libs`
	
.cpp.o:
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@
	
clean:
	rm $(OBJECTS)
