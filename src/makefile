HEADERS = processes.h
SOURCE = PMan.c processes.c
EXECUTABLE = PMan

$(EXECUTABLE): $(SOURCE) $(HEADERS)
	gcc -o $@ $(SOURCE) -Wall
	
.PHONY: clean

clean:
	rm -f PMan