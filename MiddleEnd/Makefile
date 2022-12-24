CC   := g++
NAME := O3
ARGS :=

LOGFILE := compileLog

CFLAGS := `/usr/lib/x86_64-linux-gnu/ImageMagick-6.9.11/bin-q16/Magick++-config --cxxflags --cppflags` -D _DEBUG -g -std=c++20 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -Wstack-usage=8192 -pie -fPIE -Wstack-protector -Wpedantic #-Wlarger-than=8192
SANITIZERS := -fsanitize=address,leak #,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr
LFLAGS := -lpthread -lasan -lmatplot `/usr/lib/x86_64-linux-gnu/ImageMagick-6.9.11/bin-q16/Magick++-config --ldflags --libs`
#	-L/usr/lib/ -lFestival -L/usr/lib/speech_tools/lib -lestools -lestbase -leststring
SRCDIR := src ../src
SRCDIR := $(shell find $(SRCDIR) -type d)

OBJDIR := objects
INCDIR := include ../include
INCDIR := $(shell find $(INCDIR) -type d)

DEPDIR := dependences

SOURCES     := $(wildcard $(addsuffix /*.cpp, $(if $(SRCDIR), $(SRCDIR), .)) )
OBJECTS     := $(patsubst %.cpp, $(if $(OBJDIR), $(OBJDIR)/%.o, ./%.o), $(notdir $(SOURCES)) )
DEPENDENCES := $(patsubst %.cpp, $(if $(DEPDIR), $(DEPDIR)/%.d, ./%.d), $(notdir $(SOURCES)) )

VPATH := $(SRCDIR)

.PHONY: clean cleanLog run  dependences cleanDependences makeDependencesDir objects check openLog rebuild execute

$(NAME):  dependences objects $(OBJECTS) cleanDependences
	@$(if $(OBJECTS), $(CC) $(OBJECTS) $(LFLAGS) -o $@ #2>>$(LOGFILE))

clean:
	@rm -rf $(OBJECTS) $(DEPENDENCES) $(DEPDIR) $(NAME)

cleanLog:
	@rm -rd .log/

openLog:
	@xdg-open $(shell ls .log/*.html -t | head -1)

check: clean $(NAME)
	@$(if $(NAME), valgrind --leak-check=full \
         --show-leak-kinds=all -s	          \
         ./$(NAME) $(ARGS))

rebuild: clean $(NAME)

run: $(NAME)
	@$(if $(NAME), ./$(NAME) $(ARGS))

dependences: makeDependencesDir $(DEPENDENCES)

makeDependencesDir:
	@$(if $(DEPDIR), mkdir -p $(DEPDIR))

$(if $(DEPDIR), $(DEPDIR)/%.d, %.d): %.cpp
	@$(CC) -M $(addprefix -I, $(INCDIR)) $< -o $@ #2>>$(LOGFILE)

cleanDependences:
	@rm -rf $(DEPENDENCES) $(DEPDIR)

objects:
	@$(if $(OBJDIR), mkdir -p $(OBJDIR))

$(if $(OBJDIR), $(OBJDIR)/%.o, %.o): %.cpp
	@$(CC) -c $(addprefix -I, $(INCDIR)) -save-temps $(CFLAGS) $(SANITIZERS) $< -o $@ #2>>$(LOGFILE)

include $(wildcard $(DEPDIR)/*.d)
