.SETUP : 
	mkdir -p bin build

bin/sds : .SETUP build/main.o build/reader.o build/writer.o build/shared.o
	gcc build/main.o build/reader.o build/writer.o build/shared.o -o bin/sds -lrt -lpthread

build/shared.o : src/shared.c src/shared.h
	gcc src/shared.c -c -o build/shared.o -g

build/reader.o : src/reader.c src/reader.c src/shared.h
	gcc src/reader.c -c -o build/reader.o -g

build/writer.o : src/writer.c src/writer.h src/shared.h
	gcc src/writer.c -c -o build/writer.o -g

build/main.o : src/main.c src/main.h src/writer.h src/reader.h src/shared.h
	gcc src/main.c -c -o build/main.o -g

clean : 
	rm -rf bin/ build/ sim_out
