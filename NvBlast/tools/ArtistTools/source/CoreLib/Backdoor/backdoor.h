#ifndef BACKDOOR_H

#define BACKDOOR_H

// This is a tiny helper class that let's you easily perform communication using shared file memory between
// two processes on the same machine.  It is built to comple on Windows; but relatively minor changes
// are necesary to make it work on other platforms; just change the memory mapped IO calls.
//
// This same technique would work as well if you simply opened a file on disk; though shared memory is much
// faster.
//
// This is not meant to be some kind of sophisticated inter-process communciations class.  Instead it's just
// used for the very basic use case where you need to easily send simple 'commands' between two proceses.
//
// This code snippet comes with a console application which can be used to send commands to another process
// and display received messaegs from that other process.
//
// Simply launch this console app with a command line argument indicating whether it is considered the 'server'
// or the 'client'.  You can test it by launching it twice; once as server and once as client and then send
// chat messages back and forth.
//
// Written by John W. Ratcliff on February 10, 2013 and released into the public domain.
//
// mailto:jratcliffscarab@gmail.com

class Backdoor
{
public:

	virtual void send(const char *str,...) = 0;	// This sends a 'command' to the other process. Uses the printf style format for convenience.

	// This method consumes an inomcing command from the other process and parses it into an argc/argv format.
	virtual const char **getInput(int &argc) = 0;

	virtual bool sendData(char* ptr, size_t size, int dataType) = 0;

	virtual bool receiveData(char* ptr, size_t size, int dataType) = 0;

	virtual bool checkMessage(const char* msg) = 0;

	// This method releases the Backdoor interface class.
	virtual void release(void) = 0;
protected:
	virtual ~Backdoor(void)
	{
	}
};

Backdoor * createBackdoor(const char *sendName,const char *receiveName);


#endif
