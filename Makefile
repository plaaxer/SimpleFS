GXX=g++

WARNINGS=-Wall -Wno-sign-compare

# SFML Libraries
SFML_LIBS=-lsfml-graphics -lsfml-window -lsfml-system

# Final Target
simplefs: shell.o fs.o disk.o graphic_interface.o
	$(GXX) shell.o fs.o disk.o graphic_interface.o -o simplefs $(SFML_LIBS)

# Compile Shell with Graphical Interface Integration
shell.o: shell.cc
	$(GXX) -Wall shell.cc -c -o shell.o -g

# Compile File System Logic
fs.o: fs.cc fs.h
	$(GXX) -Wall fs.cc -c -o fs.o -g

# Compile Disk Logic
disk.o: disk.cc disk.h
	$(GXX) -Wall disk.cc -c -o disk.o -g

# Compile Graphical Interface
graphic_interface.o: graphic_interface.cpp
	$(GXX) -Wall graphic_interface.cpp -c -o graphic_interface.o -g $(SFML_LIBS)

# Clean Up
clean:
	rm -f simplefs shell.o fs.o disk.o graphic_interface.o
