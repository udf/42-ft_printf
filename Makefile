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

FUNCS=ft_printf ft_strcmp
NAME=libftprintf.a
INCLUDES=ft_printf
HEADERS=$(INCLUDES:%=includes/%.h)
OBJS=$(FUNCS:=.o)
CFLAGS=-Wall -Wextra -Werror -Wconversion

all: $(NAME)

$(NAME): $(OBJS)
	ar rcs $(NAME) $?

$(OBJS): %.o:srcs/%.c $(HEADERS)
	gcc $(CFLAGS) -I includes -c $(@:%.o=srcs/%.c)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all
