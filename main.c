/*
Andres Eduardo Lozano Diaz
Fabian Ernesto Ledezma Ledezma
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
const int tamanioInput = 1000;
const int tamanioComando = 100;
int lenComando = 0, posPipe = -1;

int encontrarPipe(char* comando[tamanioComando])//buscamos entre la entrada si hay in pipe "|"
{
    int i, ans = -1;
    for(i = 0; i < lenComando; ++i)
    {
        if(strcmp(comando[i], "|") == 0)
        {
            ans = i;
        }
    }
    return ans;
}

void encontrarInOut(char* comando[tamanioComando], char in[tamanioInput], char out[tamanioInput])
{
    int i, encontroIn = 0, encontroOut = 0;
    for(i = 0; i < lenComando; ++i)
    {
        if(strcmp(comando[i], "<") == 0)
        {
            comando[i] = NULL;//si encontramos un < lo ponemos en null para que se ejecute todo lo que este detras
            strcpy(in,comando[i + 1]); //En in ponemos lo que este adelante para ejecutarlo despues
            encontroIn = 1; //bandera que marca que encontramos un <
        }
        else if(strcmp(comando[i], ">") == 0)
        {
            comando[i] = NULL;
            strcpy(out,comando[i + 1]);
            encontroOut = 1;//bandera que marca que encontramos un >
        }
    }
    if(!encontroIn)
    {
        strcpy(in, "");//si no encontró "<" in lo pone vasio para que no se ejecute nada
    }
    if(!encontroOut)
    {
        strcpy(out, "");;//si no encontró ">" out lo pone vasio para que no se ejecute nada
    }
}

void split(char* input, char* comando[tamanioComando])
{
    int i = 0, j = 0;
    char palabra[tamanioInput], letra[2];
    palabra[0] = '\0';
    int encontroComillaS = 0, encontroComillaD = 0;
    while(input[i] != '\n' && j < tamanioComando)
    {
        if (input[i] == 34) // 34 = "
        {
            //opcion que une todo lo que este entre comillas dobles sin importar los espacios
            letra[0] = input[i];
            if(encontroComillaS)
            {
                strcat(palabra, letra);
            }
            if(encontroComillaD){
                encontroComillaD = 0;
            }
            else{
                encontroComillaD = 1;
            }
        }
        else if (input[i] == 39) // 39 = '
        {
            //opcion que une todo lo que este entre comillas simples sin importar los espacios
            letra[0] = input[i];
            if(encontroComillaD)
            {
                strcat(palabra, letra);
            }
            if(encontroComillaS){
                encontroComillaS = 0;
            }
            else{
                encontroComillaS = 1;
            }
        }
        else if(input[i] == 32 && ! encontroComillaD && ! encontroComillaS){ // 32 = espacio
            //si no encuentra comillas divide la entrada por espacios
            strcat(palabra, "\0");
            strcpy(comando[j], palabra); // copia la palabra en comando, si encontró comillas ingresa todo lo que esté entre comillas
            j++;
            palabra[0] = '\0';
        }
        else
        {
            letra[0] = input[i];
            strcat(palabra, letra);
        }
        ++i;
    }
    strcpy(comando[j], palabra);
    comando[j + 1] = NULL;
    lenComando = j; // lencomando representa cuantas palabras o argumentos tiene 
    palabra[0] = '\0';
}

void ejecutar(char* comando[tamanioComando], char* input, char* path, char* in, char* out)
{
    int pid = fork();
    if(pid == 0) // Hijo
    {
        if(strcmp(comando[lenComando], "&") == 0){
            comando[lenComando] = NULL; //como el ultimo elemento es & ejecutamos todo lo que este detras
        }
        // Ejecutar comando
        if(strcmp(in, "") != 0)
        {
            int fin = open(in, O_RDONLY, 0666);
            if(fin == -1)
            {
                fprintf(stderr, "Archivo no encontrado\n");
                exit(-1);
            }
            dup2(fin, STDIN_FILENO);
        }
        if(strcmp(out, "") != 0)
        {
            int fout = open(out, O_WRONLY | O_CREAT, 0666);
            if(fout == -1)
            {
                fprintf(stderr, "Archivo no creado correctamente\n");
                exit(-1);
            }
            dup2(fout, STDOUT_FILENO);
        }
        if(execvp(comando[0], comando) == -1)
        {
            if(strcmp(comando[0], "exit") == 0);// si el comando es "exit" solo pasamos
            else if(strcmp(input, "no_command") == 0) //si input es "no_command" sifgnifica que ingreso "!!"" de primer comando
            {
                fprintf(stderr, "Error. No hay comandos en el historial\n");
                exit(-1);
            }
            else if(strcmp(comando[0], "cd") == 0); //SI el comando es un cd passamos, puesto que el cambio de ruta lo hacemos en el padre
            else if(strcmp(comando[0], "") == 0); // si el usuario solo preciona enter no hacemos nada
            else
            {
                fprintf(stderr, "Error en el comando\n");
                exit(-1);
            }
        }
        exit(0);
    }
    else // Padre
    {
        // Espera a que el hijo termine
        if(strcmp(comando[lenComando], "&") != 0)
        {
            wait(NULL);
        }
        if(strcmp(comando[0], "cd") == 0)
        {
            if(chdir(comando[1]) != 0)//chdir nos permite cambiar el directorio en el que nos encontramos si retorna algo diferente de 0 la ruta buscada no existe
            {
                fprintf(stderr, "Error. No existe el directorio\n");
            }
            getcwd(path, PATH_MAX);//Volvemos a tomar la ruta por si la cambiamos

        }
    }
}

int main()
{
    
    int pid, i;
    char input[tamanioInput], last[tamanioInput], path[PATH_MAX], user[10000];
    char in[tamanioInput], out[tamanioInput];
    char* comando[tamanioComando];
    strcpy(input, "no_command");
    strcpy(last, "no_command");
    getcwd(path, PATH_MAX); //obtenemos la ruta de donde nos encontramos
    getlogin_r(user, 10000);
    i = 0;
    while(strcmp(input, "exit\n")) // exit es el comando para salir de la terminal
    {
        for(i = 0; i < tamanioComando; ++i)
        {
            if(comando[i] == NULL)
            {
                comando[i] = malloc(sizeof(char*));
            }
        }
        printf("%s:%s$ ", user, path);
        fgets(input, tamanioInput, stdin);// obtenemos la entrada
        fflush(stdin);
        if(strcmp(input, "!!\n") == 0)
        {
            strcpy(input, last);
        }
        split(input, comando); //dividimos la entrada por palabras
        posPipe = encontrarPipe(comando);
        encontrarInOut(comando, in, out); // vemos si la entrada tiene < o >
        if(posPipe != -1)
        {
            comando[posPipe] = NULL;
            int pipefd[2], stdIn = dup(1); // Guardar stdin para despues del uso de pipes
            if(pipe(pipefd) < 0)
            {
                fprintf(stderr, "Error al iniciar el pipe\n");
                continue;
            }
            int pidPipe = fork(); // Crear hijo para ejecutar primer comando
            if(pidPipe == 0)
            {
                close(pipefd[0]); // Cerrar pipe de lectura
                dup2(pipefd[1], STDOUT_FILENO); // Cambiar stdout al pipe de escritura
                close(pipefd[1]); // Cerrar pipe de escritura
                strcpy(out, "");
                ejecutar(comando, input, path, in, out);
                exit(0);
            }
            else
            {
                wait(NULL);
                close(pipefd[1]); // Cerrar pipe de escritura
                dup2(pipefd[0], STDIN_FILENO); // Cambiar stdin al pipe de lectura
                close(pipefd[0]); // Cerrar pipe de lectura
                strcpy(in, "");
                ejecutar(&comando[posPipe + 1], input, path, in, out);
                dup2(stdIn, STDIN_FILENO); // Devolver stdin
            }
        }
        else
        {
            ejecutar(comando, input, path, in, out);
        }
        strcpy(last, input); // copiamos el comando en last por si quiere 
    }
    return 0;
}

