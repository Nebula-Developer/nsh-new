.PHONY: run

run:
	g++ -o ./main ./src/main.cpp ./src/input.cpp -ltermcap
	./main
