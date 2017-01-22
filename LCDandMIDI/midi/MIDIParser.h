/*
 * MIDIParser.h
 *
 *  Created on: Mar 13, 2015
 *      Author: ECSE456
 */

#ifndef MIDIPARSER_H_
#define MIDIPARSER_H_

#define BYTE unsigned char

typedef struct MIDI_Message_t {
	BYTE status;
	BYTE data1;
	BYTE data2;
	double deltatime;
} MIDI_Message_In;

typedef struct MIDI_Data_t {
	BYTE note_status;                               // Note on / Note off
	BYTE channel_num;                               // Channel number
	BYTE pitch;                                     // Note value
	BYTE volume;                                    // Channel volume
	BYTE Attack;
	BYTE Decay;
	BYTE Sustain;
	BYTE Release;
} MIDI_Message_Out;

#ifdef __cplusplus
extern "C" {
#endif

MIDI_Message_Out parseMIDIMessage(MIDI_Message_In* input);

#ifdef __cplusplus
}
#endif

#endif /* MIDIPARSER_H_ */
