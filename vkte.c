#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#define CTRL_Q 17
#define UP 1000
#define DOWN 1001
#define HOME 1002
#define END 1003

//to store terminal configurations
struct terminal_editor_configuration
{
	int rows;
	int columns;
	struct termios original;
	int cursor_x_pos;
	int cursor_y_pos;
};

struct terminal_editor_configuration terminal;

void kill_process(char*);
void draw_line_numbers();
int get_terminal_size(int*, int*);
void set_terminal_size();
int get_cursor_position(int*, int*);
int length_greater_than_columns(int, int);

int return_keypress()
//wait for character input
{
	int err=0;
	char inp;
	while((err=read(0, &inp, 1))!=1)
	{
		if(err==-1)
			kill_process("read");
	}

	if(inp=='\x1b')
	{
		char sequence[3];

		if(read(0, &sequence[0], 1)!=1)
			return '\x1b';
		if(read(0, &sequence[1], 1)!=1)
			return '\x1b';

		if(sequence[0]=='[')
		{
			if(sequence[1]>='0' && sequence[1]<='9')
			{
				if(read(0, &sequence[2], 1)!=1)
				{
					return '\x1b';
				}
				if(sequence[2]=='~')
				{
					if(sequence[1]=='1' || sequence[1]=='7')
					{
						return HOME;
					}
					else if(sequence[1]=='4' || sequence[1]=='8')
					{
						return END;
					}
					else if(sequence[1]=='5')
					{
						return UP;
					}
					else if(sequence[1]=='6')
					{
						return DOWN;
					}
				}
			}
			else if(sequence[1]=='A')
				return 'w';
			else if(sequence[1]=='B')
				return 's';
			else if(sequence[1]=='C')
				return 'd';
			else if(sequence[1]=='D')
				return 'a';
			else if(sequence[1]=='H')
				return HOME;
			else if(sequence[1]=='F')
				return END;
		}
		else if(sequence[0]=='O')
		{
			if(sequence[1]=='H')
			{
				return HOME;
			}
			else if(sequence[1]=='F')
			{
				return END;
			}
		}
		return '\x1b';
	}
	else
	{
		return inp;
	}
}

// void append_to_buffer(struct append_buffer *ap, char *ch, int len)
// {
// 	char* new_buffer=realloc(ap->buffer, ap->length+len);

// 	if(new_buffer==NULL)
// 	{
// 		return;
// 	}

// 	memcpy(&new_buffer[ap->length], ch, len);
// 	ap->buffer=new_buffer;
// 	ap->length+=len;
// }

// void free_buffer(struct append_buffer *ap)
// {
// 	free(ap->buffer);
// }

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

	terminal.cursor_x_pos=0;
	terminal.cursor_y_pos=0;

	if(get_terminal_size(&terminal.rows, &terminal.columns)==-1)
		kill_process("get_terminal_size");
}

void draw_line_numbers()
{
	int line_number=1;

	for(line_number=1;line_number<=terminal.rows;line_number++)
	{

		if(line_number==(terminal.rows/3))
		{
			char string[]="VKTE: A simple text editor";
			int string_len=strlen(string);

			if(length_greater_than_columns(string_len, terminal.columns)==1)
			{
				string_len=terminal.columns;	
			}

			int get_to_centre=(terminal.columns-string_len)/2;

			if(get_to_centre!=0)
			{
				write(1, "~", 1);
				get_to_centre--;
			}

			for(int i=0;i<get_to_centre;i++)
			{
				write(1, " ", 1);
			}

			write(1, string, string_len);
		}
		else
		{
			write(1, "~", 1);
		}

		// char buffer[4];
		// sprintf(buffer, "%d\r\n", line_number);
		//write(1, "\x1b[0K", 3);

		if(line_number<=terminal.rows-1)
		{
			write(1, "\r\n", 2);
		}
	}
}

int length_greater_than_columns(int s, int c)
{
	if(s>c)
		return 1;
	else
		return 0;
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
	int inp=return_keypress();

	switch(inp)
	{
		case CTRL_Q:	
		{
			write(1, "\x1b[2J", 4);
			write(1, "\x1b[1;1H", 3);
			exit(0);
		}

		case UP:
		{
			for(int i=0;i<terminal.rows;i++)
			{
				if(terminal.cursor_y_pos!=0)
					terminal.cursor_y_pos--;
			}
			break;
		}

		case DOWN:
		{
			for(int i=0;i<terminal.rows;i++)
			{
				if(terminal.cursor_y_pos!=terminal.rows-1)
					terminal.cursor_y_pos++;
			}
			break;
		}

		case HOME:
		{
			terminal.cursor_x_pos=0;
			break;
		}

		case END:
		{
			terminal.cursor_x_pos=terminal.columns-1;
			break;
		}

		case 'a':
		{
			if(terminal.cursor_x_pos!=0)
				terminal.cursor_x_pos--;
			break;
		}

		case 'd':
		{
			if(terminal.cursor_x_pos!=terminal.columns-1)
				terminal.cursor_x_pos++;
			break;
		}

		case 's':
		{
			if(terminal.cursor_y_pos!=terminal.rows-1)
				terminal.cursor_y_pos++;
			break;
		}

		case 'w':
		{
			if(terminal.cursor_y_pos!=0)
				terminal.cursor_y_pos--;
			break;
		}
	}
}

void refresh_terminal_screen()
{
	//struct append_buffer ap={NULL, 0};
	//write(1, "\x1b[?25l", 6);
	write(1, "\x1b[2J", 4);
	write(1, "\x1b[H", 3);
	draw_line_numbers();

	char buffer[32];
	sprintf(buffer, "\x1b[%d;%dH", terminal.cursor_y_pos+1, terminal.cursor_x_pos+1);
	write(1, buffer, strlen(buffer));

	//write(1, "\x1b[H", 3);
	//write(1, "\x1b[?25h", 6);
	//write(1, ap.buffer, ap.length);
	//free_buffer(&ap);
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
	// printf("\033[0;31m");
	// printf("%s", "...Hello");
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
		refresh_terminal_screen();
		handle_keypress();
	}

	switch_to_canonical_mode(original_mode);
	//printf("033[0m"); 
	return 0;
}