EXECUTABLE=game

$(EXECUTABLE): 
	$(CXX) -g --std=c++11 -I. -I../../Libs/Nxna -DSDL_HEADER="<SDL.h>"  `pkg-config --cflags sdl2` ../../Src/Build.cpp `pkg-config --libs sdl2` -lGL -lopenal -pthread -o $@

clean:
	rm $(EXECUTABLE)

