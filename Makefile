CFLAGS=-Wall -Wextra -Werror -O2
TARGETS=lab1kamN3247 libkam.so libavg.so

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -rf *.o $(TARGETS)

lab1kamN3247:lab1kamN3247.c kamN3247.h
	gcc $(CFLAGS) -o lab1kamN3247 lab1kamN3247.c -ldl
	
libkam.so:libkam.c kamN3247.h
	gcc $(CFLAGS) -shared -fPIC -o libkam.so libkam.c -ldl -lm 
	
libavg.so: libavg.c kamN3247.h
	gcc $(CFLAGS) -shared -fPIC -o libavg.so libavg.c -ldl -lm
	