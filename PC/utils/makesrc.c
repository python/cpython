#include <stdio.h>
#include <direct.h>
#include <string.h>

/* Copy files from source directories to ./src changing
file names and #include names to 8x3 lower case */

char *usage = "You must be in the \"pc\" directory.\n";
char *list[] = {"..\\Include", "..\\Modules", "..\\Objects", "..\\Parser", "..\\Python", ".", 0};

int
main(int argc, char ** argv)
{
  DIR *dpath;
  struct dirent *dir;
  int len;
  char **plist;
  char *pt1, *pt2, *name;
  char dest_path[256], src_path[256], buf256[256];
  FILE *fpin, *fpout;

  for (plist = list; *plist; plist++){
    if ((dpath = opendir(*plist)) == NULL){
      printf(usage);
      return 1;
      }

    while (dir = readdir(dpath)){
      name = dir->d_name;
      len = strlen(name);
      if (len > 2 && name[len - 2] == '.' &&
            (name[len - 1] == 'c' || name[len - 1] == 'h')){
        strcpy(buf256, name);
        if (len > 10){
          buf256[8] = '.';
          buf256[9] = name[len - 1];
          buf256[10] = 0;
          }
        strlwr(buf256);
        strncpy(src_path, *plist, 256);
        strncat(src_path, "\\", 256);
        strncat(src_path, name, 256);
        strncpy(dest_path, ".\\src\\", 256);
        strncat(dest_path, buf256, 256);
        printf("Copying %-30s to %s\n", src_path, dest_path);
        fpin = fopen(src_path, "r");
        fpout = fopen(dest_path, "w");
        while (fgets(buf256, 256, fpin)){
          if (!strncmp(buf256, "#include", 8)){
            strlwr(buf256);
            if ((pt1 = strstr(buf256, "\"")) &&
                (pt2 = strstr(buf256, ".")) &&
                (*(pt2 + 1) == 'h') &&
                (pt2 - pt1 > 9)){
              for (pt1 += 9; *pt2; pt1++, pt2++)
                *pt1 = *pt2;
              *pt1 = 0;
              }
            }
          fputs(buf256, fpout);
          }
        fclose(fpin);
        fclose(fpout);
        }
      }
    closedir(dpath);
    }
  return 0;
}
