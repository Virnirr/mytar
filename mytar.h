#define CFLAGPOS 0
#define TFLAGPOS 1
#define XFLAGPOS 2
#define VFLAGPOS 3
#define SFLAGPOS 4
#define FFLAGPOS 5

#define REGFILE	'0'	/* Regular file (preferred code).  */
#define AREGFILE	'\0'	/* Regular file (alternate code).  */
#define SYMFILE	'2'	/* Symbolic link (hard if not supported).  */
#define DIRFILE	'5'	/* Directory.  */

/* Contents of magic field and its length.  */
#define MAGIC	"ustar"
#define MAGICLEN 6

/* Contents of the version field and its length.  */
#define TVERSION "00"
#define TVERSIONLEN 2

#define BLOCKSIZE 512
#define MAXPATH 256

#define NAME_OFFSET 0
#define MODE_OFFSET 100
#define UID_OFFSET 108
#define GID_OFFSET 116
#define SIZE_OFFSET 124
#define MTIME_OFFSET 136
#define CHKSUM_OFFSET 148
#define TYPEFLAG_OFFSET 156
#define LINKNAME_OFFSET 157
#define MAGIC_OFFSET 257
#define VERSION_OFFSET 263
#define UNAME_OFFSET 265
#define GNAME_OFFSET 297
#define DEVMAJOR_OFFSET 329
#define DEVMINOR_OFFSET 337
#define PREFIX_OFFSET 345

#define NAME_SIZE 100
#define MODE_SIZE 8
#define UID_SIZE 8
#define GID_SIZE 8
#define SIZE_SIZE 12
#define MTIME_SIZE 12
#define CHKSUM_SIZE 8
#define TYPEFLAG_SIZE 1
#define LINKNAME_SIZE 100
#define MAGIC_SIZE 6
#define VERSION_SIZE 2
#define UNAME_SIZE 32
#define GNAME_SIZE 32
#define DEVMAJOR_SIZE 8
#define DEVMINOR_SIZE 8
#define PREFIX_SIZE 155

typedef struct Ustar_Header {
    char name[NAME_SIZE]; /* NUL-terminated if NUL fits */
    char mode[MODE_SIZE];
    char uid[UID_SIZE];
    char gid[UID_SIZE];
    char size[SIZE_SIZE];
    char mtime[MTIME_SIZE];
    char chksum[CHKSUM_SIZE];
    char typeflag[TYPEFLAG_SIZE];
    char linkname[LINKNAME_SIZE]; /* NUL-terminated if NUL fits */
    char magic[MAGIC_SIZE]; /* must be "ustar", NUL-terminated */
    char version[VERSION_SIZE]; /* must be "00" */
    char uname[UNAME_SIZE]; /* NUL-terminated */
    char gname[GNAME_SIZE]; /* NUL-terminated */
    char devmajor[DEVMAJOR_SIZE];
    char devminor[DEVMINOR_SIZE];
    char prefix[PREFIX_SIZE]; /* NUL-terminated if NUL fits */
} Ustar_Header;
/* this */
void tar_create(int tar_fd, int *flags, char *path_names[], int total_path);
void tar_list(int tar_fd, int *flags, char *path_names[], int total_path);
void tar_create(int tar_fd, int *flags, char *path_names[], int total_path);
void usageError();
void flagError();
void parse_flags(char *flagArg, int *flag_store);
int check_strict(char *magic, char *version);