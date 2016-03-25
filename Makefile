all:  sample2D



sample2D: Assignment2.cpp glad.c
	g++ -o sample2D Assignment2.cpp glad.c  -lGL -ldl -lglfw -lftgl -lSOIL -I/usr/local/include -I/usr/local/include/freetype2 -L/usr/local/lib

clean:
	rm sample2D
