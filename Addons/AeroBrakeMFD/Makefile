ORBITER_PATH=/home/gondos/dev/orbiter
INSTALL_PATH=/home/gondos/orbiter_test

#NOWARN=-fsanitize=address -Werror
ORBITER_SDK_INC=$(ORBITER_PATH)/Orbitersdk/include
SCNEDITOR_PATH=$(ORBITER_PATH)/Src/Plugin/ScnEditor
ORBITER_SDK_LIB=$(ORBITER_PATH)/Src/Orbitersdk/Orbitersdk.o

CDK_PATH=Cdk

ABMFD_SRC=$(wildcard *.cpp)
ABMFD_OBJ=$(foreach src, $(ABMFD_SRC), $(src:.cpp=.o))

%.o: %.cpp
	$(CXX) -g -c -std=c++17  $(INCLUDES) $(CXXFLAGS) -fPIC -o $@ $<

$(ABMFD_OBJ): INCLUDES+=-ICdk -I$(ORBITER_SDK_INC) -I$(ORBITER_PATH)/Src/Orbiter

libAeroBrakeMFD.so: $(ABMFD_OBJ) $(ORBITER_SDK_LIB)
	$(CXX) -g -std=c++17 -fPIC -g -shared -Wl,-soname,libAeroBrakeMFD.so -o $@ $^ -Wl,-rpath='$$ORIGIN'

install: libAeroBrakeMFD.so
	mkdir -p $(INSTALL_PATH)/Modules/Plugin
	cp libAeroBrakeMFD.so $(INSTALL_PATH)/Modules/Plugin
	cp -aR *.ld $(INSTALL_PATH)/

clean:
	find . -name '*.o'|xargs rm -f
	rm -f libAeroBrakeMFD.so
