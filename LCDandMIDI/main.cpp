// ============================================================================
// Copyright (c) 2013 by Terasic Technologies Inc.
// ============================================================================
//
// Permission:
//
//   Terasic grants permission to use and modify this code for use
//   in synthesis for all Terasic Development Boards and Altera Development
//   Kits made by Terasic.  Other use of this code, including the selling
//   ,duplication, or modification of any portion is strictly prohibited.
//
// Disclaimer:
//
//   This VHDL/Verilog or C/C++ source code is intended as a design reference
//   which illustrates how these types of functions can be implemented.
//   It is the user's responsibility to verify their design for
//   consistency and functionality through the use of formal
//   verification methods.  Terasic provides no warranty regarding the use
//   or functionality of this code.
//
// ============================================================================
//
//  Terasic Technologies Inc
//  9F., No.176, Sec.2, Gongdao 5th Rd, East Dist, Hsinchu City, 30070. Taiwan
//
//
//                     web: http://www.terasic.com/
//                     email: support@terasic.com
//
// ============================================================================

#include "terasic_os_includes.h"
#include "LCD_Lib.h"
#include "lcd_graphic.h"
#include "font.h"
#include "music_info_display.h"
#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <thread>
#include <queue>
#include "MIDIParser.h"
#include "RtMidi.h"

/* LCD defs */
#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
#define BIT_KEY_0	( 0x00200000 )
#define BIT_KEY_1	( 0x00400000 )
#define BIT_KEY_2	( 0x00800000 )
#define BIT_KEY_3	( 0x01000000 )
#define BIT_KEY_ALL	( BIT_KEY_0 | BIT_KEY_1 | BIT_KEY_2 | BIT_KEY_3 )
#define LCD_REFRESH_RATE_IN_MICROSECONDS 1//100000
#define LCD_READ_KEY_RATE_IN_MICROSECONDS 125000

/* Bridge defs */
#define PAGE_SIZE 4096
#define LWHPS2FPGA_BRIDGE_BASE 0xff200000
#define BRIDGE_OFFSET 0x0
#define BYTE unsigned char
void *bridge_map;
volatile MIDI_Message_Out *bridge_mem;

using namespace std;

queue<MIDI_Message_t> midi_msgs_to_LCD;
queue<MIDI_Message_t> midi_msgs_to_Bridge;

bool take_midi_input = false;

volatile sig_atomic_t quit_interrupt_flag_in_bridge = 0;
volatile sig_atomic_t quit_interrupt_flag_in_lcd = 0;

void quit_interrupt_handler(int sig);

void lcd()
{
	void *virtual_base;
	int fd;
	uint32_t scan_input;

	LCD_CANVAS LcdCanvas;

	// map the address space for the LED registers into user space so we can interact with them.
	// we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 )
	{
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}

	virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );

	if( virtual_base == MAP_FAILED )
	{
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	signal(SIGINT, quit_interrupt_handler); // Register signals

	printf("\nLCD Demo\n");

	LcdCanvas.Width = LCD_WIDTH;
	LcdCanvas.Height = LCD_HEIGHT;
	LcdCanvas.BitPerPixel = 1;
	LcdCanvas.FrameSize = LcdCanvas.Width * LcdCanvas.Height / 8; // frame size in bytes
	LcdCanvas.pFrame = (void *) malloc(LcdCanvas.FrameSize);

	if (LcdCanvas.pFrame == NULL)
	{
		printf("\nFailed to allocate lcd frame buffer\n");
	}

	else
	{
		// setup lcd
		LCDHW_Init(virtual_base);
		LCDHW_BackLight(true);
		LCD_Init();
		DRAW_Clear(&LcdCanvas, LCD_WHITE); // clear screen

		Music_menu_init(&LcdCanvas, &font_16x16);

		int context = Music_menu_display(0);
		int play_live_selected = 1;
		char* instrument = "none";

		DRAW_Refresh(&LcdCanvas);

		printf("\nLCD Running...\n");

		while(1)
		{
			scan_input = alt_read_word( ( virtual_base + ( ( uint32_t )( ALT_GPIO2_EXT_PORTA_ADDR ) & ( uint32_t )( HW_REGS_MASK ) ) ) );

			if ((~scan_input & BIT_KEY_0)) // key is active-low BACK BUTTON
			{
				printf("Back button pressed\n");
				break;
			}

			if ((~scan_input & BIT_KEY_1)) // key is active-low ENTER BUTTON
			{
				printf("Enter button pressed\n");

				if (context == MENU_PAGE && play_live_selected)
				{
					DRAW_Clear(&LcdCanvas, LCD_WHITE);
					DRAW_PrintString(&LcdCanvas, 16, 0, "Please choose", LCD_BLACK, &font_16x16);
					DRAW_PrintString(&LcdCanvas, 16, 8, "an instrument", LCD_BLACK, &font_16x16);
					DRAW_Refresh(&LcdCanvas);

					sleep(2);

					DRAW_Clear(&LcdCanvas, LCD_WHITE);
					context = Music_instrument_display("piano");
					instrument = "piano";
					DRAW_Refresh(&LcdCanvas);
				}

				else if (context == INSTRUMENT_PAGE)
				{
					if(strcmp(instrument, "piano") == 0)
					{
						// some piano specific setting configuration...

						context = PLAY_PAGE;
					}

					else if(strcmp(instrument, "guitar") == 0)
					{
						// some piano specific setting configuration...

						context = PLAY_PAGE;
					}
				}
			}

			if ((~scan_input & BIT_KEY_2)) // key is active-low RIGHT BUTTON
			{
				printf("Right button pressed\n");

				if (context == MENU_PAGE)
				{
					play_live_selected = 1;
					DRAW_Clear(&LcdCanvas, LCD_WHITE);
					Music_menu_display(0);
					DRAW_Refresh(&LcdCanvas);
				}

				else if (context == INSTRUMENT_PAGE)
				{
					if(strcmp(instrument, "piano") == 0)
					{
						DRAW_Clear(&LcdCanvas, LCD_WHITE);
						context = Music_instrument_display("guitar");
						instrument = "guitar";
						DRAW_Refresh(&LcdCanvas);
					}

					//					else if(strcmp(instrument, "guitar") == 0)
					//					{
					//
					//					}
				}

			}

			if ((~scan_input & BIT_KEY_3)) // key is active-low LEFT BUTTON
			{
				printf("Left button pressed\n");

				if (context == MENU_PAGE)
				{
					play_live_selected = 0;
					DRAW_Clear(&LcdCanvas, LCD_WHITE);
					Music_menu_display(1);
					DRAW_Refresh(&LcdCanvas);
				}

				else if (context == INSTRUMENT_PAGE)
				{
					if(strcmp(instrument, "guitar") == 0)
					{
						DRAW_Clear(&LcdCanvas, LCD_WHITE);
						context = Music_instrument_display("piano");
						instrument = "piano";
						DRAW_Refresh(&LcdCanvas);
					}

					//					else if(strcmp(instrument, "piano") == 0)
					//					{
					//
					//					}
				}
			}

			if (context == PLAY_PAGE)
			{
				static double pitch = 0.0;
				static unsigned int volume = 0;
				static unsigned int channel = 1;

				take_midi_input = true;

				if (!midi_msgs_to_LCD.empty())
				{
					MIDI_Message_In midi_in = midi_msgs_to_LCD.front(); // get midi message from queue
					midi_msgs_to_LCD.pop(); // remove from queue

					//					cout << "\nMIDI INPUT: ";
					cout << "LCD : Byte " << hex << 0 << " = " << (int)midi_in.status << ", ";
					cout << "LCD : Byte " << hex << 1 << " = " << (int)midi_in.data1 << ", ";
					cout << "LCD : Byte " << hex << 2 << " = " << (int)midi_in.data2 << ", ";
					cout << "LCD : stamp = " << (double)midi_in.deltatime << endl;

					// parse midi input
					MIDI_Message_Out midi_data;
					midi_data = parseMIDIMessage(&midi_in);
					pitch = midi_data.pitch;
					volume = midi_data.volume;
					channel = midi_data.channel_num;

					//					cout << "pitch = " << pitch << ", " << "volume = " << volume << ", " << "channel = " << channel << endl;

					double tmp = (double) volume/127 * 100;
					volume = (int)tmp;

					//					cout << "tmp = " << tmp << endl;
					//					cout << "vol = " << volume << endl;

					if (channel > 99) channel = 99;

				}

				if(quit_interrupt_flag_in_lcd)
				{
					cout << "LCD : CTRL-C caught" << endl;
					quit_interrupt_flag_in_lcd = 0;
					break;
				}

				// animate
				DRAW_Clear(&LcdCanvas, LCD_WHITE);

				// convert int to c-style string without itoa
				char* str_vol = (to_string(volume)).c_str();
				char* str_channel = (to_string(channel)).c_str();

				Music_volume_display(str_vol);
				//cout << "VOLUME:" << str_vol << endl;
				Music_channel_display(str_channel);
				Music_preset_display("1");
				Music_wave_display(pitch, volume);
				DRAW_Refresh(&LcdCanvas);

				usleep(LCD_REFRESH_RATE_IN_MICROSECONDS);
			}

			if (context != PLAY_PAGE)
				usleep(LCD_READ_KEY_RATE_IN_MICROSECONDS);
		}

		printf("Exiting...\n");

		DRAW_Clear(&LcdCanvas, LCD_WHITE);
		DRAW_PrintString(&LcdCanvas, 0, 0, "QUITTING...", LCD_BLACK, &font_16x16);
		DRAW_Refresh(&LcdCanvas);

		usleep(1000000);

		// clearing screen before exiting
		DRAW_Clear(&LcdCanvas, LCD_WHITE);
		DRAW_Refresh(&LcdCanvas);

		free(LcdCanvas.pFrame);
	}

	// clean up memory mapping and exit
	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 )
	{
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return 1;
	}

	close( fd );

	printf(">>>>>>>>>>>End of LCD\n");
}

void mycallback( double deltatime, vector< unsigned char > *message, void */*userData*/ )
{
	MIDI_Message_In midi_msg;

	unsigned int nBytes = message->size();

	cout << "\nMidi Message Size = " << nBytes << endl;

	for ( unsigned int i=0; i<nBytes; i++ )
		cout << "Cmidiin : " << "Byte " << i << " = " << (int)message->at(i) << ", ";

	midi_msg.status = (BYTE) message->at(0);
	midi_msg.data1 = (BYTE) message->at(1);
	midi_msg.data2 = (BYTE) message->at(2);

	cout << endl;
	if ( nBytes > 0 )
	{
		//cout << "stamp = " << deltatime << endl;
		midi_msg.deltatime = deltatime;
		if (take_midi_input)
		{
			midi_msgs_to_LCD.push(midi_msg);
			midi_msgs_to_Bridge.push(midi_msg);
		}
	}
}

// This function should be embedded in a try/catch block in case of
// an exception.  It offers the user a choice of MIDI ports to open.
// It returns false if there are no ports available.
bool chooseMidiPort( RtMidiIn *rtmidi, unsigned int portNumber )
{
	string portName;
	unsigned nPorts = rtmidi->getPortCount();
	if ( nPorts == 0 ) {
		cout << "No input ports available!" << endl;
		return false;
	}
	portName = rtmidi->getPortName(portNumber);
	cout << " Opening Input port #" << portNumber << ": " << portName << '\n';
	rtmidi->openPort( portNumber );

	return true;
}

int midi()
{
	RtMidiIn *midiin = 0;

	try
	{
		midiin = new RtMidiIn();

		unsigned int port = 5;

		if ( chooseMidiPort( midiin, port ) == false ) goto cleanup;

		// Set callback function.  This should be done immediately after
		// opening the port to avoid having incoming messages written to the
		// queue instead of sent to the callback function.
		midiin->setCallback( &mycallback );

		// Don't ignore sysex, timing, or active sensing messages.
		midiin->ignoreTypes( false, false, false );

		cout << "\nReading MIDI input ...";
		char input;
		cin.get(input);

	} catch ( RtMidiError &error ) {
		error.printMessage();
	}

	cleanup:

	delete midiin;

	cout << ">>>>>>>>>>>End of Midi" << endl;

	return 0;
}

void quit_interrupt_handler(int sig)
{
	quit_interrupt_flag_in_bridge = 1;
	quit_interrupt_flag_in_lcd = 1;
}

int bridge()
{
	int fd, ret = EXIT_FAILURE;

	cout << "\nRunning bridge...\n" << endl;

	signal(SIGINT, quit_interrupt_handler); // Register signals

	cout << "\nDisabling all bridges" << endl;
	cout << "\nEnabling Lightweight HPS-FPGA bridge" << endl;
	system("./setup.sh"); // open bridge

	off_t bridge_base = LWHPS2FPGA_BRIDGE_BASE;

	while (true)
	{
		if (!midi_msgs_to_Bridge.empty())
		{
			MIDI_Message_In midi_in = midi_msgs_to_Bridge.front(); // get midi message from queue
			midi_msgs_to_Bridge.pop(); // remove from queue
			cout << "(Before parsing) Bridge : " << "BYTE 0 = " << hex << midi_in.status << ", " << "BYTE 1 = " << hex << midi_in.data1 << ", "  << "BYTE 2 = " << hex << midi_in.data2 << endl;

			MIDI_Message_Out value;
			value = parseMIDIMessage(&midi_in);

			// Set default values
//			value.volume = 121;
//			value.pitch = 60;
//			value.Attack = 2;
//			value.Decay = 4;
//			value.Sustain = 6;
//			value.Release = 15;
//			value.note_status = 8;
//			value.channel_num = 3;

			/* open the memory device file */
			fd = open("/dev/mem", O_RDWR|O_SYNC);
			if (fd < 0)
			{
				perror("open");
				exit(EXIT_FAILURE);
			}

			/* map the LWHPS2FPGA bridge into process memory */
			bridge_map = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, fd, bridge_base);
			if (bridge_map == MAP_FAILED)
			{
				perror("mmap");
				close(fd);
				return ret;
			}

			/* get the delay_ctrl peripheral's base address */
			bridge_mem = (MIDI_Message_Out *) (bridge_map + BRIDGE_OFFSET);

			cout << "Sending Midi messages to Bridge" << endl;

			cout << "(After parsing) Bridge : " << hex << "pitch = " << value.pitch << ", " << "volume = " << value.volume << ", "  << "channel = " << value.channel_num << endl;

			/* write the value */
			*bridge_mem = value;

			cout << "Finished Midi messages to Bridge" << endl;

			if (munmap(bridge_map, PAGE_SIZE) < 0)
			{
				perror("munmap");
				close(fd);
				return ret;
			}
		}

		else if(quit_interrupt_flag_in_bridge)
		{
			cout << "Bridge : CTRL-C caught" << endl;
			quit_interrupt_flag_in_bridge = 0;
			break;
		}

		else
			usleep(1000); // to reduce cpu cycle wasting
	}

	cout << "\nDisabling FPGA..." << endl;
	system("./cleanup.sh"); // cleanup bridge
	cout << ">>>>>>>>>>>End of Bridge" << endl;

	return 0;
}

int main()
{
	thread lcd_thread (lcd);
	thread midi_thread (midi);
	thread bridge_thread (bridge);

	lcd_thread.join();
	midi_thread.join();
	bridge_thread.join();

	cout << ">>>>>>>>>>>End Of Program." << endl;

	return 0;
}
