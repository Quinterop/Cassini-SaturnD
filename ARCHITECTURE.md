Partie client :
Il y a un fichier cassini.c qui utilise des fonctions de common.c et de timing-text-io.c.
Le fichier cassini.h contient les signatures des fonctions de son .c et un include vers common.h qui contient tous les includes des librairies, les fonctions communes et les structures communes.

Partie Demon : 
Il y a un fichier saturnd.c qui utilise des fonctions de common.c et de timing-text-io.c.
Le fichier saturnd.h contient les signatures des fonctions de son .c et un include vers common.h qui contient tous les includes des librairies, les fonctions communes et les structures communes.

On a défini des structures string et commandline dans common.h, task et run dans saturnd.h
