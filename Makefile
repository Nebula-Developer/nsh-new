.PHONY: run

run:
	@g++ -o ./main ./src/main.cpp ./src/input.cpp ./src/util.cpp ./src/colors.cpp -ltermcap -std=c++11
	@./main
