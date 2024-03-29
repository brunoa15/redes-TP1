#include "funcoes.hpp"
#include "common.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <map>
#include <set>

#define BUFSZ 500

using namespace std;

void usage(int argc, char **argv) {
  printf("usage: %s <server port>\n", argv[0]);
  printf("example: %s 51511\n", argv[0]);
  exit(EXIT_FAILURE);
}

struct client_data {
  int csock;
  struct sockaddr_storage storage;
};

void * client_thread(void *data) {
  struct client_data *cdata = (struct client_data *)data;
  struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

  char caddrstr[BUFSZ];
  addrtostr(caddr, caddrstr, BUFSZ);
  printf("[log] connection from %s\n", caddrstr);

  set<string> tags;
  tags_clientes.insert(make_pair(cdata->csock, tags));

  char buf_recebido[BUFSZ];
  string buf_envio;
  size_t count;

  while(1) {
    memset(buf_recebido, 0, BUFSZ);
    count = recv(cdata->csock, buf_recebido, BUFSZ - 1, 0);
    buf_envio = tratar_mensagem(buf_recebido, cdata->csock);
    count = send(cdata->csock, buf_envio.c_str(), buf_envio.size(), 0);
    if (count != buf_envio.size()) {
      logexit("send");
    }
  }
  
  close(cdata->csock);
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argc, argv);
  }

  struct sockaddr_storage storage;
  if (0 != server_sockaddr_init(argv[1], &storage)) {
    usage(argc, argv);
  }

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if (s == -1) {
    logexit("socket");
  }

  int enable = 1;
  if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
    logexit("setsockopt");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != bind(s, addr, sizeof(struct sockaddr))) {
    logexit("bind");
  }

  if (0 != listen(s, 10)) {
    logexit("listen");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);
  printf("bound to %s, waiting connections\n", addrstr);

  while (1) {
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);

    int csock = accept(s, caddr, &caddrlen);
    if (csock == -1) {
      logexit("accept");
    }

    struct client_data *cdata = (struct client_data *)malloc(sizeof(*cdata));
    if (!cdata) {
      logexit("malloc");
    }
    cdata->csock = csock;
    memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, cdata);
  }

  exit(EXIT_SUCCESS);
}
