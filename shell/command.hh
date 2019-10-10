#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

// Command Data Structure


struct Command {
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _background;

  /* 
   * store the opened file descriptors for _inFile, _outFile and _errFile
   * 
   * Note: This would store the fd for the entire command 
   */
  int fdCommand[3];  

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();
	void io_in_file(std::string * fileName);
	void io_out_file(int code, std::string * fileName, bool append);

  static SimpleCommand *_currentSimpleCommand;
	bool executeBuiltIn();	/*the command is in _currentSimpleCommand */
};

#endif
