/*
 * MIDIParser.c
 */
#include "MIDIParser.h"


// need some way to change the pitch information while a note is being held down
MIDI_Message_Out parseMIDIMessage(MIDI_Message_In* input) {
	MIDI_Message_Out output;

	// Set default values
	output.volume = 127; // max volume
	output.pitch = 60; // standard C
	output.Attack = 1;
	output.Decay = 1;
	output.Sustain = 1;
	output.Release = 1;

	// Check for NoteOn/NoteOff
//	BYTE statusByte = input->status >> 4;
//	if (statusByte == 0b1000 /* Note off */ || statusByte == 0b1001 /* Note on */) {
//		output.note_status = statusByte;
//		output.channel_num = 0b00001111 & input->status;
//		output.pitch = input->data1;
//		//BYTE velocity_scale = (1 + input->data2) >> 7;
//		float velocity_scale = input->data2 / 127.0f;
//		output.volume = (BYTE) (output.volume * velocity_scale);
//	}


	BYTE velocity = input->data2;
	BYTE statusByte = input->status >> 4;
	if(statusByte == 0b1001 && velocity == 0/*Note off*/){
		output.note_status = 0b1000;
		output.channel_num = 0b00001111 & input->status;
		output.pitch = input->data1;
		//BYTE velocity_scale = (1 + input->data2) >> 7;
		float velocity_scale = input->data2 / 127.0f;
		output.volume = (BYTE) (output.volume * velocity_scale);
	}else if(statusByte == 0b1001 && velocity != 0){/*Note on*/
		output.note_status = 0b1001;
		output.channel_num = 0b00001111 & input->status;
		output.pitch = input->data1;
		//BYTE velocity_scale = (1 + input->data2) >> 7;
		float velocity_scale = input->data2 / 127.0f;
		output.volume = (BYTE) (output.volume * velocity_scale);
	}

	return output;
}
