/*
	ECE357 Operating Systems
	Dolen Le
	PS 2 Filesystem Exploration
	Prof. Hakner
*/

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

char *linkBuf = NULL;

char timeString[23];
char *user = NULL;
int userid = -1;
long mtime = 0;
long now;
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
			if(sscanf(optarg, "%li", &mtime) == 0) {
				fprintf(stderr, "Invalid modify time %s\n", optarg);
			}
			break;
		}
		case '?':
			fprintf(stderr, "Unrecognized option %c\n", optopt);
			fprintf(stderr, "Usage: %s [-u username | uid] [-m modify_time] starting_path\n", argv[0]);
			exit(-1);
		}
						
	}
	now = time(NULL);
	//printf("user=%s\n",user);
	if(argc - optind == 1) {
		recursiveWalk(argv[optind]); //begin at start path
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
		printf("Path: %s\n", path);
		exit(-1);
	}

	while(entry = readdir(dir)) { //evaluate each entry in the directory
		if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
			char* pathname = malloc(strlen(entry->d_name) + strlen(path) + 2);
			sprintf(pathname, "%s/%s", path, entry->d_name);

			if(!lstat(pathname, info)) {
				if((userid == -1 || info->st_uid == userid) && (mtime == 0 ||
					 (mtime > 0 && now-info->st_mtime > mtime) ||
					 (mtime < 0 && info->st_mtime-now > mtime))) {
					
					printf("0x%04X/", info->st_dev); //device #
					printf("%li\t", (long) info->st_ino); //inode #

					char printSize = 1; //flag for size printing
					switch(info->st_mode & S_IFMT) { //file type bits
						case S_IFBLK: //block special file
							printf("b");
							printSize = 0;
							break;
						case S_IFCHR: //char special file
							printf("c");
							printSize = 0;
							break;
						case S_IFIFO: //FIFO
							printf("p");
							break;
						case S_IFREG: //regular file
							printf("-");
							break;
						case S_IFDIR: //directory
							printf("d");
							break;
						case S_IFLNK: //symlink
							printf("l");
							linkBuf = malloc(info->st_size+1);
							if(readlink(pathname, linkBuf, info->st_size) == info->st_size) {
								linkBuf[info->st_size] = 0;
							} else {
								perror("Could not read symlink");
								exit(-1);
							}
							break;
						case S_IFSOCK: //socket
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
					strftime(timeString, sizeof(timeString), "%D %r", localtime(&(info->st_mtime)));
					printf("%s\t", timeString);

					printf("%s", pathname);
					if(linkBuf) {
						printf(" -> %s\n", linkBuf);
					} else {
						printf("\n");
					}
				}

				free(linkBuf);
				linkBuf = NULL;

				if(S_ISDIR(info->st_mode)) {
					recursiveWalk(pathname);
				}

			} else {
				perror("Could not get inode info");
				exit(-1);
			}
			free(pathname);
		}
	}

	free(info);
	if(closedir(dir)) {
		perror("Could not close directory");
		exit(-1);
	}
}