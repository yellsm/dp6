/*
 * music_info_display.h
 *
 *  Created on: Mar 14, 2015
 *      Author: Haisin
 */

#ifndef MUSIC_INFO_DISPLAY_H_
#define MUSIC_INFO_DISPLAY_H_

#define MENU_PAGE 1
#define INSTRUMENT_PAGE 2
#define PLAY_PAGE 3

#ifdef __cplusplus
extern "C" {
#endif

void Music_menu_init(LCD_CANVAS *pCanvas, FONT_TABLE *font_table);

// --- Initial Menu page ---

// show options
// 1. Play Midi file
// 2. Live Play
int Music_menu_display(int option);

// -------------------

// --- Instrument selection page ---

// draw instrument icon and print name
int Music_instrument_display(char* instrument);

// ---------------------------------

// --- Play page ---

// draw generic wave parametrized by pitch(frequency) and volume(amplitude)
void Music_wave_display(double pitch, int volume); // to animate

// thin vertical bar
void Music_velocity_bar_display(int height); // to animate

// Persistent info display
void Music_volume_display(char* volume);
void Music_channel_display(char* channel_number);
void Music_preset_display(char* preset_number);

// -----------------

void Music_mod_display(int level); // not sure if relevant
void Music_pitch_display(int level); // not sure if relevant


#ifdef __cplusplus
}
#endif

#endif /* MUSIC_INFO_DISPLAY_H_ */
