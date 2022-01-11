Partie client :
Il y a un seul fichier cassini.c qui utilise des fonctions de common.c et timing-text-io.c
Les fichier cassini.h et timing*.h contiennent les fonctions et un include vers common.h qui contient tous les include de librairies.

Partie Demon : 
Un autre couple saturnd.c/.h qui redirige vers common.h. common.c contient utile des fonctions utiles aux deux parties. 