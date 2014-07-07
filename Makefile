# Copyright (c) 2014, Tamas Csala

BINARY = LoD
SRC_DIR = src
OBJ_DIR = .obj
PRECOMPILED_HEADER_SRC = $(SRC_DIR)/engine/oglwrap_all.h
FREETYPE_GL_DIR = $(SRC_DIR)/engine/gui/freetype-gl
FREETYPE_GL_ARCHIVE = $(FREETYPE_GL_DIR)/libfreetype-gl.a
THIRD_PARTY_DIR = $(OBJ_DIR)/third_party
GLEW_FOUND = $(THIRD_PARTY_DIR)/glew.found
GLFW3_FOUND = $(THIRD_PARTY_DIR)/glfw3.found
MAGICKPP_FOUND = $(THIRD_PARTY_DIR)/magickpp.found
ASSIMP_FOUND = $(THIRD_PARTY_DIR)/assimp.found
FREETYPE2_FOUND = $(THIRD_PARTY_DIR)/freetype2.found
BULLET_FOUND = $(THIRD_PARTY_DIR)/bullet.found
THIRD_PARTY_LIBS_FOUND = $(GLEW_FOUND) $(GLFW3_FOUND) $(MAGICKPP_FOUND) \
												 $(ASSIMP_FOUND) $(FREETYPE2_FOUND) $(BULLET_FOUND)

CPP_FILES := $(shell find -L $(SRC_DIR) -name '*.cc')
OBJECTS := $(subst $(SRC_DIR),$(OBJ_DIR),$(CPP_FILES:.cc=.o))
DEPS := $(OBJECTS:.o=.d)

CXX = clang++
CXX_PRECOMPILED_HEADER_EXTENSION = pch

PRECOMPILED_HEADER = $(PRECOMPILED_HEADER_SRC).$(CXX_PRECOMPILED_HEADER_EXTENSION)
PRECOMPILED_HEADER_DEP = $(subst $(SRC_DIR),$(OBJ_DIR),$(PRECOMPILED_HEADER).d)

BASE_CXXFLAGS = -std=c++11 -Wall -Weffc++ -Qunused-arguments \
					 			`pkg-config --cflags glfw3` \
					 			`pkg-config --cflags bullet` \
					 			`Magick++-config --cxxflags`

ifeq ($(MAKECMDGOALS),release)
	CXXFLAGS = -O3 -DOGLWRAP_DEBUG=0 $(BASE_CXXFLAGS)
else
	CXXFLAGS = -g -rdynamic $(BASE_CXXFLAGS)
endif

CXXFLAG_PRECOMPILED_HEADER = -include $(PRECOMPILED_HEADER_SRC)

OPENGL_LDFLAGS = -lGL -lGLEW
ASSIMP_LDFLAGS = -lassimp
GLFW_LDFALGS = `pkg-config --libs glfw3` \
								-lXxf86vm -lX11 -lXrandr -lXi -lXcursor -lpthread
IMAGEMAGIC_LDFLAGS = `Magick++-config --ldflags --libs`
FREETYPE_GL_LDFLAGS = -lfreetype -L$(FREETYPE_GL_DIR) -lfreetype-gl
BULLET_FLAGS = -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath

BASE_LDFLAGS = -lm $(OPENGL_LDFLAGS) $(ASSIMP_LDFLAGS) $(GLFW_LDFALGS) \
							 $(IMAGEMAGIC_LDFLAGS) $(FREETYPE_GL_LDFLAGS) $(BULLET_FLAGS)

ifeq ($(MAKECMDGOALS),release)
	LDFLAGS = $(BASE_LDFLAGS)
else
	LDFLAGS = -g -rdynamic $(BASE_LDFLAGS)
endif

# Terminal font colors
NORMAL = \e[0m
GREEN = \e[32m
RED = \e[91m
BOLD = \e[1m
YELLOW = \e[93m
CYAN = \e[96m

# The nocolor target is for sublime-text termial, that doesn't support font colors.
ifeq ($(MAKECMDGOALS),nocolor)
	printf = /bin/echo "$(1)$(subst $(OBJ_DIR)/,,$(2))"
else
	printf = /bin/echo -e "$(1)$(3)$(subst $(OBJ_DIR)/,,$(2))$(NORMAL)"
endif

.PHONY: all debug release nocolor clean clean_deps update

all: $(BINARY)
debug: $(BINARY)
nocolor: $(BINARY)
release: $(BINARY)

clean:
	@rm -f $(BINARY) -rf $(OBJ_DIR) -f $(PRECOMPILED_HEADER) -f $(FREETYPE_GL_ARCHIVE)

clean_deps:
	@find $(OBJ_DIR) -name '*.d*' | xargs rm -f

update:
	@git pull && git submodule update

ifneq ($(MAKECMDGOALS),clean) 						 # don't create .d files just to remove them...
ifneq ($(MAKECMDGOALS),clean_deps)
ifneq ($(MAKECMDGOALS),update)
$(shell mkdir -p $(OBJ_DIR))							 # make OBJ_DIR for a helper file
$(shell mkdir -p $(THIRD_PARTY_DIR))			 # make the dir for third party libs
$(shell echo 0 > $(OBJ_DIR)/objs_current)  # reset the built object counter
$(shell touch .MakefileObjsTotal) 				 # force the helper makefile to always run
$(shell $(MAKE) -f .MakefileObjsTotal all) # count the number of objects to be built
$(shell rm -rf $(OBJ_DIR)/deps)						 # remove the dir used to sign the first .d file
$(shell rm -rf .lockdir)									 # reset the lock for .make_get_progress.sh

# include the dependency files
-include $(DEPS)
-include $(PRECOMPILED_HEADER_DEP)
endif
endif
endif

# The dependency list files
%.d:
	@ # print some text for the user if its the first .d file
	@ if mkdir $(OBJ_DIR)/deps 2> /dev/null; then $(call printf,[  0%] ,Scanning dependencies,$(YELLOW)); fi;

	@ # if the file doesn't exist, create its directory,
	@ # else create a .d2 file to sign, that we have just created it (see %.o)
	@ if [ ! -f $@ ]; then mkdir -p $(dir $@); touch $(@:.d=.d2); fi;

	@ # create the dependency list using clang -MM
	@ $(CXX) $(CXXFLAGS) -MM $(subst $(OBJ_DIR),$(SRC_DIR),$(@:.d=.cc)) -MT $(@:.d=.o) -MF $@

	@ # Manually insert the precompiled header as a dependency
	@ sed -i 's,.o: ,.o: $(PRECOMPILED_HEADER) \\\n  ,' $@

# We need a dep list for the precompiled header too
$(PRECOMPILED_HEADER_DEP):
	@ if mkdir $(OBJ_DIR)/deps 2> /dev/null; then $(call printf,[  0%] ,Scanning dependencies,$(YELLOW)); fi;
	@ if [ ! -f $@ ]; then mkdir -p $(dir $@); touch $(@:.d=.d2); fi;
	@ $(CXX) $(CXXFLAGS) -x c++-header -MM $(subst $(OBJ_DIR),$(SRC_DIR),$(@:.$(CXX_PRECOMPILED_HEADER_EXTENSION).d=)) -MT $(subst $(OBJ_DIR),$(SRC_DIR),$(@:.d=)) -MF $@

%.o:
	@ $(call printf,$(shell ./.make_get_progress.sh) ,Building object $@,$(GREEN))

	@ # One of the depencies changed, which probably introduced new dependencies,
	@ # that we don't depend on now. So if a new header is included from a header,
	@ # normally we wouldn't track that header's changes, and the make would claim
	@ # that everything is up to date, even if the newly included header changed.
	@ # So we have to recalculate the dependencies for this cc file at next
	@ # compilation. The easiest to solve this is to remove the outdated .d file.
	@ # Alternatively we could rebuild the .d right now. I choose the second option
	@ # as it makes the makefile's output nicer (will only print Checking Dependecies
	@ # at the first compilation).
	@ #
	@ # But note, that we shouldn't create the .d file if we have just built them
	@ # for the includes (the .d2 signs this)
	@ if [ -f $(@:.o=.d2) ]; then rm $(@:.o=.d2);	else $(CXX) $(CXXFLAGS) -MM $(subst $(OBJ_DIR),$(SRC_DIR),$(@:.o=.cc)) -MT $@ -MF $(@:.o=.d); sed -i 's,.o: ,.o: $(PRECOMPILED_HEADER) \\\n  ,' $(@:.o=.d); fi;

	@ $(CXX) $(CXXFLAGS) $(CXXFLAG_PRECOMPILED_HEADER) -c $(subst $(OBJ_DIR),$(SRC_DIR),$(@:.o=.cc)) -o $@

$(PRECOMPILED_HEADER):
	@ $(call printf,$(shell ./.make_get_progress.sh) ,Building precompiled header $@,$(CYAN))
	@ rm -f $(PRECOMPILED_HEADER)-*
	@ if [ -f $(subst $(SRC_DIR),$(OBJ_DIR),$@).d2 ]; then rm $(subst $(SRC_DIR),$(OBJ_DIR),$@).d2;	else $(CXX) $(CXXFLAGS) -x c++-header -MM $(@:.$(CXX_PRECOMPILED_HEADER_EXTENSION)=) -MT $@ -MF $(subst $(SRC_DIR),$(OBJ_DIR),$@).d; fi;
	@ $(CXX) $(CXXFLAGS) -x c++-header $(@:.$(CXX_PRECOMPILED_HEADER_EXTENSION)=) -o $@

$(FREETYPE_GL_ARCHIVE):
	@ $(call printf,$(shell ./.make_get_progress.sh) ,Building archive $@,$(GREEN))
	@ cd $(FREETYPE_GL_DIR) && cmake . > /dev/null && $(MAKE) all > /dev/null

$(BINARY): $(THIRD_PARTY_LIBS_FOUND) $(OBJECTS) $(FREETYPE_GL_ARCHIVE)
	@ $(call printf,[100%] ,Linking executable $@,$(BOLD)$(RED))
	@ $(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.h:
	@
%.hpp:
	@
%.inl:
	@
%:
	@

$(GLEW_FOUND):
	@if `pkg-config --atleast-version=1.8 glew`; then touch $(GLEW_FOUND); else /bin/echo -e "$(RED)GLEW version 1.8 or newer is required $(NORMAL)"; exit 1; fi;

$(GLFW3_FOUND):
	@if `pkg-config --atleast-version=3.0 glfw3`; then touch $(GLFW3_FOUND); else /bin/echo -e "$(RED)GLFW version 3.0 or newer is required $(NORMAL)"; exit 1; fi;

$(MAGICKPP_FOUND):
	@if `pkg-config --atleast-version=6.0 Magick++`; then touch $(MAGICKPP_FOUND); else /bin/echo -e "$(RED)Magick++ version 6.0 or newer is required $(NORMAL)"; exit 1; fi;

$(ASSIMP_FOUND):
	@if `pkg-config --atleast-version=3.0 assimp`; then touch $(ASSIMP_FOUND); else /bin/echo -e "$(RED)Assimp version 3.0 or newer is required $(NORMAL)"; exit 1; fi;

$(FREETYPE2_FOUND):
	@if `pkg-config --atleast-version=15.0 freetype2`; then touch $(FREETYPE2_FOUND); else /bin/echo -e "$(RED)FreeType2 version 15.0 or newer is required $(NORMAL)"; exit 1; fi;

$(BULLET_FOUND):
	@if `pkg-config --atleast-version=2.5 bullet`; then touch $(BULLET_FOUND); else /bin/echo -e "$(RED)Bullet version 2.5 or newer is required $(NORMAL)"; exit 1; fi;

