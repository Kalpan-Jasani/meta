/* command.cc - Command Class 
 *
 * This file contains functions related to execution and generation of commands
 * and hence, in a sense, the crux of the shell project
 * 
 * Note: For securtiy concerns, some parts of the code has been omitted. This
 * code is not for direct use by anybody and there is no liability!
 */

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "command.hh"
#include "shell.hh"

extern bool pushBuffer(FILE * file);	/* the function in lex */
Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

/* 
 * set the io input for command
 * 
 * This function is called from external sources to set the input redirection
 * for this command. 
 * Note: A command is composite command, not just one simple execution, but a
 * mix of redirects, pipes and simple commands, and more.
 *
 * Input:
 *  filename: the name of the file
 */
void Command::io_in_file(std::string * fileName)
{
	if(_inFile != NULL)
	{
		fprintf(stderr, "Ambiguous input redirect.\n");
		return;
	}
	_inFile = new std::string(fileName->c_str());
	fdCommand[0] = open(fileName->c_str(), O_RDONLY);
	if(fdCommand[0] == -1)
	{
		fprintf(stderr, "Could not open file %s, using standard input\n", fileName->c_str());
		delete _inFile;
		_inFile = NULL;
	}
}

/*
 * set output or error output
 * 
 * this function is called from external sources to set the output file
 * or error file for this command
 *
 * Input
 *  code: code is a meta parameter to discern between output and error
 *       1: output
 *       2: error
 *  fileName: the file name to write to
 * append: whether new stuff should be appended or the previous file should be
 * cleared.
 */   
void Command::io_out_file(int code, std::string * fileName, bool append)
{
	int fd;

    /* sanity checks */

    /* if for output and there is a previous out file already set
	if(code == 1 && _outFile != NULL) {
        /* 
         * print an error to stderr and return. This would still use the old
         * output file set
         */
		fprintf(stderr, "Ambiguous output redirect.\n");
		return;
	}

    /* if error file and there is a previous error file set */
	if(code == 2 && _errFile != NULL) {
		fprintf(stderr, "Ambiguous error redirect.\n");
		return;
	}

    /* if choosing to append */
	if(append) {
		fd = open(fileName->c_str(), O_WRONLY | O_APPEND | O_CREAT,0600);

        /* if failed to open the file */
		if(fd == -1) {
			fprintf(stderr, "Could not open file: %s\n", fileName->c_str());
            return;
		}
	}

	else {
		fd = open(fileName->c_str(), O_WRONLY | O_CREAT,0600);

        /* if failed to open file */
		if(fd == -1)
		{
			fprintf(stderr, "Could not open file: %s\n", fileName->c_str());
            return;
		}
	}

	switch(code) {
		case 1:
			_outFile = new std::string(fileName->c_str());
			fdCommand[1] = fd;
			break;

		case 2:
			_errFile = new std::string(fileName->c_str());
			fdCommand[2] = fd;
			break;
	}

} 

/* 
 * execute a command when the line has finished
 * 
 * The command should have been made ready by other calls before calling
 * this function
 */
void Command::execute() {
    // if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }

    /* 
     * return of fork call in the parent process 
     *
     * the last simple command's pid is stored in ret at a later point in
     * execution, if successfull.
     */
	int ret;

    /* 
     * to store file descriptors for original stdin, stdout and stderr before
     * changing them for each command
     */
	int fdOrig[3];  

	/* 
     * between to simple commands, the communiction is via pipes. The array is 
	 * reused for storing new pipes between pairs of commands.
	 */
	int fdPipe[2];

    /*
     * flag to say if a pipe is open. It is basically used to check before 
     * closing pipes, as there might not be a pipe open if there is just a 
     * single command 
     */
	int pipeCreated = false;	

	const char ** tmpPointer;

	/* 
	 * flag to indicate if last simple command is a built in command 
	 * this will be set in the iteration over simple commands later
	 */
	bool builtInEndFlag = false;

    /* store the original file descriptors to later restore them */
	fdOrig[0] = dup(0);
	fdOrig[1] = dup(1);
	fdOrig[2] = dup(2);

    /* checking if user redirect is set, then doing it */

    if (_outFile != NULL ) {	
		dup2(fdCommand[1], 1);
    }
    if (_inFile != NULL ) {
		dup2(fdCommand[0], 0);
    }
    if (_errFile != NULL) {
		dup2(fdCommand[2], 2);	
    }
    
    /* loop through list of simple commands */
	for(int i = 0, n = _simpleCommands.size(); i < (int) n; i++ )
	{
        /* take the next simple command */
		SimpleCommand * simpleCommand = _simpleCommands[i];

        /* name of the command to execute */
		const char * simpleCommandSimple = simpleCommand->_arguments[0]->c_str();

        /* if last command */
		if(i == (n - 1)) {
            /* checking if an outfile is not open */
			if(_outFile == NULL)	
                /* set original stdout to be the stdout */
				dup2(fdOrig[1], 1);
			else
                /* use the _outFile */
				
                /*
                 * the fdCommand holds the io redirections for the entire 
                 * command
				 */
				dup2(fdCommand[1], 1);	

			if(Shell::checkBuiltIn(simpleCommandSimple))
				builtInEndFlag = true;	/* last command is a built in command */

            /*
             * underscore is a reserved $ expansion to be the last argument
             * of the last command executed
             */
			Shell::uscoreVal->assign(*(simpleCommand->_arguments.back()));	/* set the uscoreVal to the correct last argument of the last simple command (fully expanded( which is done in my case)) */
		}

		else {  /* non-terminal simple command */

            /* create and pipe and check for error */
			if((pipe(fdPipe)) != 0) {
				perror("pipe\n");
			}
			pipeCreated = true;
			dup2(fdPipe[1], 1);	/* chaging stdout to that of pipe */
			
		}

		// check for builtin
        
        /*
         * Note: printenv is a special case.
         * we consider printenv as not a builtin (or at the least, not to 
         * be executed in this process, as we are making a child process for it 
         * This is for the reason that its output is to be redirected
         */
		if(Shell::checkBuiltIn(simpleCommandSimple)) {
			_currentSimpleCommand = simpleCommand;
            /* this line will also execute the builtin command (ofcourse synchronously) */
			if(! executeBuiltIn())	
				Shell::commandExit = 1;
			else
				Shell::commandExit = 0;
		}

		else {  /*new process else block*/
	   		ret = fork();
			if(ret == -1) {
				perror("fork\n");
				exit(2);	/* shell quits */		
			}
		
            /* inside the child process */
			if(ret == 0) {

                /* check for special case of printenv */
				if(strcmp(simpleCommand->_arguments[0]->c_str(),"printenv") ==
                  0) { 
					char **p=environ;
					while (*p!=NULL) 
					{
	      				printf("%s",*p);
						printf("\n");
						p++; 
					}
					exit(0); 
				}

                /* 
                 * apart from stdin, stdout, stderr, other open file descriptors
                 * are also duplicated in the fork call. Most of these are closed.
                 */
                    
                /* check if there were pipes created */
				if(pipeCreated) {
					close(fdPipe[0]);
					close(fdPipe[1]);	
				}

                /* 
                 * close original stdin, stdout, stderr file descriptors to
                 * be used in parent process for restoration
                 */

				close(fdOrig[0]);
				close(fdOrig[1]);
				close(fdOrig[2]);
		
                /* 
                 * close out, in and error files, that were created in the 
                 * parent
                 */

				if(_outFile != NULL) {
					close(fdCommand[1]);		
				}

				if(_inFile != NULL) {
						close(fdCommand[0]);
				}
		
				if(_errFile != NULL) {
					close(fdCommand[2]);
				}

                /* prepare parameters for execvp system call */

				const char ** argValues = (const char **) calloc(simpleCommand->_arguments.size() + 1, sizeof(char *));
				tmpPointer = argValues;

                /* set all the arguments in an array */ 
				for(
                  int j = 0, n = (int)(simpleCommand->_arguments.size()); 
                  j < n; 
                  j++) {

                    /*
                     * storing const char * type values in the argValues array 
                     * of const char * type values
                     */
				    *tmpPointer = simpleCommand->_arguments[j]->c_str();	
					tmpPointer++;
				}
				*tmpPointer = NULL;

                /* execvp system call in unistd.h */
				execvp(
                    /* path for executable */
                    simpleCommand->_arguments[0]->c_str(),

                    /* arguments to executable */
                    (char * const *) argValues);

                /* why is the code still here ? If it is, it is an error */
				fprintf(stderr, "Invalid command: %s\n", simpleCommand->_arguments[0]->c_str());

                /* dirty exit as it is a wasted child process */
			    _exit(2);
			}   /* inside child process */
		}

		/* this next part is only in the parent process */

		if(pipeCreated)
		{
            /* set input to pipe's out. This is for the next command */
			dup2(fdPipe[0], 0);	

            /* close extra reference to read or write end of pipe */
			close(fdPipe[0]);
			close(fdPipe[1]);

			pipeCreated = false;
		}
    }

    /* restore original stdin, stdout and stderr */
	dup2(fdOrig[0], 0);
	dup2(fdOrig[1], 1);
	dup2(fdOrig[2], 2);

    /* close extra file descriptors */
	close(fdOrig[0]);
	close(fdOrig[1]);
	close(fdOrig[2]);
	
	int status;

    /* if the command is to be executed in the background */
	if(_background) {
		Shell::pidBackground = ret;
	}

    /* if not a background and not ending with a built in command */
	if(!(_background) && !builtInEndFlag)
	{
        /* blocking call till last process finishes */
		waitpid(ret, &status, 0);	

        /* 
         * only do next statement if exit is normal 
         * (by the child itself calling exit or _exit ) 
         */
		if(WIFEXITED(status)) {
	        /* 
             * set the variable on updated result from this new child that 
             * finished 
             */
			Shell::commandExit = WEXITSTATUS(status);	

            /* 
             * if environment variable is active, and 
             * shell is in interactive mode and
             * status exit is bad
             */
			if(getenv("ON_ERROR") != NULL && 
              isatty(0) == 1 && 
              WEXITSTATUS(status) != 0) {
                    /* print whatever ON_ERROR prescribes */
					printf("%s\n", getenv("ON_ERROR"));
				}
		}
	}

    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

/* other code has been omitted */

SimpleCommand * Command::_currentSimpleCommand;
