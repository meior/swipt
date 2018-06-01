SHELL=cmd.exe
RUN=swipt.exe
OBJS=main.o Point.o EData.o Network.o
main: $(OBJS)
		g++ -o $(RUN) $(OBJS) -lfreeglut_static -lopengl32 -lglu32 -lwinmm -lgdi32 -static-libgcc -Wall
main.o: ./src/main.cpp ./src/Network/Network.h
		g++ -c ./src/main.cpp -D FREEGLUT_STATIC
Network.o: ./src/Network/Network.cpp ./src/Network/Network.h
		g++ -c ./src/Network/Network.cpp
EData.o: ./src/EData/EData.cpp ./src/EData/EData.h
		g++ -c ./src/EData/EData.cpp
Point.o: ./src/Point/Point.cpp ./src/Point/Point.h
		g++ -c ./src/Point/Point.cpp
clean:
		del $(RUN) $(OBJS)