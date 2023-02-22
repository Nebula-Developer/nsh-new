.PHONY: run

run:
	g++ -o ./main ./src/main.cpp ./src/input.cpp ./src/util.cpp -ltermcap
	./main
