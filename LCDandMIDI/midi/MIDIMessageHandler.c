/*
 * MIDIStreamInput.c
 */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "MIDIParser.h"

// Get raw MIDI data (somehow)
// Pass data to MIDIParser

#define PAGE_SIZE 4096
#define LWHPS2FPGA_BRIDGE_BASE 0xff200000 // TODO: verify
#define MIDI_FPGA_OFFSET 0x0


volatile unsigned char* midi_fpga_mem;
void* bridge_map;

int main3() {

	MIDI_Message_In msg1;
	msg1.status = 0x90; // Note on, channel 0
	msg1.data1 = 60; // standard C
	msg1.data2 = 64; // half volume

	MIDI_Message_Out msg_out;
	msg_out = parseMIDIMessage(&msg1);

	printf("%d %d %d %d\n", msg_out.note_status, msg_out.pitch, msg_out.channel_num, msg_out.volume);


	// Bridge communication
	off_t midi_fpga_base = LWHPS2FPGA_BRIDGE_BASE; // TODO: ???

	int fd = EXIT_FAILURE;
	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) {
		perror("FAILED TO OPEN /DEV/MEM");
		exit(EXIT_FAILURE);
	}

	bridge_map = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, fd, midi_fpga_base);
	if (bridge_map == MAP_FAILED) {
		perror("FAILED TO MAP TO BRIDGE");
		goto cleanup;
	}

	midi_fpga_mem = (unsigned char*) (bridge_map + MIDI_FPGA_OFFSET);
	*(midi_fpga_mem) = msg_out.note_status;
	*(midi_fpga_mem + 1) = msg_out.channel_num;
	*(midi_fpga_mem + 2) = msg_out.pitch;
	*(midi_fpga_mem + 3) = msg_out.volume;
	*(midi_fpga_mem + 4) = msg_out.Attack;
	*(midi_fpga_mem + 5) = msg_out.Decay;
	*(midi_fpga_mem + 6) = msg_out.Sustain;
	*(midi_fpga_mem + 7) = msg_out.Release;

	if (munmap(bridge_map, PAGE_SIZE) < 0) {
		perror("FAILED TO UNMAP BRIDGE");
		goto cleanup;
	}

cleanup:
	close(fd);
	return 0;
}
