CFLAGS=g++ -Wall -Wextra -pedantic -std=c++11
NAME=firmaHridele

all: $(NAME).o
	$(CFLAGS) -o $(NAME) $(NAME).o -lsimlib -lm
	rm -f *.o

clean:
	rm -f $(NAME)

run: $(NAME).o
	$(CFLAGS) -o $(NAME) $(NAME).o -lsimlib -lm
	rm -f *.o
	./$(NAME)