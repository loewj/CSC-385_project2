BASE = object-scene-test

all: $(BASE)

OS := $(shell uname -s)

ifeq ($(OS), Linux) # Science Center Linux Boxes
  CPPFLAGS = -I/home/l/i/lib175/usr/glew/include
  LDFLAGS += -L/home/l/i/lib175/usr/glew/lib -L/usr/X11R6/lib
  LIBS += -lGL -lGLU -lglut
endif

ifeq ($(OS), Darwin) # Assume OS X
  CPPFLAGS += -D__MAC__ -stdlib=libstdc++ -Wno-deprecated-declarations -I/opt/local/include
  LDFLAGS += -framework GLUT -framework OpenGL /opt/local/lib/libGLEW.a
endif

ifdef OPT 
  #turn on optimization
  CXXFLAGS += -O2
else 
  #turn on debugging
  CXXFLAGS += -g
endif

CXX = g++ 

OBJ = $(BASE).o glsupport.o

$(BASE): $(OBJ)
	$(LINK.cpp) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJ) $(BASE)