/*
 Anna JOLLY
 260447198
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

struct JOB;
typedef struct JOB
{
    int pid;
    char *cmd;
    struct JOB *next;
    struct JOB *previous;
}JOB;


JOB *addJob(JOB *head, int newpid, char *newcmd)
{
    JOB *temp;
    JOB *holder;
    holder = (JOB *)malloc(sizeof(JOB));
    holder = head;
    temp = (JOB *)malloc(sizeof(JOB));
    temp->pid = newpid;
    temp->cmd = newcmd;
    if(head == NULL)
    {
        head=temp;
        head->previous = NULL;
        head->next = NULL;
    }
    else
    {
        //find last job
        while(holder->next != NULL)
        {
            holder = holder->next;
        }
        holder->next = temp;
        //add new job
        temp->next = NULL;
        temp->previous = holder;
    }
    return head;
}

JOB *jobs(JOB *head)
{
    int status;
    JOB *holder;
    holder = (JOB *)malloc(sizeof(JOB));
    
    //print jobs
    holder = head;
    printf("Background jobs:\n");
    if (holder == NULL) {
	printf("\tNone.\n");
    }
    else {
	while(holder != NULL) {
            if(waitpid(holder->pid, &status, WNOHANG) == 0) {
        	printf("\tpid #%d: %s\n",holder->pid, holder->cmd);
            }
            holder=holder->next;
	}
    }
    return head;
}

void sendToFG(int pid)
{
	int status;
	waitpid(pid, &status, WUNTRACED);
}

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line;
    size_t linecap = 0;

    printf("%s", prompt);
    //allocate memory block
    line = (char *) malloc (101);
    length = getline(&line, &linecap, stdin);
    
    if (length <= 0) {
        exit(-1);
    }
    
    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    }
    else {
        *background = 0;
    }
    
    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (int j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
    args[i++] = '\0';
    
    return i;
}

void freecmd(char *args[], int cnt)
{
    //reset args/history values to 0
    for(int i=0; i<cnt; i++) {
        args[i] = NULL;
    }
}
    
int getHistCmd(char *args[], char *history[], int *bg)
{
    int found = 0;
    char *token, *loc;
    int i = 0;
    
    //traverse last 10 entries of history looking for request
    for (int k=0; k < 10; k++) {
        //pull out the number corresponsing to that history entry
        if( history[k] != NULL) {
            char *entry = strdup(history[k]);
            char *entryNo = strsep(&entry, " ");
            //compare history request number to number of this history entry
            if ( strcmp(args[0], entryNo) == 0 ) {
                //indicate that we have found the entry
                found = 1;
                //if correct entry, re-populate args
                // Check if background is specified..
                if ((loc = index(entry, '&')) != NULL) {
                    *bg = 1;
                    *loc = ' ';
                }
                else {
                    *bg = 0;
                }
                while ((token = strsep(&entry, " \t\n")) != NULL) {
                    for (int j = 0; j < strlen(token); j++)
                        if (token[j] <= 32)
                            token[j] = '\0';
                    if (strlen(token) > 0)
                        args[i++] = token;
                }
                args[i++] = '\0';
                return i;
            }
            //if k=9 and we haven't found the command, return -1
            else if(k==9 && !found) {
                return -1;
            }
        }
        //if we reach a null value and we haven't found the command, return -1
        else if(k==9 && !found) {
            return -1;
        }
    }
    return i;
}

int addToHistory(char *args[], char *history[], int *count, int *mostRecent, int *bg)
{
    char *countStr;
    
    countStr = (char *) malloc (4);
    //increment count
    *count = *count + 1;
    //increment or loop mostRecent count
    if (*mostRecent<9) {
        *mostRecent = *mostRecent + 1;
    }
    else {
        *mostRecent = 0;
    }
    int l = 0;
    while (args[l] != NULL) {
        //if first loop, put count in before args
        if (l == 0) {
            sprintf(countStr, "%d", *count);
            history[*mostRecent] = countStr;
        }
        //add in argument
        strcat(history[*mostRecent], " ");
        strcat(history[*mostRecent], args[l]);
        l++;
    }
    //if command has ampersand, add it in
    if (*bg) {
        strcat(history[*mostRecent], " ");
        strcat(history[*mostRecent], "&");
    }
    return 0;
}

int main()
{
    char *args[20];
    int bg;
    int status;
    int pid, pid2;
    char *history[10];
    int count=0;
    int mostRecent=-1;
    int stdoutFD, skipFork, doNotAdd;
    int i = 0;
    
    freecmd(args, 20);
    freecmd(history, 10); 
    
    JOB *head = NULL;

    while(1)
    {
        bg=0;
        i = 0;
        doNotAdd = 0;
        skipFork = 0;
        int cnt = getcmd("\n>>  ", args, &bg);
	
	//check if command was empty command
	if (args[0] == NULL) {
	    doNotAdd = 1;
	    skipFork = 1;
	}
        //if command is a history request (ie command is a number)
        //repopulate args with command from history
        //if command not found, -1 is returned
        if (cnt > 1) {
	    if ( atoi(args[0]) != 0 && args[1] == NULL ) {
            	cnt = getHistCmd(args, history, &bg);
            	if (cnt < 0) {
                    printf("No command found in history.\n");
            	}
            }
	}
        
        //if command is cd
        if (cnt > 1) {
            if (strcmp(args[0], "cd") == 0) {
                skipFork = 1;
                //if correctly call cd
                if (args[2] == NULL && args[1] != NULL) {
                    if (chdir(args[1]) < -1) {
                        printf("Error changing directory.\n");
                        doNotAdd = 1;
                    }
                }
                else {
                    printf("Correct syntax is \"cd filepath\".\n");
                    doNotAdd = 1;
                }
            }
        }
        
        //if command is pwd
        if (cnt > 1) {
            if (strcmp(args[0], "pwd") == 0) {
                skipFork = 1;
                //if correctly call cd
                if (args[3] == NULL) {
                    char *buffer = (char *) malloc (50);
                    char *ptr = getcwd(buffer, 50);
                    printf("Current working directory: %s\n", ptr);
                }
                else {
                    printf("Correct syntax is \"pwd filepath\".\n");
                    doNotAdd = 1;
                }
            }
        }
        
        //if command is history
	if (cnt > 1) {
	    if (strcmp(args[0], "history") == 0) {
		printf("History:\n");
		for(int z=0; z<10; z++) {
		    if(history[z] != NULL) {
			printf("\t%s\n", history[z]);
		    }
		}
	    }
	}

	//if command is fg
        if (cnt > 1) {
        	if (strcmp(args[0], "fg") == 0) {
        		if (args[1] == NULL) {
        			printf("Correct syntax is \"fg p_id\"\n");
        		}
        		else {
        			sendToFG(atoi(args[1]));
        		}
        	}
        }
        
        
        //if command is jobs
        if (cnt > 1) {
	    if (strcmp(args[0], "jobs") == 0) {
            	if (args[1] != NULL) {
                    printf("Correct syntax is \"jobs\".\n");
                    doNotAdd = 1;
            	}
            	else {
                    jobs(head);
            	}
            }
	}
        
        //if command is exit
        if (cnt > 1) {
	    if (strcmp(args[0], "exit") == 0) {
            	exit(0);
            }
	}

        //if user enters CTRL+D
        if (cnt > 1) {
	    if (strcmp(args[0], "^D") == 0) {
            	exit(0);
            }
	}
    
	    //Add command to history if command was found
	    if (cnt > 1 && !doNotAdd) {
            addToHistory(args, history, &count, &mostRecent, &bg);
	    }

        //output redirection if necessary
        if (cnt > 1) {
            for (int c = 0; c<cnt-1; c++) {
                if (strcmp(args[c], ">") == 0) {
                    skipFork = 1;
                    //redirect to file
                    //save stdout file descriptor
                    stdoutFD = dup(1);
                    close(1);
                    //open file
                    int fd = open(args[c+1], O_RDWR|O_CREAT|O_APPEND|O_TRUNC,0666);
                    args[c] = '\0';
                    if ((pid2=fork()) == 0) {
                        execvp(args[0], args);
                    }
                    else {
                        //if bg
                        if(strcmp(args[c-1], "&") == 0) {
		    	    char *cmd = (char *) malloc (40);
		    	    int a=0;
		    	    while(args[a] != NULL) {
				strcat(cmd, " ");
				strcat(cmd, args[a]);
				a++;
		    	    }
                            head = addJob(head, pid2, args[0]);
                        }
                        else {
                            //while child has not terminated
                            while (waitpid(pid2, &status, WUNTRACED) != pid2)
                            {
                                //wait
                                ;
                            }
                        }
                    }
                    //redirect stdout to terminal
                    close(fd);
                    dup2(stdoutFD, 1);
                    close(stdoutFD);
                    skipFork = 1;
                }
            }
        }

        if (skipFork == 0) {
            if( ( pid = fork() ) == 0)
            {
                //child does this
                execvp(args[0], args);
                exit(0);
            }
            
            else
            {
                //parent does this
                if (bg) {
		    char *cmd = (char *) malloc (40);
		    int a=0;
		    while(args[a] != NULL) {
			strcat(cmd, " ");
			strcat(cmd, args[a]);
			a++;
		    }
                    head = addJob(head, pid, cmd);
                }
                else
                {
                    //while child has not terminated
                    waitpid(pid, &status, WUNTRACED);
                }
            }
        }

        freecmd(args, 20);
    }
    return 0; 
}
