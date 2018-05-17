#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define CTRL_Q 17

struct termios original;

char return_keypress()
//wait for character input
{
	int err=0;
	char inp;
	while((err=read(0, &inp, 1))!=1)
	{
		if(err==-1)
			kill_process("read");
	}

	return inp;
}

void clear_screen()
//clear the screen by using escape sequence
{
	if(write(1, "\x1b[2J", 4)==-1)
		kill_process("write");
}

void handle_keypress()
//handle the character entered
{
	char inp=return_keypress();

	switch(inp)
	{
		case CTRL_Q:	exit(0);
	}
}

void kill_process(char* msg)
//for debugging
{
	char buffer[100];
	sprintf(buffer, "vkte: %s", msg);
	perror(buffer);
	exit(1);
}

void switch_to_raw_mode()
//switches input mode to raw mode from canonical mode
{
	struct termios get_mode;

	if(tcgetattr(0, &get_mode)==-1)
		kill_process("tcgetattr");
	
	get_mode.c_cc[VTIME]=1;	//set min timeout for read
	get_mode.c_cc[VMIN]=0;	//set min number of characters to read for read after which to return
	get_mode.c_iflag&=~(IXON|ICRNL|INPCK|ISTRIP|BRKINT);	//disable ctrl-q, ctrl-s and ctrl-m
	get_mode.c_oflag&=~(OPOST);		//disable carriage return and newline
	get_mode.c_lflag&=~(ECHO|ISIG|ICANON|IEXTEN);	//disable ctrl-v, ctrl-c, ctrl-z and canonical mode
	get_mode.c_cflag|=~(CS8);

	if(tcsetattr(0, TCSAFLUSH, &get_mode)==-1)
		kill_process("tcsetattr");
}

void switch_to_canonical_mode(struct termios orig)
//switches input mode back to canonical mode
{
	if(tcsetattr(0, TCSAFLUSH, &orig)==-1)
		kill_process("tcsetattr");
}

void switch_to_canonical_mode_atexit()
//switches input mode back to canonical mode(atexit)
{
	if(tcsetattr(0, TCSAFLUSH, &original)==-1)
		kill_process("tcsetattr");
}

int main(int argc, char const *argv[])
//main
{
	struct termios original_mode;
	if(tcgetattr(0, &original_mode)==-1)
		kill_process("tcgetattr");
	if(tcgetattr(0, &original)==-1)
		kill_process("tcgetattr");;
	atexit(switch_to_canonical_mode_atexit);
	switch_to_raw_mode();

	char inp;
	while(1)
	{
		clear_screen();
		handle_keypress();
	}

	switch_to_canonical_mode(original_mode);
	return 0;
}