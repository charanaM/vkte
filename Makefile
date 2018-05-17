all: vkte

vkte: vkte.c
	gcc vkte.c -o vkte

clean:
	rm vkte