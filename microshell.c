/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: asamuilk <asamuilk@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/05/21 15:40:11 by asamuilk          #+#    #+#             */
/*   Updated: 2024/05/21 15:42:15 by asamuilk         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

void	error(char *message, char *arg)
{
	int	i;

	i = 0;
	while (message[i])
		i ++;
	write(STDERR_FILENO, message, i);
	if (arg)
	{
		i = 0;
		while (arg[i])
			i ++;
		write(STDERR_FILENO, arg, i);
	}
	write(STDERR_FILENO, "\n", 1);
}

int	count_pipes(char **av)
{
	int	res;

	res = 0;
	while (*av)
	{
		if (!strcmp(*av, "|"))
			res ++;
		av ++;
	}
	return (res);
}

void	free_pipes(int **pipes, int pipe_count)
{
	int	i;

	i = 0;
	while (i < pipe_count)
	{
		free(pipes[i]);
		i ++;
	}
	free(pipes);
}

void	exec(char **av, char **env)
{
	char	**begin;
	int		**pipes;
	int		pipe_count;
	int		i;
	int		pid;

	pipe_count = count_pipes(av);
	if (pipe_count == 0)
	{
		if (!strcmp(av[0], "cd"))
		{
			if (av[1] && !av[2])
			{
				if (chdir(av[1]) == -1)
					error("error: cd: cannot change directory to ", av[1]);
			}
			else
				error("error: cd: bad arguments", NULL);
		}
		else
		{
			pid = fork();
			if (pid == 0)
			{
				if (execve(av[0], av, env) == -1)
					error("error: cannot execute ", av[0]);
			}
			waitpid(pid, NULL, 0);
		}
		return ;
	}
	i = 0;
	pipes = malloc(pipe_count * sizeof(int *));
	while (i < pipe_count)
		pipes[i++] = malloc(2 * sizeof(int));
	i = 0;
	while (*av)
	{
		begin = av;
		if (i < pipe_count)
			pipe(pipes[i]);
		while (*av && strcmp(*av, "|"))
			av ++;
		if (*av)
		{
			*av = 0;
			av ++;
		}
		if (i == 0)
		{
			pid = fork();
			if (pid == 0)
			{
				close(pipes[i][0]);
				dup2(pipes[i][1], STDOUT_FILENO);
				close(pipes[i][1]);
				if (execve(begin[0], begin, env) == -1)
					error("error: cannot execute ", begin[0]);
			}
			close(pipes[i][1]);
			waitpid(pid, NULL, 0);
		}
		else if (i > 0 && i < pipe_count)
		{
			pid = fork();
			if (pid == 0)
			{
				close(pipes[i][0]);
				dup2(pipes[i - 1][0], STDIN_FILENO);
				close(pipes[i - 1][0]);
				dup2(pipes[i][1], STDOUT_FILENO);
				close(pipes[i][1]);
				if (execve(begin[0], begin, env) == -1)
					error("error: cannot execute ", begin[0]);
			}
			close(pipes[i - 1][0]);
			close(pipes[i][1]);
			waitpid(pid, NULL, 0);
		}
		else
		{
			pid = fork();
			if (pid == 0)
			{
				dup2(pipes[i - 1][0], STDIN_FILENO);
				close(pipes[i - 1][0]);
				if (execve(begin[0], begin, env) == -1)
					error("error: cannot execute ", begin[0]);
			}
			close(pipes[i - 1][0]);
			waitpid(pid, NULL, 0);
		}
		i ++;
	}
	free_pipes(pipes, pipe_count);
}


int	main(int ac, char **av, char **env)
{
	char	**begin;

	if (ac > 1)
	{
		av = &av[1];
		while (*av)
		{
			begin = av;
			while (*av && strcmp(*av, ";"))
				av ++;
			if (*av)
			{
				*av = 0;
				av ++;
			}
			if (*begin)
				exec(begin, env);
		}
	}
}
