
                Tema 2 - Aplicatie client-server TCP si UDP pentru gestionarea mesajelor
        
        Programele construite functioneaza in felul urmator (descris succint):
          server.c:
            - Serverul porneste si deschide 2 socketi (unul de listen pentru clienti
              TCP si unul pentru datagrame primite de la clienti prin UDP).
            - In cadrul serverului se creeaza o lista de profile ale clientilor TCP.
            - Cand clientii TCP vor sa se conecteze li se va creea un nou profil 
              (numarul de profile de clienti este variabil dar numarul de socket-uri
              TCP deschise este limitat).
            - Gestiunea datelor este facuta de catre server care multiplexeaza intre
              mai multe socket-uri deschise, interpreteaza datele primite si trimite
              mai departe mesaje (daca e nevoie)
            - Serverul mai tine de asemenea si log-urile de mesaje netransmise ale 
              clientilor pe topicurile cu optiunea store-forward activata.
        
          subscriber.c:
            - Clientul deschide isi trimite id-ul serverului pentru autentificare
              si dupa se conecteaza dupa caz.
            - Multiplexeaza intre citire de la standard input si citire de pe socketul
              serverului pentru date de la server pe care apoi le gestioneaza in
              functie de informatiile primite de la protocolul definit de mine

          client_utils.c & client_utils.h:
            - contin structura profilelor clientilor TCP (cli_info) si functiile
              necesare pentru gestiunea acestora.

          protocol.c & protocol.h:
            - contin structura protocolului definit de mine (myProtocol) si functiile
              necesare pentru gestiunea acestora.

        Protocolul creat de mine functioneaza in felul urmator:
          - avem o structura myProtocol cu 3 campuri: type, size si payload.
          - type (1 B) - semnifica tipul de mesaj ce vrem sa il trimitem in cadrul
            aplicatiei si va fi interpretat de catre receptor.
          - size (4 B) - semnifica marimea (in B) a payload-ului trimis
          - payload (<size> B) - semnifica informatiile pe care vrem sa le transmitem
          
          Functiile majore folosite (send si receive):
          - trimiterea mesajelor prin protocol:
              - se creeaza un buffer de forma: |type (1 B) | payload (<size> B) |
              - se trimite marimea payload-ului (size)
              - se trimite buffer-ul creat
          - primirea mesajelor prin protocol:
              - se primeste marimea payload-ului (size)
              - se initializeaza o variabila de tip myProtocol
              - se primeste buffer-ul si se completeaza variabila
              - variabila se foloseste pentru interpretare mai departe

        PS: Mai multe detalii, in special legate de implementare, se regasesc in cod.    
         
