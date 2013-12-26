/* dummy for OS/2 */

struct passwd
{
  char *pw_name;
  char *pw_dir;
  char *pw_gecos;
};

#define gcos_name(x, y)  x

struct passwd *getpwnam(char *);
struct passwd *getpwuid(int);
