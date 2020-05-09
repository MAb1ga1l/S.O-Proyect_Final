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

int leeChar() {
  int chars[5];
  int ch,i=0;
  nodelay(stdscr, TRUE);
  while((ch = getch()) == ERR); /* Espera activa */
  ungetch(ch);
  while((ch = getch()) != ERR) {
    chars[i++]=ch;
  }
  /* convierte a numero con todo lo leido */
  int res=0;
  for(int j=0;j<i;j++) {
    res <<=8;
    res |= chars[j];
  }
  return res;
}

/*int main(void) {
  int fd = open("test_file", O_RDWR | O_CREAT, (mode_t)0600);
  const char *text = "hello";
  size_t textsize = strlen(text) + 1;
  lseek(fd, textsize-1, SEEK_SET);
  write(fd, "", 1);
  char *map = mmap(0, textsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  memcpy(map, text, strlen(text));
  msync(map, textsize, MS_SYNC);
  munmap(map, textsize);
  close(fd);
  return 0;
}*/

void archivo (char *name)
{
  initscr();
  raw ();
  noecho ();
  int fd = open(name,O_RDWR);
  if (fd==-1){
    perror ("Error abriendo el archivo");
    exit (EXIT_FAILURE);
  }
  /* Mapea el archivo */
  struct stat st;
  fstat (fd,&st);
  int fs = st.st_size;

  char *map = mmap(0, fs, PROT_READ | PROT_WRITE  , MAP_SHARED, fd, 0);
  if (map == MAP_FAILED){
    close (fd);
    perror("Error mapeando el archivo");
    exit (EXIT_FAILURE);
  }

  int ch,a;
  int c=0,r=0;
  int x=0,y=0;
  do{
    for (int i=0;i<25;i++){
	    char *l = hazLinea(map,i*16);
	    mvprintw(i,0,l);
	  }
    move (c,9+r);
    refresh();
    ch = getch();
    switch(ch){
      case 'D': //Flechas de izquierda
        //r = (r>0) ? r-3 : 45; 
        if (r>0 && r<48) {
          r= r-3;
        }else{
          if (r>48 && r<63){
            r = r-1;
          }else{
            r = 63;
          }
        }
        break;
      case 'C': //Flecha de derecha
        //r = (r<45) ? r + 3 : 0;
        if (r<63) {
          if (r<48){
            r = r+3;
          }else{
            r= r+1;
          }
        }else{
          r = 0;
        }
        break;
      case 'B': //Flecha de abajo
        c = (c<24) ? c+1 : 0; 
        break;
      case 'A': //Flecha de arriba
	      c = (c>0) ? c-1 : 24;
	    break;
	  }
    y = r;
    x = (c<16) ? c*3+9 : 41+c;
  }while (ch!=24);
  if(munmap(map,fs)==-1){
    perror("Error un-unmapping the file");
  }
  /*leeChar();
  memcpy(map, text, strlen(text));
  msync(map, textsize, MS_SYNC);
  munmap(map, textsize);*/
  close(fd);
  endwin ();
  clear ();
}

void insetrar(){
  int fdl = open(FILEPATH, O_RDONLY);
  int fde = open(FILEPATH, O_RDWR);

  /* Mapea el archivo de entrada y salida*/
  char *mapo = mmap(0, fs, PROT_REA, MAP_SHARED, fd, 0);
  char *map = mmap(0, fs + SLACK, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  memcpy(mapo, map, fs);

  //Pedir cambio y hacerlo
  
  if (munmap(map, fs_REAL) == -1)
  {
    perror("Error un-mmapping the file");
  }
  if (munmap(mapo, fs) == -1)
  {
    perror("Error un-mmapping the file");
  }

  /* Cierra el archivo
*/
  close(fde);
  close(fdl);
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
      insertar();
    }while (c!=24);
  endwin();
  return 0;
}
