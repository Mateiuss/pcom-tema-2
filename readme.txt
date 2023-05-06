# Tema 2 - PCom
## DUDU Matei-Ioan - 323CB

---

### Structuri folosite

Pentru a explica rezolvarea acestei teme intr-un mod cat mai inteligibil, voi
incepe prin a prezenta si explica structurile de date folosite:

#### udp_payload

```cpp
struct udp_payload {
    char topic[TOPIC_LEN];
    uint8_t tip_date;
    char payload[PAYLOAD_LEN];
};
```

* In aceasta structura sunt stocate informatiile venite de la clientii UDP
in conformitate cu cerinta

#### tcp_packet

```cpp
struct tcp_packet {
  sockaddr_in udp_client;
  udp_payload payload;
};
```

* Aceasta este structura pe care server-ul o trimite catre clientii TCP
* Structura contine informatiile (ip, port) despre clientul UDP care a trimis
mesajul si mesajul propriu-zis salvat in structura de mai devreme

#### notify_packet

```cpp
struct notify_packet {
  char command[COMMAND_LEN];
  char topic[TOPIC_LEN];
  uint8_t sf;
};
```

* Mesajele pe care clientii TCP le trimit catre server sunt salvate sub forma
structurii de mai sus
* Dupa cum se poate observa, aceasta cuprinde comanda data (exit, subscribe sau
ubsubscribe), topic-ul si sf (unde este cazul)

#### client

```cpp
struct client {
  char id[ID_LEN];
  int fd;
  map<string, bool> topics;
  queue<tcp_packet> sf_queue;
};
```

* Structura de mai sus este cea folosita de catre server pentru a retine infor-
tiile necesare despre clientii TCP
* Dupa cum se observa si mai sus, aceasta contine id-ul clientului, fd-ul acestuia
(setat la -1 daca clientul este deconectat), un map care are drept cheie topic-ul
la care clientul este abonat si valoarea un bool ce spune daca store and forward
este activat
* In final, structura mai contine si o coada a pachetelor pe care clientul urmeaza
sa le primeasca odata ce acesta se va reconecta (pentru topicurile cu SF activ)
* In plus, server-ul se foloseste de un map ce are drept cheie id-ul clientului
si drept valoare structura de tip client aferenta acestuia

### Protocol de trimitere al mesajelor

Din dorinta de a minimiza numarul de octeti trimisi de oricare dintre cele doua
entitati (server si client), am venit cu urmatoarea conventie:

- Inainte de a trimite un pachet (fie el un notify_packet sau tcp_packet), mai
intai se calculeaza si se trimite numarul de octeti utili ai pachetului (packet_len)
si abia dupa se trimit packet_len octeti catre destinatie

- Astfel, 
