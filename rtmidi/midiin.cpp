//*****************************************//
//  cmidiin.cpp
//
//  Simple program to test MIDI input and
//  use of a user callback function.
//
//*****************************************//

#include <iostream>
#include <cstdlib>
#include "RtMidi.h"


void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications.
  std::cout << "\nuseage: cmidiin <port>\n";
  std::cout << "    where port = the device to use (default = 0).\n\n";
  exit( 0 );
}

void mycallback( double deltatime, std::vector< unsigned char > *message, void */*userData*/ )
{
  unsigned int nBytes = message->size();
  for ( unsigned int i=0; i<nBytes; i++ )
    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
  if ( nBytes > 0 )
    std::cout << "stamp = " << deltatime << std::endl;
}


// This function should be embedded in a try/catch block in case of
// an exception.  It offers the user a choice of MIDI ports to open.
// It returns false if there are no ports available.

bool chooseMidiPort( RtMidiIn *rtmidi,unsigned int port );

int main( int argc, char ** /*argv[]*/ )
{
  RtMidiIn *midiin = 0;

  // Minimal command-line check.
  if ( argc > 2 ) usage();

  try {

    // RtMidiIn constructor
    midiin = new RtMidiIn();

    
    // Call function to select port.
    //default portNumber for QuNexus is 1
    unsigned int port = 1; 
    if ( chooseMidiPort( midiin,port ) == false ) goto cleanup;

    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the
    // queue instead of sent to the callback function.
    midiin->setCallback( &mycallback );

    // Don't ignore sysex, timing, or active sensing messages.
    midiin->ignoreTypes( false, false, false );

    std::cout << "\nReading MIDI input ... press <enter> to quit.\n";
    char input;
    std::cin.get(input);

  } catch ( RtMidiError &error ) {
    error.printMessage();
  }

 cleanup:

  delete midiin;

  return 0;
}

bool chooseMidiPort( RtMidiIn *rtmidi, unsigned portNumber )
{
 // std::string keyHit;
  std::string portName;
  unsigned int nPorts = rtmidi->getPortCount();
  if ( nPorts == 0 ) {
    std::cout << "No input ports available!" << std::endl;
    return false;
  }
  portName = rtmidi->getPortName(portNumber);
  std::cout << " Opening Input port #" << portNumber << ": " << portName << '\n';
  rtmidi->openPort(portNumber);
  return true;
}


