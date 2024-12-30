mainTargets = main.o util.o pixel.o image.o
testFile := flower.bmp
testOutFile := flower_edited.bmp
FLAGS := -Wall -g
# CC_FLAGS := -lm

# CLEAN_OTHER := *Copy.wav *copy.wav *.txt

go: $(mainTargets)
	gcc $(mainTargets) -o go $(FLAGS)

main.o: main.c main.h image.o pixel.o util.o
	gcc -c main.c $(FLAGS)

util.o: util.c util.h
	gcc -c util.c $(FLAGS)

pixel.o: pixel.c pixel.h
	gcc -c pixel.c $(FLAGS)

image.o: image.c image.h pixel.o util.o
	gcc -c image.c $(FLAGS)

clean:
	rm ./go *.o -f

test:
	make go
	./go $(testFile) $(testOutFile)
