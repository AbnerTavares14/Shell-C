#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_INPUT_SIZE 1024

void compile_and_execute(char *filename, char *program, const char *name_file, char *name_equal)
{
    int status;

    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Erro ao criar o processo!\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        char program_gcc[MAX_INPUT_SIZE];
        snprintf(program_gcc, sizeof(program_gcc), "./%s", filename);
        execlp("gcc", "gcc", "-o", filename, name_equal, NULL);
        perror("Não foi possível compilar");
        exit(EXIT_FAILURE);
    }
    else
    {
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            pid_t id2 = fork();

            if (id2 == 0)
            {
                execl(program, filename, NULL);
                perror("execl falhou em executar o programa!");
                exit(EXIT_FAILURE);
            }
            else
            {
                wait(&status);
            }
        }
    }
}

void auto_compile(char *filename, char *program)
{
    char *directory = ".";
    DIR *dir;
    struct dirent *entry;
    struct stat st1, st2;
    // int status;

    if ((dir = opendir(directory)) == NULL)
    {
        perror("Erro ao abrir o diretório");
        exit(EXIT_FAILURE);
    }

    char *name_equal;
    int is_equal = 0;
    name_equal = strdup(filename);
    strcat(name_equal, ".c");

    while ((entry = readdir(dir)) != NULL)
    {
        const char *name_file = entry->d_name;
        if (strcmp(name_file, filename) == 0)
        {
            stat(name_file, &st1);
            stat(name_equal, &st2);

            if (st1.st_mtime < st2.st_mtime)
            {
                compile_and_execute(filename, program, name_file, name_equal);
                break;
            }
            else
            {
                execl(program, filename, NULL);
                perror("execl falhou em executar o programa!");
                exit(EXIT_FAILURE);
            }
        }
        else if (strcmp(name_equal, name_file) == 0)
        {

            compile_and_execute(filename, program, name_file, name_equal);
        }
    }
    closedir(dir);
    free(name_equal);
}

void execute_pipes(char *filename, char *path)
{
    char *token;
    char *token2;
    char *token3;
    char *args[64];  // vetor que vai guardar cada parte da entrada, ex: ["ls -l", "wc -l"]
    char *args2[64]; // vetor que vai guardar o primeiro programa a ser executado e o seu argumento, ex: ["ls", "-l"]
    char *args3[64]; // vetor que vai guardar o segundo programa a ser executado e o seu argumento, ex: ["wc", "-l"]
    int args_count = 0;
    int status;
    int fd[2];
    pipe(fd);

    token = strtok(filename, "|");

    while (token != NULL)
    {
        args[args_count++] = strdup(token);
        token = strtok(NULL, "|");
    }

    args[args_count] = NULL;
    args_count = 0;
    token2 = strtok(args[0], " \t\n");

    while (token2 != NULL)
    {
        args2[args_count++] = token2;
        token2 = strtok(NULL, " \t\n");
    }
    args2[args_count] = NULL;
    args_count = 0;

    token3 = strtok(args[1], " \t\n");

    while (token3 != NULL)
    {
        args3[args_count++] = token3;
        token3 = strtok(NULL, " \t\n");
    }
    args3[args_count] = NULL;

    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Fork falhou!");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        close(fd[1]);
        dup2(fd[0], 0);
        execvp(args3[0], args3);
        perror("Erro no execvp\n");
    }
    else
    {
        close(fd[0]);
        dup2(fd[1], 1);
        execvp(args2[0], args2);
        perror("Erro no execvp\n");
    }
}

int main(int argc, char *argv[])
{
    int status;
    char buffer[MAX_INPUT_SIZE];
    char *token;
    char *args[64];
    char *path = getenv("CAMINHO");

    while (1)
    {
        pid_t id;
        char filename[100];
        if (getcwd(buffer, sizeof(buffer)) != NULL)
        {
            printf("%s >> ", buffer);

            if (fgets(filename, sizeof(filename), stdin) == NULL)
            {
                perror("Erro ao ler a entrada");
                exit(1);
            }

            if (strcmp(filename, "\n") == 0)
            {
                continue;
            }
            char aux_copy[100];
            strncpy(aux_copy, filename, sizeof(aux_copy));

            char *flag_pipe = strstr(filename, "|");
            char *flag_background_execution = strstr(filename, "&");
            int background_execution = (flag_background_execution != NULL);

            if (background_execution)
            {
                filename[strlen(filename) - 2] = '\0'; // Remova "&\n"
            }

            int args_count = 0;
            token = strtok(filename, " \t\n");

            while (token != NULL)
            {
                args[args_count++] = strdup(token);
                token = strtok(NULL, " \t\n");
            }

            args[args_count] = NULL;

            if (strcmp(args[0], "cd") == 0)
            {
                if (chdir(args[1]))
                {
                    perror("Erro ao mudar de diretório");
                }
                for (int i = 0; i < args_count; i++)
                {
                    free(args[i]);
                }
                continue;
            }

            char program[MAX_INPUT_SIZE];

            if (path != NULL)
            {
                snprintf(program, sizeof(program), "%s%s", path, args[0]);
            }
            else
            {
                snprintf(program, sizeof(program), "./%s", args[0]);
            }

            printf("%s\n", program);

            id = fork();

            if (id < 0)
            {
                printf("Erro ao criar o processo\n");
                exit(EXIT_FAILURE);
            }

            if (id == 0)
            {
                if (path == NULL)
                {
                    auto_compile(args[0], program);
                }
                else if (flag_pipe == NULL)
                {
                    execvp(program, args);
                    perror("Comando não encontrado!");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    execute_pipes(aux_copy, path);
                }
            }
            else
            {
                if (!background_execution)
                {
                    wait(&status);
                }
                else
                {
                    continue;
                }
            }

            for (int i = 0; i < args_count; i++)
            {
                free(args[i]);
            }
        }
    }
    return 0;
}
