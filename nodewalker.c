#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//for getpwnam, getpwuid and getgrgid
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <dirent.h> //for directory manipulation
#include <sys/stat.h> //for stat
#include <time.h> //for ctime

extern char* optarg;
extern int optind, opterr, optopt;

const char *permText[] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};

char *user = NULL;
int userid = -1;
unsigned int mtime = 0;
DIR *startDir = NULL;

int main(int argc, char *argv[]) {
	int c;
	while((c = getopt(argc, argv, "u:m:")) != -1) { //get options
		switch(c) {
		case 'u': {
			int uid;
			if(sscanf(optarg, "%d", &uid) > 0) {
				struct passwd *p = getpwuid(uid);
				if(p) {
					user = p->pw_name;
					userid = p->pw_uid;
				} else {
					fprintf(stderr, "Invalid UID %d\n", uid);
					exit(-1);
				}
			} else {
				struct passwd *p = getpwnam(optarg);
				if(p) {
					user = p->pw_name;
					userid = p->pw_uid;
				} else {
					fprintf(stderr, "Invalid User %s\n", optarg);
					exit(-1);
				}
			}
			
			break;
		}
		case 'm': {
			if(sscanf(optarg, "%u", &mtime) == 0) {
				fprintf(stderr, "Invalid modify time %s\n", optarg);
			} 
		}
		case '?':
			fprintf(stderr, "Unrecognized option %c\n", optopt);
			fprintf(stderr, "Usage: %s [-u username | uid] [-m modify_time] starting_path\n", argv[0]);
			exit(-1);
		}
						
	}
	//printf("user=%s\n",user);
	if(argc - optind == 1) {
		recursiveWalk(argv[optind]); //begin a start path
	} else {
		fprintf(stderr, "Inccorect number of arguments\n");
		fprintf(stderr, "Usage: %s [-u username | uid] [-m modify_time] starting_path\n", argv[0]);
		exit(-1);
	}

	
}

int recursiveWalk(char *path) {
	struct dirent *entry;
	struct stat *info = malloc(sizeof(struct stat));

	DIR *dir = opendir(path);
	
	if(!dir) {
		perror("Could not open directory");
		exit(-1);
	}

	while(entry = readdir(dir)) {
		//printf("%s ",entry->d_name);
		if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
			if(!stat(entry->d_name, info)) {
				if(userid == -1 || info->st_uid == userid) {
					
					printf("0x%04X/", info->st_dev); //device #
					printf("%li\t", (long) info->st_ino); //inode #

					char printSize = 1; //flag for size printing
					switch(info->st_mode & S_IFMT) { //file type bits
						case S_IFBLK:
							printf("b");
							printSize = 0;
							break;
						case S_IFCHR:
							printf("c");
							printSize = 0;
							break;
						case S_IFIFO:
							printf("p");
							break;
						case S_IFREG:
							printf("-");
							break;
						case S_IFDIR:
							printf("d");
							break;
						case S_IFLNK:
							printf("l");
							break;
						case S_IFSOCK:
							printf("s");
							break;
						default:
							fprintf(stderr, "Invalid file type.\n");
							exit(-1);
					}

					//print permission bits
					unsigned char modeBits = (info->st_mode >> 9) & 0x07;
					int i;
					for(i=2; i>=0; i--) {
						unsigned char permBits = (info->st_mode >> 3*i) & 0x07;
						char* temp = strdup(permText[permBits]);
						if((modeBits >> i) & 0x01) {
							if(permBits & 0x01) {
								temp[2] = 's';
							} else {
								temp[2] = 'S';
							}
							if(i == 0) //for group permissions sticky bit
								temp[2]++;
						}
						printf("%s", temp);
						free(temp);
					}

					printf("\t%d\t", info->st_nlink); //number of hard links

					struct passwd *p = getpwuid(info->st_uid); //owner
					if(p) {
						printf("%s\t", p->pw_name);
					} else {
						printf("%d\t", info->st_uid);
					}

					struct group *g = getgrgid(info->st_gid); //group
					if(p) {
						printf("%s\t", g->gr_name);
					} else {
						printf("%d\t", info->st_gid);
					}

					if(printSize) {
						printf("%li\t", (long) info->st_size); //print size in bytes
					} else {
						printf("0x%lX\t", (long) info->st_rdev); //print device ID
					}

					//print modify time
					char timeString[23];
					strftime(timeString, sizeof(timeString), "%D %r", localtime(&(info->st_mtime)));
					printf("%s\t", timeString);
				}

			} else {
				perror("stat failure");
				exit(-1);
			}
			printf("\t%s ",entry->d_name);
			printf("\n");
		}
	}
}