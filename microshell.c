#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

static int error(const char *msg, const char *arg)
{
	while (*msg)
		write(2, msg++, 1);
	if (arg)
		while (*arg)
			write(2, arg++, 1);
	write(2, "\n", 1);
	return 1;
}

static int exec(int ac, char *av[], char *env[], int input_fd)
{
	if (dup2(input_fd, STDIN_FILENO) == -1)
		return error("error: fatal", 0);
	close(input_fd);
	av[ac] = 0;
	execve(av[0], av, env);
	return error("error: cannot execute ", av[0]);
}

int main(int i, char *av[], char *env[])
{
	int	input_fd = dup(STDIN_FILENO);
	if (input_fd == -1)
		return error("error: fatal", 0);
	i = 0;
	while (av[i] && av[++i])
	{
		// printf("1. av[%d]: %s\n", i, av[i]);
		av = &av[i];
		i = 0;
		while(av[i] && strcmp(av[i], ";") && strcmp(av[i], "|"))
			i++;
		// printf("2. av[%d]: %s\n", i, av[i]);
		if (av[i] && !strcmp(av[0], "cd"))
		{
			// printf("3. av[%d]: %s\n", i, av[i]);
			if (i != 2)
				error("error: cd: bad arguments", 0);
			else if (chdir(av[1]) == -1)
				error("error: cd: cannot change directory to ", av[1]);
		}
		else if (i != 0 && av[i] && !strcmp(av[i], "|"))
		{
			int fd[2];
			if (pipe(fd) == -1)
				return error("error: fatal", 0);
			pid_t pid = fork();
			if (pid == 0)
			{
				if (dup2(fd[1], STDOUT_FILENO) == -1)
					return error("error: fatal", 0);
				close(fd[1]);
				exec(i, av, env, input_fd);
			}
			else if (pid != -1)
			{
				close(fd[1]);
				close(input_fd);
				input_fd = fd[0];
			}
			else
				return error("error: fatal", 0);
		}
		else //if (i != 0 && (av[i] == NULL || strcmp(av[i], ";") == 0))
		{
			pid_t pid = fork();
			if (pid == 0)
				exec(i, av, env, input_fd);
			else if (pid != -1)
			{
				for (close(input_fd); waitpid(-1, 0, WUNTRACED) != -1;);
				if ((input_fd = dup(STDIN_FILENO)) == -1)
					return error("error: fatal", 0);
			}
			else
				return error("error: fatal", 0);
		}
	}
	close(input_fd);
	return 0;
}