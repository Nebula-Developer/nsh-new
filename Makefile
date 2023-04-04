.PHONY: run

run:
	@g++ -o ./main ./src/main.cpp ./src/util.cpp ./src/graphics/input.cpp ./src/graphics/colors.cpp -ltermcap -std=c++11
	@./main
