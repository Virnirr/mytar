#define CFLAGPOS 0
#define TFLAGPOS 1
#define XFLAGPOS 2
#define VFLAGPOS 3
#define SFLAGPOS 4
#define FFLAGPOS 5

void usageError();
void flagError();
void parse_flags(char *flagArg, int *flag_store);