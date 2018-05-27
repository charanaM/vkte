#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#define CTRL_Q 17

//to store terminal configurations
struct terminal_editor_configuration
{
	int rows;
	int columns;
	struct termios original;
};

struct append_buffer
{
	char* buffer;
	int length;
};

struct terminal_editor_configuration terminal;

void kill_process(char*);
void draw_line_numbers();
int get_terminal_size(int*, int*);
void set_terminal_size();
int get_cursor_position(int*, int*);

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

void append_to_buffer(struct append_buffer *ap, char *ch, int len)
{
	char* new_buffer=realloc(ap->buffer, ap->length+len);

	if(new_buffer==NULL)
	{
		return;
	}

	memcpy(&new_buffer[ap->length], ch, len);
	ap->buffer=new_buffer;
	ap->length+=len;
}

void free_buffer(struct append_buffer *ap)
{
	free(ap->buffer);
}

void clear_screen()
//clear the screen by using escape sequence
{
	if(write(1, "\x1b[2J", 4)==-1)
		kill_process("write");
	if(write(1, "\x1b[1;1H", 3)==-1)
		kill_process("write");

	draw_line_numbers();

	if(write(1, "\x1b[1;1H", 3)==-1)
		kill_process("write");

}

int get_terminal_size(int *r, int *c)
{
	struct winsize size;

	if(ioctl(1, TIOCGWINSZ, &size)==-1||size.ws_col==0)
	{
		if(write(1, "\x1b[999C\x1b[999B", 12)!=12)
			return -1;
		return get_cursor_position(r, c);
	}
	else
	{
		*r=size.ws_row;
		*c=size.ws_col;
		return 0;
	}
}

void set_terminal_size()
{
	if(get_terminal_size(&terminal.rows, &terminal.columns)==-1)
		kill_process("get_terminal_size");
}

void draw_line_numbers(struct append_buffer *ap)
{
	int line_number=1;

	for(line_number=1;line_number<=terminal.rows;line_number++)
	{
		//char buffer[4];
		//sprintf(buffer, "%d\r\n", line_number);
		append_to_buffer(ap, "~", 1);

		if(line_number<=terminal.rows-1)
		{
			append_to_buffer(ap, "\r\n", 2);
		}
	}
}

int get_cursor_position(int *r, int *c)
{
	char buffer[32];
	unsigned int i=0;

	if(write(1, "\x1b[6n", 4)!=4)
		return -1;

	while(i<sizeof(buffer)-1) 
	{
    	if(read(1, &buffer[i], 1)!=1) 
    		break;
    	if(buffer[i]=='R')
    	    break;
    	i++;
  	}
  	
  	buffer[i] = '\0';
  	
  	if(buffer[0]!='\x1b' || buffer[1]!='[')
  		 return -1;
    if(sscanf(&buffer[2], "%d;%d", r, c)!=2)
    	 return -1;
  	return 0;
}

void handle_keypress()
//handle the character entered
{
	char inp=return_keypress();

	switch(inp)
	{
		case CTRL_Q:	
		{
			write(1, "\x1b[2J", 4);
			write(1, "\x1b[1;1H", 3);
			exit(0);
		}
	}
}

void refresh_terminal_screen()
{
	struct append_buffer ap={NULL, 0};
	append_to_buffer(&ap, "\x1b[2J", 4);
	append_to_buffer(&ap, "\x1b[H", 3);
	draw_line_numbers(&ap);
	append_to_buffer(&ap, "\x1b[H", 3);
	write(1, ap.buffer, ap.length);
	free_buffer(&ap);
}

void kill_process(char* msg)
//for debugging error
{
	write(1, "\x1b[2J", 4);
	write(1, "\x1b[1;1H", 3);

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
	if(tcsetattr(0, TCSAFLUSH, &terminal.original)==-1)
		kill_process("tcsetattr");
}

int main(int argc, char const *argv[])
//main
{
	struct termios original_mode;
	if(tcgetattr(0, &original_mode)==-1)
		kill_process("tcgetattr");
	if(tcgetattr(0, &terminal.original)==-1)
		kill_process("tcgetattr");;
	atexit(switch_to_canonical_mode_atexit);
	switch_to_raw_mode();
	set_terminal_size();

	char inp;
	while(1)
	{
		clear_screen();
		handle_keypress();
	}

	switch_to_canonical_mode(original_mode);
	return 0;
}