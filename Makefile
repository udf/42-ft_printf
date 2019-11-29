# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: mhoosen <mhoosen@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2018/01/25 07:39:20 by mhoosen           #+#    #+#              #
#    Updated: 2018/08/21 10:29:03 by mhoosen          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

define GET_OFF_SRC
#include <stdio.h>
#include <unistd.h>
int main() {
	printf("%ld", (void *)&vdprintf - (void *)&write);
	return 0;
}
endef

FUNCS=ft_printf
NAME=libftprintf.a
INCLUDES=ft_printf
HEADERS=$(INCLUDES:%=includes/%.h)
OBJS=$(FUNCS:=.o)
CFLAGS=-Wall -Wextra -Werror -Wconversion

all: $(NAME)

$(NAME): $(OBJS)
	ar rcs $(NAME) $?

export GET_OFF_SRC
$(OBJS): %.o:srcs/%.c $(HEADERS)
	echo "$$GET_OFF_SRC" > get_off.c
	gcc get_off.c -o get_off
	gcc -DWRITE_VDPRINTF_OFF=$$(./get_off) $(CFLAGS) -I includes -c $(@:%.o=srcs/%.c)
	#rm -f get_off.c get_off

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all
