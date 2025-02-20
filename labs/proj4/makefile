include = -I ~/VScode/school/cs360-sp25/include
lib = ~/VScode/school/cs360-sp25/lib

EXECUTABLES: fakemake

all: $(EXECUTABLES) 

fakemake: fakemake.c
	gcc -g $(include) -o fakemake  fakemake.c $(lib)/libfdr.a

clean:
	rm $(EXECUTABLES) 