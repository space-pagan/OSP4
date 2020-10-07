/* Author: Zoya Samsonov
 * Date: October 6, 2020
 */

#include <sys/types.h>	 		//pid_t
#include <sys/wait.h>			//wait()
#include <unistd.h>				//fork(), execvp()
#include <csignal>				//kill(), SIGTERM
#include <sstream>				//istringstream
#include <vector>				//vector
#include <string>				//string
#include <cstring>				//strcpy
#include "error_handler.h"		//perrandquit
#include "child_handler.h"		//function defs for self

std::vector<pid_t> PIDS;

char** makeargv(std::string line, int& size) {
	// tokenizes an std::string on whitespace and converts it to char**
	// with the last element being a nullptr. Saves the column size to 'size'
	std::istringstream iss(line);
	std::vector<std::string> argvector;
	while (iss) {
		// extract characters until the next whitespace, and add it to
		// argvector
		std::string sub;
		iss >> sub;
		argvector.push_back(sub);
	}
	// instantiate the char** array to be the correct size to hold all the
	// tokens, plus nullptr
	char** out = new char*[argvector.size()];
	for (int i = 0; i < (int)argvector.size()-1; i++) {
		// instantiate the inner array and copy back the token
		out[i] = new char[argvector[i].size()];
		strcpy(out[i], argvector[i].c_str());
	}
	size = argvector.size();
	// the last token extracted from iss is "" (empty string), it's safe to
	// overwrite this with nullptr
	out[size-1] = nullptr;
	return out;
}

void freeargv(char** argv, int size) {
	for (int x = 0; x < size; x++) {
		delete[] argv[x];
	}
	delete[] argv;
}

void forkexec(std::string cmd, int& pr_count) {
	forkexec(cmd.c_str(), pr_count);
}

void forkexec(const char* cmd, int& pr_count) {
	int child_argc;
	char** child_argv = makeargv(cmd, child_argc);
	const pid_t child_pid = fork();
	switch(child_pid) {
		case -1:
			// fork() failed. Print the error and terminate.
			perrandquit();
			return;
		case 0:
			execvp(child_argv[0], child_argv);
			// this line is only reachable if execvp() failed. Print the error
			// and terminate (the child process)
			perrandquit();
			return;
		default:
			// fork() succeeded. Add child pid to list to quickly kill if
			// SIGINT. Increment pr_count and return
			PIDS.push_back(child_pid);
			pr_count++;
			freeargv(child_argv, child_argc);
			return;
	}
}

int updatechildcount(int& pr_count) {
	// checks if any children have exited, without waiting.
	// if a child has exited, decrement pr_count
	int wstatus;
	switch(waitpid(-1, &wstatus, WNOHANG)) {
		case -1:
			// waitpid() failed. Print the error and terminate.
			perrandquit();
			return -1;
		case 0:
			// no children have terminated. return and do not wait.
			return 0;
		default:
			// a child has terminated. Decrement pr_count and return
			// the child's exit status.
			pr_count--;
			return wstatus;
	}
}

int waitforanychild(int& pr_count) {
	// waits for a child to exit, and decrements pr_count when one has.
	int wstatus;
	switch(waitpid(-1, &wstatus, 0)) {
		case -1:
			// waitpid() failed. Print the error and terminate.
			perrandquit();
			return -1;
		default:
			// wait until a child terminates, then decrement pr_count
			// and return the child's exit status.
			pr_count--;
			return wstatus;
	}
}

void killallchildren() {
	// sends SIGTERM to any PID in the PIDS vector
	for (int p : PIDS) 
		// ignore failure due to child already terminated
		if (kill(p, SIGTERM) == -1 && errno != ESRCH) 
			perrandquit();
}
