
#include <iostream>
#include <cstdlib>

using manespace std;

#ifndef MIDImsg_H
#define MIDImsg_H

class MIDImsgIn{
	
public:
//Default Constructor
	MIDImsgIn();


private:
//member variables
	BYTE status;
	BYTE data1;
	BYTE data2;
	double deltatime;

};

#endif
