CFLAGS = -lpthread -lm \
		-pipe -Wpedantic -Wextra -Wall -Wcast-align=strict -Wcast-qual \
		-Wfloat-equal -Wformat=2 -Winline -Wjump-misses-init -Wshadow \
		-Wbad-function-cast -Wmissing-declarations \
		-Wmissing-prototypes -Wnested-externs -Wstrict-prototypes
CFLAGS_OPTIM = -march=native -O3 $(CFLAGS)
CFLAGS_DEBUG = -g -fsanitize=address -fstack-protector-all \
		-Wstack-protector $(CFLAGS)

NAME = lcpu
NAME_DEBUG = $(NAME)_debug

all: $(NAME)

$(NAME): main.c
	gcc -o $@ $^ $(CFLAGS_OPTIM)

$(NAME_DEBUG): main.c
	gcc -o $@ $^ $(CFLAGS_DEBUG)

clear:
	rm -f $(NAME) $(NAME_DEBUG)
