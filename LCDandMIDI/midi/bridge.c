#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define LWHPS2FPGA_BRIDGE_BASE 0xff200000
#define BRIDGE_OFFSET 0x0

#define BYTE unsigned char

void *bridge_map;

void cleanup();

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

volatile MIDI_Message_Out *bridge_mem;

int main34(int argc, char *argv[])
{
	int fd, ret = EXIT_FAILURE;
	MIDI_Message_Out value;
	off_t bridge_base = LWHPS2FPGA_BRIDGE_BASE;
	
	
	// Set default values
	value.volume = 121; 
	value.pitch = 60; 
	value.Attack = 2;
	value.Decay = 4;
	value.Sustain = 6;
	value.Release = 15;
	value.note_status = 8;
	value.channel_num = 3;

	/* open the memory device file */
	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	/* map the LWHPS2FPGA bridge into process memory */
	bridge_map = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED,
				fd, bridge_base);
	if (bridge_map == MAP_FAILED) {
		perror("mmap");
		cleanup(fd);
		return ret;
	}

	/* get the delay_ctrl peripheral's base address */
	bridge_mem = (MIDI_Message_Out *) (bridge_map + BRIDGE_OFFSET);

	/* write the value */
	*bridge_mem = value;

	if (munmap(bridge_map, PAGE_SIZE) < 0) {
		perror("munmap");
		cleanup(fd);
		return ret;
	}

	return 0;

}

void cleanup(int fd) {
		close(fd);
}
