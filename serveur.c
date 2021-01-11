#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define PORT 6000
#define MAX_BUFFER 1000
#define MAX_CLIENTS 3
#define MAX_RESERV 100
#define MAX_NUMDOSS 99999999
#define MIN_NUMDOSS 10000000

const char* HELPREP = "Pour afficher la liste des places taper 'list'\nPour reserver une place taper 'reserv'\nPour annuler une reservation  taper 'annul'\nPour quitter taper 'exit'";
const char* CLIENT = "Bienvenue,\nPour afficher l'aide taper 'help'";
const char* HELP = "help";
const char* LIST = "list";
const char* RESERV = "reserv";
const char* ANNUL = "annul";
const char* EXIT = "exit";
const char* ERROR = "Aucune correspondance taper 'help' pour obtenir l'aide";
const char* DPLACE = "Quel place souhaiter-vous ?";
const char* NPLACE = "Place non reconnu,\nRetour à l'accueil";
const char* INDISPLACE = "Place indisponible,\nRetour à l'accueil";
const char* DNOM = "Quel est votre nom ?";
const char* DPRENOM = "Quel est votre prénom ?";
const char* REPRESERV = "Réservation confirmé";


int testQuitter(char tampon[]) {
    return strcmp(tampon, EXIT) == 0;
}

int testAide(char tampon[]) {
    return strcmp(tampon, HELP) == 0;
}

int testList(char tampon[]) {
    return strcmp(tampon, LIST) == 0;
}

int testReserv(char tampon[]) {
    return strcmp(tampon, RESERV) == 0;
}

int testAnnul(char tampon[]) {
    return strcmp(tampon, ANNUL) == 0;
}

int LibreNDossier(long nDossier) {
    FILE* fichier;
    fichier = fopen("Reservation.txt", "r");
    if (fichier != NULL) {
        char chaine[MAX_BUFFER];
        int place;
        char* nom;
        char* prenom;
        long Ndossier;
        while (fgets(chaine, MAX_BUFFER, fichier) != NULL) {
            sscanf(chaine, "%d %s %s %ld", &place, nom, prenom, &Ndossier);
            if (nDossier == Ndossier)
                return 0;
        }
        fclose(fichier);
    }
    return 1;
}

long generateNDossier(int place) {
    long NDossier;
    do {
        srand(time(NULL));
        NDossier =((rand()%(MAX_NUMDOSS-MIN_NUMDOSS)) + MIN_NUMDOSS );
    } while (!(LibreNDossier(NDossier)));

    return NDossier;
}

int dispo(int i) {
    FILE* fichier;
    fichier = fopen("Reservation.txt", "r");
    if (fichier != NULL){
        char chaine[MAX_BUFFER];
        int place;
        char* nom;
        char* prenom;
        long NDossier;
        while (fgets(chaine, MAX_BUFFER, fichier) != NULL) {
            sscanf(chaine, "%d %s %s %ld", &place, nom, prenom,&NDossier);
            if (i == place)
                return 0;
        }
        fclose(fichier);
    }
    return 1;
}

int dispolist(int i) {
    FILE* fichier;
    fichier = fopen("Reservation.txt", "r");
    if (fichier != NULL) {
        char chaine[MAX_BUFFER];
        int place;
        while (fgets(chaine, MAX_BUFFER, fichier) != NULL) {
            sscanf(chaine, "%d", &place);
            if (i == place)
                return 0;
        }
        fclose(fichier);
    }
    return 1;
}

void ListReserv(int fdSocketCommunication) {
    char chaine[MAX_BUFFER] = "D=Disponible I=Indisponible\n";
    for (int i = 0; i < MAX_RESERV; i++) {
        char* chaine2;
        if(!dispolist(i))
            sprintf(chaine2, "%2d : I\n", i);
        else
            sprintf(chaine2, "%2d : D\n", i);
        strcat(chaine, chaine2);
    }
    send(fdSocketCommunication, (const char*)chaine, strlen(chaine), 0);
}

void AjoutReserv(int place, char* nom, char* prenom) {
    FILE* fichier;
    fichier = fopen("Reservation.txt", "a");
    if (fichier != NULL) {
        char* chaine;
        sprintf(chaine, "%d %s %s %ld\n", place, nom, prenom,generateNDossier(place));
        fputs(chaine, fichier);
        fclose(fichier);
    }
}

void Reserver(int fdSocketCommunication,char* tampon) {
   int nbRecu;
   int place;
   send(fdSocketCommunication,DPLACE, strlen(DPLACE), 0);
   nbRecu = recv(fdSocketCommunication, tampon, MAX_BUFFER, 0);
   if (nbRecu > 0) {
       tampon[nbRecu] = 0;
       place = atoi(tampon);
       if (place > MAX_RESERV || place < 0){
           send(fdSocketCommunication, NPLACE, strlen(NPLACE), 0);
       }
       else {
           if(!dispo(place))
               send(fdSocketCommunication, INDISPLACE, strlen(INDISPLACE), 0);
           else {
               send(fdSocketCommunication, DNOM, strlen(DNOM), 0);
               nbRecu = recv(fdSocketCommunication, tampon, MAX_BUFFER, 0);
               if (nbRecu > 0) {
                   tampon[nbRecu] = 0;
                   char* nom = strdup(tampon);
                   printf("%s\n", nom);
                   send(fdSocketCommunication, DPRENOM, strlen(DPRENOM), 0);
                   nbRecu = recv(fdSocketCommunication, tampon, MAX_BUFFER, 0);
                   if (nbRecu > 0) {
                       tampon[nbRecu] = 0;
                       char* prenom = strdup(tampon);
                       printf("%s\n", prenom);
                       AjoutReserv(place, nom, prenom);
                       send(fdSocketCommunication, REPRESERV, strlen(REPRESERV), 0);
                   }
               }
           }
       }
   }
}


int main(int argc, char const* argv[]) {
    int fdSocketAttente;
    int fdSocketCommunication;
    struct sockaddr_in coordonneesServeur;
    struct sockaddr_in coordonneesAppelant;
    char tampon[MAX_BUFFER];
    int nbRecu;
    int longueurAdresse;
    int pid;

    fdSocketAttente = socket(PF_INET, SOCK_STREAM, 0);

    if (fdSocketAttente < 0) {
        printf("socket incorrecte\n");
        exit(EXIT_FAILURE);
    }

    // On prépare l’adresse d’attachement locale
    longueurAdresse = sizeof(struct sockaddr_in);
    memset(&coordonneesServeur, 0x00, longueurAdresse);

    coordonneesServeur.sin_family = PF_INET;
    // toutes les interfaces locales disponibles
    coordonneesServeur.sin_addr.s_addr = htonl(INADDR_ANY);
    // toutes les interfaces locales disponibles
    coordonneesServeur.sin_port = htons(PORT);

    if (bind(fdSocketAttente, (struct sockaddr*)&coordonneesServeur, sizeof(coordonneesServeur)) == -1) {
        printf("erreur de bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(fdSocketAttente, 5) == -1) {
        printf("erreur de listen\n");
        exit(EXIT_FAILURE);
    }

    socklen_t tailleCoord = sizeof(coordonneesAppelant);

    int nbClients = 0;

    while (nbClients < MAX_CLIENTS) {
        if ((fdSocketCommunication = accept(fdSocketAttente, (struct sockaddr*)&coordonneesAppelant,
            &tailleCoord)) == -1) {
            printf("erreur de accept\n");
            exit(EXIT_FAILURE);
        }

        printf("Client connecté - %s:%d\n",
            inet_ntoa(coordonneesAppelant.sin_addr),
            ntohs(coordonneesAppelant.sin_port));

        if ((pid = fork()) == 0) {
            close(fdSocketAttente);
            send(fdSocketCommunication, CLIENT, strlen(CLIENT), 0);
            while (1) {
                // on attend le message du client
                // la fonction recv est bloquante
                nbRecu = recv(fdSocketCommunication, tampon, MAX_BUFFER, 0);

                if (nbRecu > 0) {
                    tampon[nbRecu] = 0;

                    printf("Recu de %s:%d : %s\n",
                        inet_ntoa(coordonneesAppelant.sin_addr),
                        ntohs(coordonneesAppelant.sin_port),
                        tampon);
                    if (testAide(tampon))
                        send(fdSocketCommunication, HELPREP, strlen(HELPREP), 0);
                    else if (testReserv(tampon))
                        Reserver(fdSocketCommunication, tampon);
                    else if (testList(tampon))
                        ListReserv(fdSocketCommunication);
                    else if (testQuitter(tampon))
                        break; // on quitte la boucle
                    else
                        send(fdSocketCommunication, ERROR, strlen(ERROR), 0);
                }
            }

            exit(EXIT_SUCCESS);
        }

        nbClients++;
    }

    close(fdSocketCommunication);
    close(fdSocketAttente);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        wait(NULL);
    }

    printf("Fin du programme.\n");
    return EXIT_SUCCESS;
}