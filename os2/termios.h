/* dummy for OS/2 */

struct termios
{
  int c_cc[4];
  int c_lflag;
};

#define ICANON  1
#define ECHO    2

#define VERASE  0
#define VKILL   1
#define VMIN    2
#define VTIME   3

#define TCSADRAIN   4711

int tcgetattr(int, struct termios *);
int tcsetattr(int, int, struct termios *);

