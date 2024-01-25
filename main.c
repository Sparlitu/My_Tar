#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NAME_SIZE 100
#define BUFFER_SIZE 8192
#define HEADER_SIZE 512

char *my_malloc(long int size)
{
  char *buffer = NULL;

  if((buffer = malloc((size + 1) * sizeof(char))) == NULL)
    {
      return NULL;
    }

  return buffer;
}

void arhivare(int argc, char **argv)
{
  //forma ./my_tar a nume_arhiva.tar nume_fisier_1 nume_fisier_2 ... nume_fisier_n
  FILE *fout = NULL;
  char *filename = argv[2];
  
  if((fout = fopen(filename, "wb")) == NULL)
    {
      perror(NULL);
      exit(-1);
    }

  char zero[BUFFER_SIZE] = "";
  
  for(int i = 3; i < argc; i++)
    {
      //header + file
      FILE *fin = NULL;

      if((fin = fopen(argv[i], "rb")) == NULL)
	{
	  perror(NULL);
	  exit(-1);
	}

      //header
      long int header_begin_offset = ftell(fout);
      
      //file name
      fwrite(argv[i], sizeof(char), strlen(argv[i]), fout);
      strcpy(zero, "");
      fwrite(zero, sizeof(char), 100 - strlen(argv[i]), fout);

      //file mode, owner id, group id ??
      char file_mode[8] = "0000644";
      fwrite(file_mode, sizeof(char), 8, fout);
      char owner_id[8] = "0001750";
      fwrite(owner_id, sizeof(char), 8, fout);
      char group_id[8] = "0001750";
      fwrite(group_id, sizeof(char), 8, fout);

      //file size
      fseek(fin, 0, SEEK_END);
      long int file_size = ftell(fin);
      char file_size_octal[12];
      sprintf(file_size_octal, "%011lo", file_size);
      fwrite(file_size_octal, sizeof(char), 12, fout);
      fseek(fin, 0, SEEK_SET);

      //last modification ?? nu stiu cum sa il fac
      char last_modification[12];
      sprintf(last_modification, "%011o", 0);
      fwrite(last_modification, sizeof(char), 12, fout);

      //completez momentan checksum cu ' '
      long int chksum_offset = ftell(fout);
      char chksum[8] ="      ";
      fwrite(chksum, sizeof(char), 7, fout);
      char blank = ' ';
      fwrite(&blank, sizeof(char), 1, fout);

      //type flag = 0, normal file
      char flag = '0';
      fwrite(&flag, sizeof(char), 1, fout);

      //name of linked file
      fwrite(zero, sizeof(char), 100, fout);

      //ustar indicator "ustar" then NUL
      char ustar[8] = "ustar  ";
      fwrite(ustar, sizeof(char), 8, fout);
      
      //ustar version "00" sau 2 blank-uri; pe versiunea mea apare ustar iar apoi doua blank-uri si then NUL
      //fwrite(&blank, sizeof(char), 2, fout);

      //owner user name, owner group name
      char owner_user_name[32] = "sparlitu";
      fwrite(owner_user_name, sizeof(char), 32, fout);
      
      char owner_group_name[32] = "sparlitu";
      fwrite(owner_group_name, sizeof(char), 32, fout);
      
      //restul campurilor ?? suntem la deplasamentul 329
      fwrite(zero, sizeof(char), 183, fout);
      long int header_end_offset = ftell(fout);

      //calculez checksum
      /*
      fseek(fout, header_begin_offset, SEEK_SET);
      unsigned int my_checksum = 0;
      printf("chksum_offset = %ld\n", chksum_offset);
      for(int i = 0; i < 512; i++)
	{
	  char c = 0;
	  if(i >= 148 && i < 156)
	    {
	      c = ' ';
	    }
	  else
	    {
	      if(fread(&c, sizeof(char), 1, fout) != 1)
		{
		  perror(NULL);
		  exit(-1);
		}
	    }
	  my_checksum += c;
	}
      printf("my_checksum = %d\n", my_checksum);
      fseek(fout, chksum_offset, SEEK_SET);
      sprintf(chksum, "%06o", my_checksum);
      printf("chksum = %s\n", chksum);
      fwrite(chksum, sizeof(char), 6, fout);
      fseek(fout, header_end_offset, SEEK_SET);
      printf("header_begin_offset = %ld\n", header_begin_offset);
      printf("header_end_offset = %ld\n", header_end_offset);
      */
      
      //file
      char *buffer = my_malloc(file_size);
      
      fread(buffer, sizeof(char), file_size, fin);
      buffer[file_size] = 0;
      fwrite(buffer, sizeof(char), file_size, fout);
      
      free(buffer);

      //printf("file_size = %ld\n", file_size);
      fwrite(zero, sizeof(char), 512 - file_size % 512, fout);
      
      if(fclose(fin) != 0)
	{
	  perror(NULL);
	  exit(-1);
	}
    }

  //arhiva se termina cu doua sectoare nule

  fwrite(zero, sizeof(char), 1024, fout);
  
  if(fclose(fout) != 0)
    {
      perror(NULL);
      exit(-1);
    }
  
}

void dezarhivare(int argc, char **argv)
{
  //forma ./my_tar x nume_arhiva.tar
  FILE *fin = NULL;
  char *filename = argv[2];

  if((fin = fopen(filename, "rb")) == NULL)
    {
      perror(NULL);
      exit(-1);
    }

  while(!feof(fin))
    {
      //calculez my_checksum
      long int offset = ftell(fin);
      unsigned int my_checksum = 0;
      
      for(int i = 0; i < 512; i++)
	{
	  char c = 0;
	  if(i >= 148 && i < 156)
	    {
	      c = ' ';
	    }
	  else
	    {
	      if(fread(&c, sizeof(char), 1, fin) != 1)
		{
		  perror(NULL);
		  exit(-1);
		}
	    }
	  my_checksum += c;
	}
      fseek(fin, offset, SEEK_SET);
      
      //la deplasament 0 se afla numele fisierului
      char nume_fisier[NAME_SIZE + 1];
      fread(nume_fisier, sizeof(char), NAME_SIZE, fin);

      //daca am ajuns la cele doua block-uri de 512 bytes de 0 putem iesi
      if(strcmp(nume_fisier, "") == 0)
	{
	  break;
	}
      
      //la deplasament 124 se afla dimensiunea fisierului in bytes in octal
      fseek(fin, 24, SEEK_CUR);
      char file_size_octal[12];
      fread(file_size_octal, sizeof(char), 12, fin);
      long int file_size = strtol(file_size_octal, NULL, 8);

      //la deplasament 148 se afla checksum
      fseek(fin, 12, SEEK_CUR);
      char chksum[8];
      fread(chksum, sizeof(char), 8, fin);
      long int file_checksum = strtol(chksum, NULL, 8);
      printf("my_checksum = %d\n", my_checksum);
      printf("file_checksum = %ld\n", file_checksum);
      
      //trec de header
      fseek(fin, 356, SEEK_CUR);
      
      FILE *fout = NULL;

      if((fout = fopen(nume_fisier, "wb")) == NULL)
	{
	  perror(NULL);
	  exit(-1);
	}

      //nu stim cat este file_size, are o dimensiune variabila
      char *buffer = my_malloc(file_size);

      fread(buffer, sizeof(char), file_size, fin);
      buffer[file_size] = 0;
      fwrite(buffer, sizeof(char), file_size, fout);

      free(buffer);

      //ma pot deplasa in fisier ca sa scap de padding-uri
      fseek(fin, 512 - file_size % 512, SEEK_CUR);

      if(fclose(fout) != 0)
	{
	  perror(NULL);
	  exit(-1);
	}
      
    }
  
  if(fclose(fin) != 0)
    {
      perror(NULL);
      exit(-1);
    }
  
}

int main(int argc, char **argv)
{
  //pentru procesul de arhivare, fișierele din directorul curent ce urmează a fi arhivate se vor da prin numele lor ca și argument în linie de comandă după un alt argument în linie de comandă ce va specifica numele arhivei tar
  //pentru procesul de dezarvhivare, fișierele vor fi create cu numele găsit în arhivă iar programul vostru va prelua numele arhivei ca și argument în linie de comandă.
  //daca programul primeste doar un argument atunci printeaza informatii despre usage

  if(argc == 1)
    {
      printf("Programul poate primi 2 comenzi:\n\n");
      printf("a - arhiveaza fisierele din folderul curent in arhiva data ca argument\n\n");
      printf("Forma generala: ./my_tar nume_arhiva.tar nume_fisier_1 nume_fisier_2 ... nume_fisier_n\n\n");
      printf("x - dezarhiveaza fisierele din arhiva data ca argument in folderul curent\n\n");
      printf("Forma generala: ./my_tar x nume_arhiva.tar\n\n");
      return 0;
    }

  if(strcmp(argv[1], "a") == 0) // arhivare
    {
      //forma ./my_tar a nume_arhiva.tar nume_fisier_1 nume_fisier_2 ... nume_fisier_n

      if(argc > 20)
	{
	  printf("Prea multe argumente\n");
	  exit(-1);
	}
      
      arhivare(argc, argv);
    }
  else if(strcmp(argv[1], "x") == 0) // dezarhivare
    {
      //forma ./my_tar x nume_arhiva.tar

      if(argc != 3)
	{
	  printf("Numar invalid de argumente\n");
	  exit(-1);
	}
      
      dezarhivare(argc, argv);
    }
  else
    {
      printf("Argument invalid\n");
      exit(-1);
    }
  
  return 0;
}
