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

void archivo (char *name)
{
  initscr();
  raw ();
  noecho ();
  int fdl = open(name, O_RDONLY);
  int fde = open("copiaEditable", O_RDWR|O_CREAT, 0600);
  //int fd = open(name,O_RDWR);
  if (fdl==-1){
    perror ("Error abriendo el archivo");
    exit (EXIT_FAILURE);
  }
  if (fde==-1){
    perror ("Error abriendo el archivo nuevo");
    exit (EXIT_FAILURE);
  }
  /* Mapea el archivo */
  struct stat st;
  //fstat (fd,&st);
  fstat (fdl,&st);
  int fs = st.st_size;
  lseek(fde,fs+512,SEEK_SET);
  write(fde,&st,1);
  fsync(fde);
  char *mapo = mmap(0 ,  fs , PROT_READ, MAP_SHARED,  fdl ,  0 );
  char *map = mmap(0, fs+512, PROT_READ | PROT_WRITE, MAP_SHARED, fde, 0);
  memcpy(map, mapo, fs);
  //char *map = mmap(0, fs, PROT_READ | PROT_WRITE  , MAP_SHARED, fd, 0);
  if (map == MAP_FAILED){
    close (fde);
    perror("Error mapeando el archivo");
    exit (EXIT_FAILURE);
  }
  if (mapo == MAP_FAILED){
    close (fdl);
    perror("Error mapeando el archivo para editar");
    exit (EXIT_FAILURE);
  }

  int ch,a;
  int c=0,r=0, t=0;
  int x=0,y=0;
  do{
    for (int i=0;i<25;i++){
	    char *l = hazLinea(map,i*16);
	    //mvprintw(i,0,l);
	    move (i,0);
	    addstr(l);
	  }
    move (c,9+r);
    refresh();
    ch =leeChar();
    switch(ch){
      //case 'D': //Flechas de izquierda
      case 0x1B5B44: //Flechas de izquierda
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
      //case 'C': //Flecha de derecha
      case 0x1B5B43:
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
      //case 'B': //Flecha de abajo
      case 0x1B5B42:
        c = (c<24) ? c+1 : 0;
        break;
      //case 'A': //Flecha de arriba
      case 0x1B5B41:
	      c = (c>0) ? c-1 : 24;
	    break;
      default://Quiere cambiar el texto
        if(r == 0 && c == 0){
          map[r] = ch;
        }else{
          if(r <48 && r!=0){
            t = r/3;
            map[t+(16*c)]=ch;
          }else{
            if(r == 0 && c!= 0){
              map[r+(16*c)] = ch;
            }
          }
        }


      break;
	  }
    y = r;
    x = (c<16) ? c*3+9 : 41+c;
  }while (ch!=24);
  /*if(munmap(map,fs)==-1){
    perror("Error un-unmapping the file");
  }*/
  struct stat st2;
  fstat (fde,&st2);
  int fs2 = st.st_size;
  if (munmap(map, fs2) == -1)
  {
    perror("Error un-mmapping the file");
  }
  if (munmap(mapo, fs) == -1)
  {
    perror("Error un-mmapping the file");
  }
  close(fde);
  close(fdl);
  //close(fd);
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
      //insertar();
    }while (c!=24);
  endwin();
  return 0;
}
