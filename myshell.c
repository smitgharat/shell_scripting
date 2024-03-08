//BT21CSE108 Smit Gharat
//OS Assignment-1 Make your own Shell

/********************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the single handler code (should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you not need to use any features for this assignment that are supported by C++ but not by C).
*******************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()
#include <ctype.h>

#define MAX_CMD_LEN 100
#define MAX_WORD_SIZE 1000

// Function prototypes
void displayPrompt();
char **getInput(char *);
int parseInput(char **);
void changeDirectory(char **);
void executeCommand(char **);
void executeParallelCommands(char **);
void executeSequentialCommands(char **);
void executeCommandRedirection(char **);

void executePipedCommands(char **arg) {
    int pipefd[2];
    pid_t child1_pid, child2_pid;

    if (pipe(pipefd) == -1) {
        perror("Shell: Pipe creation failed\n");
        return;
    }

    child1_pid = fork();

    if (child1_pid < 0) {
        perror("Shell: Forking child process failed\n");
        return;
    }

    if (child1_pid == 0) {
        // Child process 1
        close(pipefd[0]); // Close unused read end

        // Redirect stdout to the write end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]); // Close the write end

        // Execute the first command in the pipeline
        execvp(arg[0], arg);
        perror("Shell: Execution of first command failed\n");
        exit(1);
    } else {
        // Parent process
        child2_pid = fork();

        if (child2_pid < 0) {
            perror("Shell: Forking child process failed\n");
            return;
        }

        if (child2_pid == 0) {
            // Child process 2
            close(pipefd[1]); // Close unused write end

            // Redirect stdin to the read end of the pipe
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]); // Close the read end

            // Execute the second command in the pipeline (arguments[2])
            execvp(arg[2], arg + 2);
            perror("Shell: Execution of second command failed\n");
            exit(1);
        } else {
            // Parent process
            close(pipefd[0]); // Close both ends of the pipe in the parent
            close(pipefd[1]);

            // Wait for both child processes to finish
            wait(NULL);
            wait(NULL);
        }
    }
}

int main() {
    // Ignore SIGINT and SIGTSTP signals
    signal(SIGINT,SIG_IGN);    //to ignore signal interrupt occured by CTRL+C
    signal(SIGTSTP,SIG_IGN);   //to ignore signal of suspending execution occured by CTRL+Z

    char cwd[MAX_WORD_SIZE];   //variable to store current working directory
    char *cmd=NULL;          //line pointer for getline function and it is set to null so that buffer will be allocated to store line 
    size_t cmd_size=0;

    while(1)            //while loop which will run till termination condition is found
    {                
        int cmd_type=-1;     //variable to keep track of commands to be executed and also function to be called
        char **arguments=NULL;
        //getcwd(cwd, MAX_WORD_SIZE);
        //printf("%s$", cwd);
        displayPrompt();       // Display the shell prompt with the current working directory

        getline(&cmd,&cmd_size,stdin);  //getline used to get complete line input
        cmd=strsep(&cmd,"\n");         //to remove new line character from input

        if(strlen(cmd)==0)     //if NULL input, then continue
        {           
            continue;
        }

        arguments=getInput(cmd);        //extract token surrounded by delimiter '\n'

        if (strcmp(arguments[0],"##")==0)  //if the first command in sequential input is NULL then continue to new line
        {  
            continue;
        }

        cmd_type=parseInput(arguments);  //calling the function parse input to check if the command is single word or has parallel sequential and redirection and storing the output in cmd_type

        switch (cmd_type) {
            case 0:         //if input command is exit
                printf("Exiting shell...\n");
                return 0;
            case 1:         //if && is found in input then this function is executed that is parallel execution
                executeParallelCommands(arguments);
                break;
            case 2:         //if ## is found in input then this function is executed that is serial execution
                executeSequentialCommands(arguments);
                break;
            case 3:         //if > is found in input then this function is executed that is redirection output
                executeCommandRedirection(arguments);
                break;
            case 4:         //if | is found in input then this function is executed that is pipeline
                executePipedCommands(arguments);
                break;
            default:        // if none of the above is found then input is signle word command then this function is called
                executeCommand(arguments);
                break;
        }

    }

    return 0;
}


void displayPrompt()    //display prompt function it is called each time new line is made
{
    char cwd[MAX_WORD_SIZE];
    getcwd(cwd,MAX_WORD_SIZE);
    printf("%s$",cwd);      //prints current working directory
}


//function to get command-line input and split it into arguments
char **getInput(char *input) {
    char **arguments=malloc(MAX_CMD_LEN*sizeof(char *));    //allocate memory to store an array of char pointers (arguments)
    int arg_index=0;  //initialize an index for storing arguments

    char *token=strtok(input," ");   //" " is used as delimiter
    while(token!=NULL&&arg_index<MAX_CMD_LEN-1) // while loop used for tokenizing until reaching the maximum number of arguments
    {
        arguments[arg_index++]=token;   //storing the current token argument
        token=strtok(NULL," ");     //get the next token argument
    }
    arguments[arg_index]=NULL;      //setting the last set of arguments NULL indicating end of arguments

    return arguments;       //return array of arguments
}

//function to determine the type of command (single, parallel, sequential, or redirection)
int parseInput(char **arg) 
{
    if(strcmp(arg[0],"exit")==0)    //checking if the first argument is "exit"
    {
        return 0;
    }

    int cmd_type=4; //default for single command

    for (int i=0;arg[i]!=NULL;i++) 
    {
        if(strcmp(arg[i],"&&")==0)  //checking if the current argument is "&&" (parallel execution operator)
        {
            cmd_type=1;
            break;      //exit the loop
        } 
        else if(strcmp(arg[i],"##")==0) //checking if the current argument is "##" (sequential execution operator)
        {
            cmd_type=2;
            break;      //exit the loop
        } 
        else if(strcmp(arg[i],">")==0)  //checking if the current argument is ">" (output redirection operator)
        {
            cmd_type=3;
            break;      //exit the loop
        }
        else if(strcmp(arg[i],"|")==0)  //checking if the current argument is "|" (pipeline)
        { 
            cmd_type = 4;
            break;      //exit the loop
        }
    }

    return cmd_type;    //return cmd_type
}

//function to change the current working directory
void changeDirectory(char **arg) {
    const char *path = arg[1];
    if (path == NULL) 
    {
        printf("Shell: Incorrect command\n");   //handle incorrect command
    } 
    else 
    {
        if (chdir(path) == -1) 
        {
            printf("Shell: Incorrect command\n");   //handle incorrect command
        }
    }
}

//function to execute a single command
void executeCommand(char **arg) 
{
    if(strlen(arg[0])==0)       //checking if the first argument is an empty string
    {
        return;
    }
    if(strcmp(arg[0],"cd")==0)     //checking if the first argument is "cd" change directory command
    {
        changeDirectory(arg);      //function call to changeDirectory
        return;
    }
    if(strcmp(arg[0], "exit")==0)  //checking if the first argument is "exit" (exit command)
    {
        exit(0);
    }
    pid_t child_pid = fork();      //creating child process
    if(child_pid<0)                //if forked failed then incorrect command
    {
        printf("Shell: Incorrect command\n");
    } 
    else if (child_pid == 0) 
    {
        // Child process
        signal(SIGINT,SIG_DFL);    //to ignore signal interrupt occured by CTRL+C
        signal(SIGTSTP,SIG_DFL);   //to ignore signal of suspending execution occured by CTRL+Z

        if (execvp(arg[0], arg) < 0) //if execvp() failed
        {
            printf("Shell: Incorrect command\n");
        }
        exit(0);
    } 
    else 
    {
        // Parent process
        wait(NULL);     //wait for child process to get complete
    }
}

//function to execute multiple commands in parallel
void executeParallelCommands(char **arg) {
    char **temp_cmds=arg;
    char *cmds_to_implement[MAX_CMD_LEN][MAX_CMD_LEN];

    int i=0;        //row index
    int j=0;        //col index
    int count=0;    //count for total no of commands

    pid_t rc;       //process id for fork
    int status;     //status of child fork

    //while loop for extracting commands to execute into a 2D array
    while(*temp_cmds)   
    {
        if(strcmp(*temp_cmds,"&&")!=0)  //if && is found
        {
            cmds_to_implement[i][j++]=*temp_cmds;   //storing the commands in cmds_to_implement
        } 
        else 
        {
            cmds_to_implement[i][j]=NULL;   //terminating the current command
            i++;        //next row
            j=0;        //reset col index to 0
            count++;    //incrementing command count
        }
        temp_cmds++;    //command to move to next argument
    }

    //execute the commands in parallel
    for (int k=0;k<=count;k++) 
    {
        rc=fork();      //create a child process
        if(rc<0)        //if fork failed then 
        {
            perror("Shell: Incorrect command\n");
            exit(1);
        } 
        else if (rc == 0)   
        {
            // Child process
            signal(SIGINT,SIG_DFL);    //to ignore signal interrupt occured by CTRL+C
            signal(SIGTSTP,SIG_DFL);   //to ignore signal of suspending execution occured by CTRL+Z
            execvp(cmds_to_implement[k][0],cmds_to_implement[k]);   //execute command in child process
            perror("Shell: Incorrect command\n");   //execvp failure
            exit(1);
        }
    }

    //loop to wait for all child processes to finish
    for (int k=0;k<=count;k++) 
    {
        wait(&status);  //wait for child process to complete
    }
}

// Execute multiple commands sequentially
void executeSequentialCommands(char **arg) 
{
    char **temp_cmds=arg;   //initializing a temporary pointer to the input arguments

    while (*temp_cmds) 
    {
        char **cmds_to_implement=malloc(MAX_CMD_LEN*sizeof(char *));   //allocating memory for storing the command
        int j=0;

        while(*temp_cmds&&strcmp(*temp_cmds,"##")!=0) //checking if "##" is encountered
        {
            cmds_to_implement[j]=*temp_cmds;    //storing the commands in cmds_to_implement
            j++;
            temp_cmds++;    //move to next argument
        }

        cmds_to_implement[j]=NULL;  //setting last element to NULL to mark end of arguments
        executeCommand(cmds_to_implement);  //calling function to executeCommand

        if(*temp_cmds) 
        {
            temp_cmds++;    //move to next set of arguments
        }

        free(cmds_to_implement); //free memory allocated for cmds_to_implement
    }
}

// Execute a command with output redirection
void executeCommandRedirection(char **temp_cmds)
{
    char **arg=temp_cmds;   //initializing a pointer to the input arguments
    arg[1]=NULL;  //terminate the argument list at index 1

    if(strlen(temp_cmds[2])==0) 
    {
        printf("Shell: Incorrect command\n");   //handling incorrect command if the filename is empty
    } 
    else
    {
        pid_t rc=fork();    //create a child process
        if (rc<0)           //if fork failed
        {
            perror("Shell: Incorrect command\n");
            return;
        }
        else if(rc==0) 
        {
            // Child process
            signal(SIGINT,SIG_DFL);    //to ignore signal interrupt occured by CTRL+C
            signal(SIGTSTP,SIG_DFL);   //to ignore signal of suspending execution occured by CTRL+Z

            int fd=open(temp_cmds[2],O_CREAT|O_WRONLY|O_APPEND,0666);   //open or create the specified file
            if(fd<0)        //handle file open failure
            {
                perror("Shell: Incorrect command\n");
                exit(1);
            }

            //redirect stdout to the file
            if(dup2(fd,STDOUT_FILENO)==-1)  //handle dup2 failure
            {
                perror("Shell: Incorrect command\n");
                exit(1);
            }

            close(fd);      //closing the file descriptor

            //execute the command
            execvp(arg[0], arg);
            perror("Shell: Incorrect command\n");   //handle execvp failure
            exit(1);
        } 
        else 
        {
            // Parent process
            wait(NULL);     //wait for child process to finish
        }
    }
}
