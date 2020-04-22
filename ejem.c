#include <curses.h>
#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

struct s_dir{
  int tipo;
  char *nombre;
}res[128];

int leer (char *cwd)
{
  DIR *dir = opendir (cwd);
  struct dirent *dp;
  int i = 0;
  while ((dp=readdir(dir))!=NULL)
    {
      res[i].tipo = dp->d_type;
      res[i].nombre = strdup(dp->d_name);
      i++;
    }
  closedir(dir);
  return i;
}
char *hazLinea (char *base, int dir)
{
  char linea[100]; // la linea es mas pequeña
  int o = 0;
  //Muestra 16 caracteres por cada linea
  o+= sprintf(linea,"%08x ",dir);//offset en hexadecimal
  for (int i=0; i<4; i++)
    {
      unsigned char a,b,c,d;
      a=base[dir+4*i+0];
      b=base[dir+4*i+1];
      c=base[dir+4*i+2];
      d= base[dir+4*i+3];
      o+=sprintf(&linea[o],"%02x %02x %02x %02x|",a,b,c,d);
    }
  for (int i=0; i<16;i++)
    {
      if (isprint(base[dir+i]))
	{
	  o += sprintf(&linea[o],"%c",base[dir+i]);
	}
      else
	{
	  o+=sprintf(&linea[o],".");
	}
    }
  sprintf(&linea[o],"\n");
  return(strdup(linea));
}

void archivo (char *name)
{
  initscr();
  raw ();
  noecho ();
  int fd = open(name,O_RDONLY);
  if (fd==-1)
    {
      perror ("Error abriendo el archivo");
      exit (EXIT_FAILURE);
    }

  /* Mapea el archivo */
  struct stat st;
  fstat (fd,&st);
  int fs = st.st_size;

  char *map = mmap(0, fs, PROT_READ, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED)
    {
      close (fd);
      perror("Error mapeando el archivo");
      exit (EXIT_FAILURE);
    }
  for (int i =0; i<25; i++)
    {
      //Haz linea, base y offset
      char *l = hazLinea(map,i*16);
      mvprintw(i,0,l);
    }
  refresh();

  int ch,a;
  int c=0,r=0;
  int x=0,y=0;
  move(c,9+r);

  do
    {
      ch = getch();
      switch(ch)
	{
	case 'A':
	  r = (r>0) ? r-1 : 24;
	  break;
	case 'B':
	  r = (r<24) ? r + 1 : 0;
	  break;
	case 'C':
	  c = (c<31) ? c+1 : 0;
	  break;
	case 'D':
	  c = (c>0) ? c-1 : 31;
	  break;
	}
      y = r;
      x = (c<16) ? c*3+9 : 41+c;
    }while (ch!=24);
  endwin ();
  clear ();
}

int main ()
{
  char *cwd;
  int sigue;
  int a;
  strcpy (cwd,".");
  a = leer(cwd);
  int i = 0;
  int c;
  initscr();
  raw();
  noecho(); //Nos muestra el caracter leido
  do
    {
      for (int j=0;j<a; j++)
	{
	  if (j==i)
	    {
	      attron (A_REVERSE);
	    }
	  mvprintw(5+j,5,res[j].nombre);
	  attroff(A_REVERSE);
	}//FOR
      move(5+i,5);
      refresh();
      c = getch();
      switch(c)
	{
	case 'A':
	  i = (i>0) ? i-1 : a-1;
	  break;
	case 'B':
	  i = (i<a-1) ? i + 1 : 0;
	  break;
	case 10:
	  if (strcmp(".",res[i].nombre)==0 || res[i].tipo == DT_REG)
	    {
	      //si es archivo salte
	      if (res[i].tipo == DT_REG)
		{
		  clear ();
		  archivo (res[i].nombre);
		}
	    }
	  else
	    {
	      if (strcmp("..",res[i].nombre)==0)
		{
		  // Cambia al papa
		  char *p = strrchr (cwd,'/');
		  *p =0;
		  a= leer(cwd);
		  i = 0;
		  clear ();
		}
	      else
		{
		  //cambia a otro dir
		  strncat (cwd,"/",256);
		  strncat (cwd,res[i].nombre,256);
		  a = leer (cwd);
		  i =0;
		  clear ();
		}
	    }
	  break;
	default:
	  //nothing
	  break;
	}
      move (1,1);
      printw("Estoy en %d: Lei %d",i,c);
    }while (c!=24);
  endwin();
  return 0;
}
