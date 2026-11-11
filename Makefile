all: elevator_sim.exe
elevator_sim.exe: src/main.o src/input.o src/safety.o src/dispatch.o src/scheduler.o src/report.o
	gcc -o elevator_sim.exe src/main.o src/input.o src/safety.o src/dispatch.o src/scheduler.o src/report.o

src/main.o: src/main.c include/common.h include/input.h include/safety.h include/dispatch.h include/scheduler.h include/report.h
	gcc -Iinclude -c src/main.c -o src/main.o

src/input.o: src/input.c include/input.h include/common.h
	gcc -Iinclude -c src/input.c -o src/input.o

src/safety.o: src/safety.c include/safety.h include/common.h
	gcc -Iinclude -c src/safety.c -o src/safety.o

src/dispatch.o: src/dispatch.c include/dispatch.h include/scheduler.h include/common.h
	gcc -Iinclude -c src/dispatch.c -o src/dispatch.o

src/scheduler.o: src/scheduler.c include/scheduler.h include/common.h
	gcc -Iinclude -c src/scheduler.c -o src/scheduler.o

src/report.o: src/report.c include/report.h include/common.h
	gcc -Iinclude -c src/report.c -o src/report.o

run: elevator_sim.exe
	./elevator_sim.exe

test: tests/test_core.exe
	./tests/test_core.exe

tests/test_core.exe: tests/test_core.cpp src/safety.o src/scheduler.o src/dispatch.o include/common.h include/safety.h include/scheduler.h include/dispatch.h
	g++ -std=c++17 -Iinclude tests/test_core.cpp src/safety.o src/scheduler.o src/dispatch.o -lgtest -lgtest_main -pthread -o tests/test_core.exe

clean:
	del /Q /F src\*.o elevator_sim.exe tests\test_core.exe 2>nul || exit 0
