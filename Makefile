mainTargets = main.c util.o pixel.o image.o
mainDeps := main.c main.h $(mainTargets)
testFile := flower.bmp
testOutFile := flower_edited.bmp
FLAGS := -Os
# CC_FLAGS := -lm

# CLEAN_OTHER := *Copy.wav *copy.wav *.txt

go: $(mainDeps)
	gcc $(mainTargets) -o go $(FLAGS)

util.o: util.c util.h
	gcc -c util.c $(FLAGS)

pixel.o: pixel.c pixel.h
	gcc -c pixel.c $(FLAGS)

image.o: image.c image.h
	gcc -c image.c $(FLAGS)

clean:
	rm ./go *.o -f

test:
	make go
	./go $(testFile) $(testOutFile)
