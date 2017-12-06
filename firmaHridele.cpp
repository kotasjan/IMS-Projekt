/*
 * @file firmaHridele.cpp
 * @author Jan Kotas<xkotas07@stud.fit.vutbr.cz>
 * @author Matúš Burzala<xburza00@stud.fit.vutbr.cz>
 */

#include <iostream>
#include <ctime>
#include <string>
#include <iomanip>
#include "simlib.h"

using namespace std;

// Důležité konstanty pro efektivnejsi práci s časem a pro lepší abstrakci.
const int MINUTA = 60;
const int HODINA = 60 * MINUTA;
const int DEN = 24 * HODINA;
const int ROK = 365 * DEN;

// Nastavení počtu zaměstnanců pracujících ve firmě
const int POCET_ZAMESTNANCU = 4;

/*
 * Serie kusů představuje množství kusů, které zaměstnanec vyrobí
 * za sebou. Tzn. zaměstnanec si z fronty vezme tento počet kusů
 * a dokud je nezpracuje, nemůže změnit činnost ani jít na přestávku.
 * Jedinou výjimkou je konec pracovní doby.
*/
const int SERIE_KUSU = 5;

// Počet generovaných surových kusů za jednotku času (perioda generovaní).
const int POCET_GENEROVANYCH_KUSU = 260;

// Perioda generovaní představuje periodu dodávky surových kusů z dodavatelské
// firmy.
const int PERIODA_GENEROVANI = 7 * DEN;

// Nastaveni počtu zařízení.
const int POCET_VRTACEK = 2;
const int POCET_SOUSTRUHU = 1;
const int POCET_FREZ = 1;
const int POCET_BRUSEK = 1;

// 5 týdnu dovolené => 25 dnů (5 dní z toho je celozavodka) + 9 statní svátek.
const int DOVOLENA = 29;

/*
 * Pravděpodobnost, že si zaměstnanec v konkrétní den vezme dovolenou. Je to
 * vypočítaná hodnota (počet dnů vovolené děleno +-248 pracovních dnů za rok)
 * 29 / 248 ~= 0.119
*/
const double PRAVDEPODOBNOST_DOVOLENE = 0.119;

// Délka pracovní doby.
const double PRACOVNI_DOBA = 8 * HODINA;

// Simulační doba představuje délku simulace v simulačním čase.
const double SIMULACNI_DOBA = 10 * ROK;

/*
 * Zařízení, které obstarávají obsluhu.
*/
Facility f_vrtacka[POCET_VRTACEK];
Facility f_soustruh[POCET_SOUSTRUHU];
Facility f_freza[POCET_FREZ];
Facility f_bruska[POCET_BRUSEK];

/*
 * Fronty sloužící k pro bežící procesy.
*/

/*
 * Fronta se zaměstnanci. Do této fronty se umístí zaměstnanci na konci
 * směny a vybírají se z ní na začátku každé směny (každého dne).
*/
Queue q_zamestnanci;

/*
 * Tato fronta obsahuje procesy představující dovezený materiál (surové kusy)
*/
Queue q_surove_kusy;

/*
 * Do této fronty jsou umístěny čerstvě vyvrtané kusy.
*/
Queue q_vyvrtane_kusy;

/*
 * Do této fronty jsou umístěny čerstvě vysoustružené kusy.
*/
Queue q_vysoustruzene_kusy;

/*
 * Do této fronty jsou umístěny čerstvě vyfrézované kusy.
*/
Queue q_vyfrezovane_kusy;

/*
 * Do této fronty jsou umístěny čerstvě vybroušené, hotové hřídele.
*/
Queue q_hridele;

bool smena = false;  // Boolean, který představuje, zda je pracovní doba / směna.
bool volno = false;  // Pokud je celozávodka nebo víkend, je "true"
bool celozavodka = false; // Pokud je vyhlášena celozávodka, je "true"
bool end_simulation = false; // Den před dokončením simulace se nastaví na "true"

bool debug = false; // Pro zapnutí výpisu činností simulace.

int days = 0;                   // Představuje počet uběhlých dnů.
int celkovy_pocet_hrideli = 0;  // Představuje celkový počet hřídelí.

// Počet zaměstnanců, kteří aktuálně pracují (na začátku simulace pracují
// všichni)
int pocet_pracujicich_zamestnancu = POCET_ZAMESTNANCU;

// Histogram celkové doby výroby od vstupu surového kusu až po finální hřídel.
// Sleduje jak dlouhou dobu trvá zpracování surových kusů od vstupu do systému.
Histogram doba_vyroby("Doba surových kusů v systému", 0, 1 * DEN, 25);

// Statistika zobrazující efektivitu zaměstanců v procentech podle odpracovaného
// času.
Stat efektivita_zamestnance;

/*
 * Tato classa představuje transakce vyrobených hřídelí.
*/
class HRIDEL : public Process {
  double prichod;  // představuje informaci o času vstupu materiálu do systému

  void Behavior() {
    q_hridele.Insert(this);
    celkovy_pocet_hrideli++;
    doba_vyroby(Time - prichod);
    Passivate();
    Cancel();
  }

 public:
  // Při vytvoření hridele předáváme konstruktoru informaci o vstupnim case
  // materialu
  // do systemu.
  HRIDEL(double prichod_noveho_kusu) : prichod(prichod_noveho_kusu) {
    Activate();
  }
};

/*
 * Tato classa představuje transakce vyfrézovaných kusů.
 * Transakce zde prochází procesem broušení.
*/

class VYFREZOVANY_KUS : public Process {
  bool correct;  // představuje informaci o tom, zda se jedná o povedený kus
  double prichod;  // představuje informaci o času vstupu materiálu do systému

  void Behavior() {
    q_vyfrezovane_kusy.Insert(
        this);  // přidej do fronty čekajících vyfrézovaných kusů

    Passivate();

    Wait(5.2 * MINUTA);  // broušení vyfrézovaného kusu

    if (Random() <=
        0.012) {  // pokud je zmetek (šance 1,2%) označ correct = false
      correct = false;
      Passivate();
    } else {  // pokud se broušení povedlo, označ correct = true
      correct = true;
      Passivate();
    }

    new HRIDEL(prichod);  // vytvoří novou transakci HŘÍDEL a předá čas vstupu
                          // materiálu do systému

    Cancel();  // transakce se ukončí
  }

 public:
  // tento getter slouží pro informování zaměstnance o stavu výrobku
  int getQuality() { return this->correct; }

  // vrátí dobu příchodu materiálu
  double getPrichod() { return this->prichod; };

  // Při vytvoření vyfrezovaneho kusu předáváme konstruktoru informaci o
  // vstupnim case materialu
  // do systemu.
  VYFREZOVANY_KUS(double prichod_noveho_kusu) : prichod(prichod_noveho_kusu) {
    Activate();
  }
};

/*
 * Tato classa představuje transakce vysoustružených kusů.
 * Transakce zde prochází procesem frézování.
*/

class VYSOUSTRUZENY_KUS : public Process {
  bool correct;  // představuje informaci o tom, zda se jedná o povedený kus
  double prichod;  // představuje informaci o času vstupu materiálu do systému

  void Behavior() {
    q_vysoustruzene_kusy.Insert(
        this);  // přidej do fronty čekajících vysoustružených kusů

    Passivate();

    Wait(7.3 * MINUTA);

    if (Random() <=
        0.017) {  // pokud je zmetek (šance 1,7%) označ correct = false
      correct = false;
      Passivate();
    } else {  // pokud se frézování povedlo, označ correct = true
      correct = true;
      Passivate();
    }

    new VYFREZOVANY_KUS(prichod);  // vytvoří se nová transakce VYFREZOVANY_KUS
                                   // a předá se čas příchodu materiálu do
                                   // systému.

    Cancel();  // transakce se ukončí
  }

 public:
  // tento getter slouží pro informování zaměstnance o stavu výrobku
  int getQuality() { return this->correct; }

  // vrátí dobu příchodu materiálu
  double getPrichod() { return this->prichod; };

  // Při vytvoření vysoustruženého kusu předáváme konstruktoru informaci o
  // vstupnim case materiálu
  // do systemu.
  VYSOUSTRUZENY_KUS(double prichod_noveho_kusu) : prichod(prichod_noveho_kusu) {
    Activate();
  }
};

/*
 * Tato classa představuje transakce vyvrtaných kusů.
 * Transakce zde prochází procesem soustružení.
*/

class VYVRTANY_KUS : public Process {
  bool correct;  // představuje informaci o tom, zda se jedná o povedený kus
  double prichod;  // představuje informaci o času vstupu materiálu do systému

  void Behavior() {
    q_vyvrtane_kusy.Insert(
        this);  // přidej do fronty čekajících vyvrtaných kusů

    Passivate();

    Wait(6.1 * MINUTA);

    if (Random() <=
        0.025) {  // pokud je zmetek (šance 2,5%) označ correct = false
      correct = false;
      Passivate();
    } else {  // pokud se soustružení povedlo, označ correct = true
      correct = true;
      Passivate();
    }

    new VYSOUSTRUZENY_KUS(prichod);  // vytvoří se nová transakce
                                     // VYSOUSTRUZENY_KUS a předá se čas
                                     // příchodu materiálu do systému.

    Cancel();  // transakce se ukončí
  }

 public:
  // tento getter slouží pro informování zaměstnance o stavu výrobku
  int getQuality() { return this->correct; }

  // vrátí dobu příchodu materiálu
  double getPrichod() { return this->prichod; };

  // Při vytvoření vyvrtaného kusu předáváme konstruktoru informaci o vstupnim
  // case materiálu
  // do systemu.
  VYVRTANY_KUS(double prichod_noveho_kusu) : prichod(prichod_noveho_kusu) {
    Activate();
  }
};

/*
 * Tato classa představuje transakce vysoustružených kusů.
 * Transakce zde prochází procesem frézování.
*/

class SUROVY_KUS : public Process {
  bool correct;  // představuje informaci o tom, zda se jedná o povedený kus
  double prichod;  // představuje informaci o času vstupu materiálu do systému

  void Behavior() {
    q_surove_kusy.Insert(this);  // přidej do fronty čekajících surových kusů
    prichod = Time;

    Passivate();

    Wait(14.4 * MINUTA);

    if (Random() <= 0.04) {  // pokud je zmetek (šance 4%) označ correct = false
      correct = false;
      Passivate();
    } else {  // pokud se vrtání povedlo, označ correct = true
      correct = true;
      Passivate();
    }

    new VYVRTANY_KUS(prichod);  // vytvoří se nová transakce VYVRTANY_KUS a
                                // předá se čas vstupu materiálu do systému.

    Cancel();  // transakce se ukončí
  }

 public:
  // tento getter slouží pro informování zaměstnance o stavu výrobku
  int getQuality() { return this->correct; }

  // vrátí dobu příchodu materiálu
  double getPrichod() { return this->prichod; };

  SUROVY_KUS(int priorita) : Process(priorita) { Activate(); }
};

/*
 * Tato classa představuje transakce zaměstnanců firmy. V tomto procesu
 * se řídí hlavní část chodu programu. Zaměstnanci zajišťují obsluhu strojů
 * s jejichž pomocí vyrábí hotové hřídele. Transakce zde prochází procesem
 * frézování.
*/

class ZAMESTNANEC : public Process {
  int dovolena;                 // počet zbývajících dnů dovolené
  int pocet_hotovych_kusu = 0;  // počet zdařilých kusů v každé sérii
  double efektivni_odpracovana_doba = PRACOVNI_DOBA;  
  // doba, po kterou zaměstnanec opravdu pracoval

  void Behavior() {
    while (1) {
      // Pokud má zaměstnanec nárok na dovolenou a pravděpodobnost dovolené
      // se vyplní, zaměstnanec si ji vybere (den).
      if ((Random() <= PRAVDEPODOBNOST_DOVOLENE) && (dovolena > 0)) {
        dovolena--;
        pocet_pracujicich_zamestnancu--;

        q_zamestnanci.Insert(this);  // Vrátit zaměstnance zpět do fronty.

        if (debug) {
          unsigned long long t = Time;
          cout << "\n" << setw(3) << (int)t / ROK << "r, " << setw(3)
               << (int)(t % ROK) / DEN << "d,  " << setw(2)
               << (int)(t % DEN) / HODINA << ":" << setw(2)
               << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
               << "     ::  " << ("Zamestnanec ma dovolenou. Zbyva: ")
               << to_string(dovolena) << "\n" << endl;
        }
      } else {

        while (1) {
          if (smena && !volno) {
            if (q_vyfrezovane_kusy.Length() >= 5) { // Zaměstnanec potřebuje minimalně 5 kusů.
              for (int j = 0; j < POCET_BRUSEK; j++) {  // Brusky mají prioritu.
                if (!f_bruska[j].Busy()) {  // Najít nevytížené zařízení.
                  Seize(f_bruska[j]);  // Pokud je bruska volná, obsadit ji.

                  this->Brouseni();  // Přejdeme do funkce starající se o
                                     // broušení.

                  Release(f_bruska[j]);
                  break;
                }
              }
            } else if (q_vysoustruzene_kusy.Length() >= 5) {
              for (int j = 0; j < POCET_FREZ; j++) {
                if (!f_freza[j].Busy()) {  // Najít nevytížené zařízení.
                  Seize(f_freza[j]);  // Pokud je fréza volná, obsadit ji.

                  this->Frezovani();  // Přejdeme do funkce starající se o
                                      // frézování.

                  Release(f_freza[j]);
                  break;
                }
              }
            } else if (q_vyvrtane_kusy.Length() >= 5) {
              for (int j = 0; j < POCET_SOUSTRUHU; j++) {
                if (!f_soustruh[j].Busy()) {  // Najít nevytížené zařízení.
                  Seize(f_soustruh[j]);  // Pokud je soustruh volná, obsadit ji.

                  this->Soustruzeni();  // Přejdeme do funkce starající se o
                                        // soustružení.

                  Release(f_soustruh[j]);
                  break;
                }
              }
            } else if (q_surove_kusy.Length() >= 5) {
              for (int j = 0; j < POCET_VRTACEK; j++) {
                if (!f_vrtacka[j].Busy()) {  // Najít nevytížené zařízení.
                  Seize(f_vrtacka[j]);  // Pokud je vrtačka volná, obsadit ji.

                  this->Vrtani();  // Přejdeme do funkce starající se o �vrtání.

                  Release(f_vrtacka[j]);
                  break;
                }
              }
            }
          } else {  // Pokud není pracovní směna, vloží se zaměstnanci do
                    // fronty.
            q_zamestnanci.Insert(this);
            break;  // Cyklus se ukončí.
          }

          // Pokud zaměstnanec dokončil sérii a stále je pracovní směna, má
          // nárok na přestávku.
          if (pocet_hotovych_kusu != 0 && smena) {
            if (Random() <= 0.5) {
              double t = Exponential(5 * MINUTA);
              Wait(t);  // Zaměstnanec má přestávku exp(5min).
              // odečteme přestávku od efektivní pracovní doby
              efektivni_odpracovana_doba -= t; 
            }
            pocet_hotovych_kusu = 0;
          } else {
            Wait(5);
            // odečteme dobu čekání na zařízení
            efektivni_odpracovana_doba -= 5;
          }

          // Pokud je simulace u konce, ukončíme činńost zaměstnanců předtím,
          // než si zabere zařízení.
          if (Time >= SIMULACNI_DOBA - 2 * HODINA) this->Cancel();
        }

        // výpočet efektivity zaměstnance na konci směny a přidání do statistik
        efektivita_zamestnance(
            (int)((efektivni_odpracovana_doba / PRACOVNI_DOBA) * 100));
      }
      // na konci směny se obnoví počáteční hodnoty
      efektivni_odpracovana_doba = PRACOVNI_DOBA;
      pocet_hotovych_kusu = 0;
      Passivate();
    }
  }

  // Funkce pro simulaci vrtání.
  void Vrtani() {
    int pocet_zmetku = 0;  // Počet zmetků, které zaměstnanec v sérii vytvoří.

    Queue pom_q;  // Pomocná fronta pro zpracování surových kusů.

    while (pom_q.Length() != SERIE_KUSU) {
      if (!q_surove_kusy.Empty()){
        // Vybere se počet kusů splňující sérii a vloží se do pomocné fronty.
        pom_q.Insert(q_surove_kusy.GetFirst());
      } else if(!smena){
      	break;
  	  } else {
        Wait(5);
        efektivni_odpracovana_doba -= 5;
      }
    }

    while (pom_q.Length() != 0) {
      if (!pom_q.Empty()) {
        SUROVY_KUS *k = (SUROVY_KUS *)pom_q.GetFirst();
        k->Activate();

        // Provádění práce na stroji.
        Wait(14.4 * MINUTA);

        // Kontrola a manipulace s výrobkem.
        Wait(Exponential(30));

        // Zjistíme výsledek kontroly výrobku.
        if (k->getQuality()) {
          pocet_hotovych_kusu++;
          k->Activate();  // Výrobek kvalitní, tak ho posuneme do další fáze
                          // aktivací.
        } else {
          pocet_zmetku++;
          k->Cancel();  // Výrobek nekvalitní, tak ukončíme jeho činnost.
        }
      }

      // Pokud nastane situace, že skončí směna a zaměstnanec nedokončil sérii,
      // přesunou se nedodělané kusy zpět do původní fronty a cyklus se přeřuší.
      if (!smena) {
        while (!pom_q.Empty()) {
          SUROVY_KUS *p = (SUROVY_KUS *)pom_q.GetFirst();
          double time = p->getPrichod();
          p->Activate();
          new SUROVY_KUS(1);
        }
        break;
      }
    }

    // V případě aktivace debugu vypíšeme počet hotových a nepovedených kusů.
    if (debug) {
      unsigned long long t = Time;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << ("Počet špatně vyvrtaných kusů = " + to_string(pocet_zmetku))
           << endl;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << "Vyvrtáno " << to_string(pocet_hotovych_kusu) << " kusů" << endl;
    }
  }

  // Funkce pro simulaci soustružení.
  void Soustruzeni() {
    int pocet_zmetku = 0;  // Počet zmetků, které zaměstnanec v sérii vytvoří.

    Queue pom_q;  // Pomocná fronta pro zpracování vyvrtaných kusů.

    while (pom_q.Length() != SERIE_KUSU) {
      if (!q_vyvrtane_kusy.Empty()){
        // Vybere se počet kusů splňující sérii a vloží se do pomocné fronty.
        pom_q.Insert(q_vyvrtane_kusy.GetFirst());
      } else if(!smena){
      	break;
  	  } else {
        Wait(5);
        efektivni_odpracovana_doba -= 5;
      }
    }

    while (pom_q.Length() != 0) {
      if (!pom_q.Empty()) {
        VYVRTANY_KUS *k = (VYVRTANY_KUS *)pom_q.GetFirst();
        k->Activate();  // Aktivuje se vyvrtaný kus.

        // Provádění práce na stroji.
        Wait(6.1 * MINUTA);

        // Kontrola a manipulace s výrobkem.
        Wait(Exponential(30));

        // Zjistíme výsledek kontroly výrobku.
        if (k->getQuality()) {
          pocet_hotovych_kusu++;
          k->Activate();  // Výrobek kvalitní, tak ho posuneme do další fáze
                          // aktivací.
        } else {
          pocet_zmetku++;
          k->Cancel();  // Výrobek nekvalitní, tak ukončíme jeho činnost.
        }
      }

      // Pokud nastane situace, že skončí směna a zaměstnanec nedokončil sérii,
      // přesunou se nedodělané kusy zpět do původní fronty a cyklus se přeřuší.
      if (!smena) {
        while (!pom_q.Empty()) {
          VYVRTANY_KUS *p = (VYVRTANY_KUS *)pom_q.GetFirst();
          double time = p->getPrichod();
          p->Activate();
          new VYVRTANY_KUS(time);
        }
        break;
      }
    }

    // V případě aktivace debugu vypíšeme počet hotových a nepovedených kusů.
    if (debug) {
      unsigned long long t = Time;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << ("Počet špatně vysoustružených kusů = " + to_string(pocet_zmetku))
           << endl;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << "Vysoustruženo " << to_string(pocet_hotovych_kusu) << " kusů"
           << endl;
    }
  }

  // Funkce pro simulaci frézování.
  void Frezovani() {
    int pocet_zmetku = 0;  // Počet zmetků, které zaměstnanec v sérii vytvoří.

    Queue pom_q;  // Pomocná fronta pro zpracování vysoustružených kusů.

    while (pom_q.Length() != SERIE_KUSU) {
      if (!q_vysoustruzene_kusy.Empty()){
        // Vybere se počet kusů splňující sérii a vloží se do pomocné fronty.
        pom_q.Insert(q_vysoustruzene_kusy.GetFirst());
      } else if(!smena){
      	break;
  	  } else {
        Wait(5);
        efektivni_odpracovana_doba -= 5;
      }
    }

    while (pom_q.Length() != 0) {
      if (!pom_q.Empty()) {
        VYSOUSTRUZENY_KUS *k = (VYSOUSTRUZENY_KUS *)pom_q.GetFirst();
        k->Activate();  // Aktivuje se vysoustružený kus.

        // Provádění práce na stroji.
        Wait(7.3 * MINUTA);

        // Kontrola a manipulace s výrobkem.
        Wait(Exponential(30));

        // Zjistíme výsledek kontroly výrobku.
        if (k->getQuality()) {
          pocet_hotovych_kusu++;
          k->Activate();  // Výrobek kvalitní, tak ho posuneme do další fáze
                          // aktivací.
        } else {
          pocet_zmetku++;
          k->Cancel();  // Výrobek nekvalitní, tak ukončíme jeho činnost.
        }
      }

      // Pokud nastane situace, že skončí směna a zaměstnanec nedokončil sérii,
      // přesunou se nedodělané kusy zpět do původní fronty a cyklus se přeřuší.
      if (!smena) {
        while (!pom_q.Empty()) {
          VYSOUSTRUZENY_KUS *p = (VYSOUSTRUZENY_KUS *)pom_q.GetFirst();
          double time = p->getPrichod();
          p->Activate();
          new VYSOUSTRUZENY_KUS(time);
        }
        break;
      }
    }

    // V případě aktivace debugu vypíšeme počet hotových a nepovedených kusů.
    if (debug) {
      unsigned long long t = Time;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << ("Počet špatně vyfrézovaných kusů = " + to_string(pocet_zmetku))
           << endl;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << "Vyfrézováno " << to_string(pocet_hotovych_kusu) << " kusů"
           << endl;
    }
  }

  // Funkce pro simulaci broušení.
  void Brouseni() {
    int pocet_zmetku = 0;  // Počet zmetků, které zaměstnanec v sérii vytvoří.

    Queue pom_q;  // Pomocná fronta pro zpracování vyfrézovaných kusů.

    while (pom_q.Length() != SERIE_KUSU) {
      if (!q_vyfrezovane_kusy.Empty()) {
        // Vybere se počet kusů splňující sérii a vloží se do pomocné fronty.
        pom_q.Insert(q_vyfrezovane_kusy.GetFirst());
      } else if(!smena){
      	break;
  	  } else {
        Wait(5);
        efektivni_odpracovana_doba -= 5;
      }
    }

    while (pom_q.Length() != 0) {
      if (!pom_q.Empty()) {
        VYFREZOVANY_KUS *k = (VYFREZOVANY_KUS *)pom_q.GetFirst();
        k->Activate();  // Aktivuje se vyfrézovaný kus.

        // Provádění práce na stroji.
        Wait(5.2 * MINUTA);

        // Kontrola a manipulace s výrobkem.
        Wait(Exponential(30));

        // Zjistíme výsledek kontroly výrobku.
        if (k->getQuality()) {
          pocet_hotovych_kusu++;
          k->Activate();  // Výrobek kvalitní, tak ho posuneme do další fáze
                          // aktivací.
        } else {
          pocet_zmetku++;
          k->Cancel();  // Výrobek nekvalitní, tak ukončíme jeho činnost.
        }
      }

      // Pokud nastane situace, že skončí směna a zaměstnanec nedokončil sérii,
      // přesunou se nedodělané kusy zpět do původní fronty a cyklus se přeřuší.
      if (!smena) {
        while (!pom_q.Empty()) {
          VYFREZOVANY_KUS *p = (VYFREZOVANY_KUS *)pom_q.GetFirst();
          double time = p->getPrichod();
          p->Activate();
          new VYFREZOVANY_KUS(time);
        }
        break;
      }
    }

    // V případě aktivace debugu vypíšeme počet hotových a nepovedených kusů.
    if (debug) {
      unsigned long long t = Time;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << ("Počet špatně vybroušených kusů = " + to_string(pocet_zmetku))
           << endl;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << "Vybroušeno " << to_string(pocet_hotovych_kusu) << " hřídelí"
           << endl;
    }
  }

 public:
  // Při vytvoření nového zaměstnance předáváme konstruktoru informaci o počtu
  // dnů jeho dovolené.
  ZAMESTNANEC(int delka_dovolene) : dovolena(delka_dovolene) { Activate(); }
  void SetDovolena(int delka_dovolene) {
    this->dovolena = delka_dovolene;
    Activate();
  }
};

// Tato třída představuje událost, zajišťující generování zaměstnanců.
class GEN_ZAMESTNANCU : public Event {
  void Behavior() {
    for (int i = 0; i < POCET_ZAMESTNANCU; i++) {
      // Vygenerované zaměstnance umístíme do fronty zaměstnanců.
      q_zamestnanci.Insert(new ZAMESTNANEC(DOVOLENA));
    }
    if (debug) {
      unsigned long long t = Time;
      cout << setw(3) << (int)t / ROK << "r, " << setw(3)
           << (int)(t % ROK) / DEN << "d,  " << setw(2)
           << (int)(t % DEN) / HODINA << ":" << setw(2)
           << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
           << "     ::  "
           << "Vygenerováni " << to_string(q_zamestnanci.Length())
           << " zaměstnanci." << endl;
    }
  }
};

/*
 * Tento event se stará o periodické generování surových kusů.
 * Tyto surové kusy se následně vloží do fronty surových kusů
 * a jsou připravené pro zpracování obslužnou linkou. Dulezite
 * je pohlidat mnozstvi surovych kusu ve vstupni fronte. Pokud
 * je mnozstvi kusu vetsi nez pocet generovanych kusu, generovani
 * se nespusti.
*/
class GEN_SUROVYCH_KUSU : public Event {
  void Behavior() {
    if (q_surove_kusy.Length() < POCET_GENEROVANYCH_KUSU) {
      for (int i = 0; i < POCET_GENEROVANYCH_KUSU; i++) new SUROVY_KUS(0);
      if (debug) {
        unsigned long long t = Time;
        cout << setw(3) << (int)t / ROK << "r, " << setw(3)
             << (int)(t % ROK) / DEN << "d,  " << setw(2)
             << (int)(t % DEN) / HODINA << ":" << setw(2)
             << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
             << "     ::  "
             << ("Vygenerováno " + to_string(POCET_GENEROVANYCH_KUSU) +
                 " surových kusů.")
             << endl;
      }
    }

    // Nastaví se aktivace následujího eventu po uplynutí předem určené periody.
    Activate(Time + PERIODA_GENEROVANI);
  }
};

/*
 * Event starající se o simulaci volna zaměstnanců. Aktivuje se v případě
 * celozávodky nebo víkendu.
*/
class GEN_VOLNO : public Event {
  int time;  // Atribut představující dobu volna.

  void Behavior() {
    if (!volno) {
      volno = true;
      if (debug) {
        unsigned long long t = Time;
        cout << setw(3) << (int)t / ROK << "r, " << setw(3)
             << (int)(t % ROK) / DEN << "d,  " << setw(2)
             << (int)(t % DEN) / HODINA << ":" << setw(2)
             << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
             << "     ::  "
             << ("Volno na " + to_string((int)(time % ROK) / DEN) + " dnů, " +
                 to_string((int)(time % DEN) / HODINA) + " hod, " +
                 to_string((int)(time % HODINA) / MINUTA) + " min, " +
                 to_string((int)(time % MINUTA)) + " sec.")
             << endl;
      }
      // Nastavíce další aktivace na dobu t + doba trvání volna.
      Activate(Time + (time));
    } else {
      volno = false;
    }
  }

 public:
  // Konstruktor pro nastavení doby volna.
  GEN_VOLNO(int t) : time(t) { Activate(); }
};

/*
 * Jednou za rok se aktivuje event celozávodka. Celozávodka představuje
 * volno pro celou firmu (všechny zaměstnance) po dobu pěti pracovních dnů.
 * Důležitým aspektem je, že celozávodka (volno) se vždy generuje od
 * následujícího pondělí, tedy tak, že je celkově volno 9 dní
 * (2 x víkend + 5 pracovních dnů).
*/
class GEN_CELOZAVODKA : public Event {
  void Behavior() {
    if (!celozavodka && (volno || (days % 7 != 1))) {
      // Pokud není pondělí a celozávodka není započata, počkáme do
      // následujícího dne.
      if (!end_simulation) Activate(Time + (DEN));
    } else if (!volno && !celozavodka) {
      celozavodka = true;
      // Celozávodka započala, generujeme volno 5 dnů.
      (new GEN_VOLNO(5 * DEN - 1))->Activate();
      if (debug) {
        unsigned long long t = Time;
        cout << setw(3) << (int)t / ROK << "r, " << setw(3)
             << (int)(t % ROK) / DEN << "d,  " << setw(2)
             << (int)(t % DEN) / HODINA << ":" << setw(2)
             << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
             << "     ::  " << ("Celozávodka započata.") << endl;
      }
      // Pro ukončení celozávodky potřebujeme opět aktivovat událost.
      if (!end_simulation) Activate(Time + (5 * DEN));
    } else {
      // Celozávodka je ukončena.
      celozavodka = false;
    }
  }
};

/*
 * Tato třída generuje periodicky den. Den rozdělujeme na pracovní dobu a dobu
 * volna. Jestliže je ovšem sobota nebo neděle, je automaticky celý den volno.
*/
class GEN_DEN : public Event {
  bool pridej_dovolenou = false;
  void Behavior() {
    // V sobotu a neděli se nepracuje.
    if ((days % 7 == 6 || (days % 7 == 0 && days != 0)) && !smena) {
      if (!end_simulation) (new GEN_VOLNO(DEN - 1))->Activate();
      days++;

      if (debug && days != 0) {
        cout << "--------------------------------------------------------------"
                "---------"
             << endl;
        unsigned long long t = Time;
        cout << "Počet pracujících zaměstnanců = "
             << pocet_pracujicich_zamestnancu << endl;
        cout << "Surových k.: " << to_string(q_surove_kusy.Length())
             << ", Vyvrtaných k.: " << to_string(q_vyvrtane_kusy.Length())
             << ", Vysoustružených k.: "
             << to_string(q_vysoustruzene_kusy.Length())
             << ", Vyfrézovaných k.: " << to_string(q_vyfrezovane_kusy.Length())
             << ", Hřídelí.: " << to_string(q_hridele.Length()) << endl;
        cout << "--------------------------------------------------------------"
                "---------"
             << endl;
      }

      Activate(Time + DEN);

      // Pokud nebyla po minulé aktivaci nastavena směna => je volno, je směna
      // aktivována.
    } else if (!smena) {
      if (debug && days != 0) {
        cout << "--------------------------------------------------------------"
                "---------"
             << endl;
        unsigned long long t = Time;
        cout << "Počet pracujících zaměstnanců = "
             << pocet_pracujicich_zamestnancu << endl;
        cout << "Surových k.: " << to_string(q_surove_kusy.Length())
             << ", Vyvrtaných k.: " << to_string(q_vyvrtane_kusy.Length())
             << ", Vysoustružených k.: "
             << to_string(q_vysoustruzene_kusy.Length())
             << ", Vyfrézovaných k.: " << to_string(q_vyfrezovane_kusy.Length())
             << ", Hřídelí.: " << to_string(q_hridele.Length()) << endl;
        cout << "--------------------------------------------------------------"
                "---------"
             << endl;
      }

      // Jednou za rok je zastavena činnost firmy a je vyhlášena celozávodka.
      if (days % 365 == 358 && !end_simulation) {
        pridej_dovolenou = true;
        (new GEN_CELOZAVODKA)->Activate();
      }

      smena = true;
      days++;

      // Pokud není volno, aktivuj zaměstnance.
      if (!volno) {
        pocet_pracujicich_zamestnancu = 0;
        for (int i = 0; i < POCET_ZAMESTNANCU; i++) {
          if (!q_zamestnanci.Empty()) {
            pocet_pracujicich_zamestnancu++;
            if (pridej_dovolenou) {
              ZAMESTNANEC *z = (ZAMESTNANEC *)q_zamestnanci.GetFirst();
              z->SetDovolena(DOVOLENA);
            } else {
              (q_zamestnanci.GetFirst())->Activate();
            }
          } else {
            if (debug)
              cout << "! Prazdny list zamestnancu ! Pocet = "
                   << to_string(pocet_pracujicich_zamestnancu) << endl;
            break;
          }
        }
        pridej_dovolenou = false;
      }
      // Pokud není konec simulace menší než doba příští aktivace, nastav příští
      // aktivaci.
      if (Time < SIMULACNI_DOBA - PRACOVNI_DOBA)
        Activate(Time + (PRACOVNI_DOBA));

    } else {  // Pokud je konec směny, nastav volno na zbytek dne.
      smena = false;
      if (!end_simulation) Activate(Time + (DEN - PRACOVNI_DOBA));
    }
  }
};

/*
 * Tato událost se spustí přesně den před plánovaným koncem simulace a připraví
 * tak simulaci
 * na její ukončení. Vteřinu před koncem simulace se aktivuje a ukončí všechny
 * zbývající transakce.
*/
class GEN_END : public Event {
  void Behavior() {
    if (end_simulation) {
      while (!q_surove_kusy.Empty()) {
        (q_surove_kusy.GetFirst())->Cancel();
      }

      while (!q_vyvrtane_kusy.Empty()) {
        (q_vyvrtane_kusy.GetFirst())->Cancel();
      }

      while (!q_vysoustruzene_kusy.Empty()) {
        (q_vysoustruzene_kusy.GetFirst())->Cancel();
      }

      while (!q_vyfrezovane_kusy.Empty()) {
        (q_vyfrezovane_kusy.GetFirst())->Cancel();
      }

      while (!q_hridele.Empty()) {
        (q_hridele.GetFirst())->Cancel();
      }

      while (!q_zamestnanci.Empty()) {
        (q_zamestnanci.GetFirst())->Cancel();
      }

    } else {
      // Nastav signalizaci blížícího se konce simulace.
      end_simulation = true;
      // Aktivuj událost vteřinu před koncem simulace.
      Activate(Time + DEN - 1);
    }
  }
};

/*
 * Řídící funkce main. Nastavuje a aktivuje simulaci.
*/
int main(int argc, char **argv) {
  RandomSeed(time(NULL));

  // Inicializace experimentu.
  Init(0, SIMULACNI_DOBA);

  // Nastavení názvů vrtaček.
  for (int i = 0; i < POCET_VRTACEK; i++) {
    f_vrtacka[i].SetName("Vrtacka");
  }
  // Nastavení názvů soustruhů.
  for (int i = 0; i < POCET_SOUSTRUHU; i++) {
    f_soustruh[i].SetName("Soustruh");
  }
  // Nastavení názvů fréz.
  for (int i = 0; i < POCET_FREZ; i++) {
    f_freza[i].SetName("Freza");
  }
  // Nastavení názvů brusek.
  for (int i = 0; i < POCET_BRUSEK; i++) {
    f_bruska[i].SetName("Bruska");
  }

  // Aktivace události blížícího se konce simulace den před koncem simulace.
  (new GEN_END)->Activate(SIMULACNI_DOBA - DEN);

  // Generuj zaměstnance.
  (new GEN_ZAMESTNANCU)->Activate();

  // Aktivace dne.
  (new GEN_DEN)->Activate();

  // Aktivace periodického generování surových kusů.
  (new GEN_SUROVYCH_KUSU)->Activate();

  // Start simulace.
  Run();

  // Vypiš celkový počet hřídelí.
  unsigned long long t = Time;
  cout << setw(3) << (int)t / ROK << "r, " << setw(3) << (int)(t % ROK) / DEN
       << "d,  " << setw(2) << (int)(t % DEN) / HODINA << ":" << setw(2)
       << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
       << "     ::  " << ("Celkovy pocet vyrobenych hrideli = " +
                          to_string(celkovy_pocet_hrideli))
       << endl;

  // Tisk statistik. Statistiky se nevytisknou, pokud je zaplý debug.

  for (int i = 0; i < POCET_VRTACEK; i++) f_vrtacka[i].Output();

  for (int i = 0; i < POCET_SOUSTRUHU; i++) f_soustruh[i].Output();

  for (int i = 0; i < POCET_FREZ; i++) f_freza[i].Output();

  for (int i = 0; i < POCET_BRUSEK; i++) f_bruska[i].Output();

  // Tisk histogramu doby výroby hřídele
  doba_vyroby.Output();

  // Tisk statistiky efektivity zaměstnanců.
  efektivita_zamestnance.Output();
}