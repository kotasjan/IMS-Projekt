/*
 * @file firmaHridele.cpp
 * @author Jan Kotas<xkotas07@stud.fit.vutbr.cz>
 * @author Matuš Burzala<xburza@stud.fit.vutbr.cz>
 */

#include <iostream>
#include <ctime>
#include <string>
#include <iomanip>
#include "simlib.h"

using namespace std;

const int PRACOVNI_DOBA = 8;
const int POCET_ZAMESTNANCU = 3;
const int SERIE_KUSU = 5;
const int POCET_GENEROVANYCH_KUSU = 200;

// nemenit - program nepodporuje nepodporuje vice (zatim)
// const int POCET_VRTACEK = 1;
// const int POCET_SOUSTRUHU = 1;
// const int POCET_FREZ = 1;
// const int POCET_BRUSEK = 1;

const int DOVOLENA = 29;  // 5 tydnu => 25 dnu (5 dni z toho je celozavodka) + 9 statni svatek
// pravdepodobnost, ze si zamestnanec v konkretni den vezme dovolenou
const double PRAVDEPODOBNOST_DOVOLENE = 0.105;

const int MINUTA = 60;
const int HODINA = 60 * MINUTA;
const int DEN = 24 * HODINA;
const int ROK = 365 * DEN;

const double SIMULACNI_DOBA = 5 * ROK;  // Mozne nastavit dle potreby

Facility f_vrtacka;
Facility f_soustruh;
Facility f_freza;
Facility f_bruska;

Queue q_zamestnanci;
Queue q_surove_kusy;
Queue q_vyvrtane_kusy;
Queue q_vysoustruzene_kusy;
Queue q_vyfrezovane_kusy;
Queue q_hridele;

bool den = false;
bool volno = false;
bool celozavodka = false;
bool debug = false;

int days = 0;
int celkovyPocetHrideli = 0;

int pocet_pracujicich_zamestnancu = POCET_ZAMESTNANCU;

// TStat delka_fronty_materialu("Delka pocatecni fronty surovych kusu", 0);
// TStat delka_fronty_vyvrtanych_kusu("Delka fronty vyvrtanych kusu", 0);
// TStat delka_fronty_vysoustruzenych_kusu("Delka fronty vysoustruzenych kusu",
// 0);
// TStat delka_fronty_vyfrezovanych_kusu("Delka fronty vyfrezovanych kusu", 0);
// TStat delka_fronty_vybrousenych_kusu("Delka fronty vybrousenych kusu", 0);
// TStat pocet_vyrobenych_hrideli("Pocet vyrobenych hrideli", 0);


class HRIDEL : public Process {
  void Behavior() {
    q_hridele.Insert(this);
    celkovyPocetHrideli++;
    Passivate();
    Cancel();
  }
};

class VYFREZOVANY_KUS : public Process {
  bool correct;

  void Behavior() {
    q_vyfrezovane_kusy.Insert(this);

    Passivate();

    Wait(5.2 * MINUTA);

    if (Random() <= 0.012) {
      correct = false;
      Passivate();
    } else {
      correct = true;
      Passivate();
    }

    (new HRIDEL)->Activate();

    Cancel();
  }

 public:
  int getQuality() { return this->correct; }
};

class VYSOUSTRUZENY_KUS : public Process {
  bool correct;

  void Behavior() {
    q_vysoustruzene_kusy.Insert(this);

    Passivate();

    Wait(7.3 * MINUTA);

    if (Random() <= 0.017) {
      correct = false;
      Passivate();
    } else {
      correct = true;
      Passivate();
    }

    (new VYFREZOVANY_KUS)->Activate();

    Cancel();
  }

 public:
  int getQuality() { return this->correct; }
};

class VYVRTANY_KUS : public Process {
  bool correct;

  void Behavior() {
    q_vyvrtane_kusy.Insert(this);

    Passivate();

    Wait(6.1 * MINUTA);

    if (Random() <= 0.025) {
      correct = false;
      Passivate();
    } else {
      correct = true;
      Passivate();
    }

    (new VYSOUSTRUZENY_KUS)->Activate();

    Cancel();
  }

 public:
  int getQuality() { return this->correct; }
};

class SUROVY_KUS : public Process {
  bool correct;

  void Behavior() {
    q_surove_kusy.Insert(this);

    Passivate();

    Wait(14.4 * MINUTA);

    if (Random() <= 0.04) {
      correct = false;
      Passivate();
    } else {
      correct = true;
      Passivate();
    }

    (new VYVRTANY_KUS)->Activate();

    Cancel();
  }

 public:
  int getQuality() { return this->correct; }
};

class ZAMESTNANEC : public Process {
  int dovolena;

  void Behavior() {
    while (1) {
      if ((Random() <= PRAVDEPODOBNOST_DOVOLENE) && (dovolena > 0)) {
        dovolena--;
        pocet_pracujicich_zamestnancu--;

        q_zamestnanci.Insert(this);

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
          if (den && !volno) {
            if (q_vyfrezovane_kusy.Length() >= 5) {
              if (!f_bruska.Busy()) {
                Seize(f_bruska);

                this->Brouseni();

                Release(f_bruska);
              }
            } else if (q_vysoustruzene_kusy.Length() >= 5) {
              if (!f_freza.Busy()) {
                Seize(f_freza);

                this->Frezovani();

                Release(f_freza);
              }
            } else if (q_vyvrtane_kusy.Length() >= 5) {
              if (!f_soustruh.Busy()) {
                Seize(f_soustruh);

                this->Soustruzeni();

                Release(f_soustruh);
              }
            } else if (q_surove_kusy.Length() >= 5) {
              if (!f_vrtacka.Busy()) {
                Seize(f_vrtacka);

                this->Vrtani();

                Release(f_vrtacka);
              }
            }
          } else {
            q_zamestnanci.Insert(this);
            break;
          }
          Wait(5);
        }
      }
      Passivate();
    }
  }

  void Vrtani() {
    int pocet_zmetku = 0;
    int pocet_hotovych_kusu = 0;

    while (pocet_hotovych_kusu < 5) {
      if (!q_surove_kusy.Empty()) {
        SUROVY_KUS *k = (SUROVY_KUS *)q_surove_kusy.GetFirst();
        k->Activate();

        // provadeni prace na stroji
        Wait(14.4 * MINUTA);

        // kontrola vyrobku
        Wait(30);

        if (k->getQuality()) {
          pocet_hotovych_kusu++;
          k->Activate();
        } else {
          pocet_zmetku++;
          k->Cancel();
        }
      } else {
        Wait(5);
      }

      if (!den) break;
    }

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

    if (pocet_hotovych_kusu == 5 && den) {
      if (Random() <= 0.5) Wait(Exponential(5 * MINUTA));
    }
  }

  void Soustruzeni() {
    int pocet_zmetku = 0;
    int pocet_hotovych_kusu = 0;

    while (pocet_hotovych_kusu < 5) {
      if (!q_vyvrtane_kusy.Empty()) {
        VYVRTANY_KUS *k = (VYVRTANY_KUS *)q_vyvrtane_kusy.GetFirst();
        k->Activate();

        // provadeni prace na stroji
        Wait(6.1 * MINUTA);

        // kontrola vyrobku
        Wait(30);

        if (k->getQuality()) {
          pocet_hotovych_kusu++;
          k->Activate();
        } else {
          pocet_zmetku++;
          k->Cancel();
        }
      } else {
        Wait(5);
      }

      if (!den) break;
    }

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

    if (pocet_hotovych_kusu == 5 && den) {
      if (Random() <= 0.5) Wait(Exponential(5 * MINUTA));
    }
  }

  void Frezovani() {
    int pocet_zmetku = 0;
    int pocet_hotovych_kusu = 0;

    while (pocet_hotovych_kusu < 5) {
      if (!q_vysoustruzene_kusy.Empty()) {
        VYSOUSTRUZENY_KUS *k =
            (VYSOUSTRUZENY_KUS *)q_vysoustruzene_kusy.GetFirst();
        k->Activate();

        // provadeni prace na stroji
        Wait(7.3 * MINUTA);

        // kontrola vyrobku
        Wait(30);

        if (k->getQuality()) {
          pocet_hotovych_kusu++;
          k->Activate();
        } else {
          pocet_zmetku++;
          k->Cancel();
        }
      } else {
        Wait(5);
      }

      if (!den) break;
    }

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

    if (pocet_hotovych_kusu == 5 && den) {
      if (Random() <= 0.5) Wait(Exponential(5 * MINUTA));
    }
  }

  void Brouseni() {
    int pocet_zmetku = 0;
    int pocet_hotovych_kusu = 0;

    while (pocet_hotovych_kusu < 5) {
      if (!q_vyfrezovane_kusy.Empty()) {
        VYFREZOVANY_KUS *k = (VYFREZOVANY_KUS *)q_vyfrezovane_kusy.GetFirst();
        k->Activate();

        // provadeni prace na stroji
        Wait(5.2 * MINUTA);

        // kontrola vyrobku
        Wait(30);

        if (k->getQuality()) {
          pocet_hotovych_kusu++;
          k->Activate();
        } else {
          pocet_zmetku++;
          k->Cancel();
        }
      } else {
        Wait(5);
      }

      if (!den) break;
    }

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

    if (pocet_hotovych_kusu == 5 && den) {
      if (Random() <= 0.5) Wait(Exponential(5 * MINUTA));
    }
  }

 public:
  ZAMESTNANEC(int delka_dovolene) : dovolena(delka_dovolene) { Activate(); }
};

class GEN_ZAMESTNANCU : public Event {
  void Behavior() {
    for (int i = 0; i < POCET_ZAMESTNANCU; i++) {
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

class GEN_SUROVYCH_KUSU : public Event {
  void Behavior() {
    for (int i = 0; i < POCET_GENEROVANYCH_KUSU; i++)
      (new SUROVY_KUS)->Activate();
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
    Activate(Time + (7 * DEN + 1));
  }
};

class GEN_VOLNO : public Event {
  int time;

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
      Activate(Time + (time));
    } else {
      volno = false;
    }
  }

 public:
  GEN_VOLNO(int t) : time(t) { Activate(); }
};

class GEN_CELOZAVODKA : public Event {
  void Behavior() {
    if (!celozavodka && (volno || (days % 7 != 1))){
      Activate(Time + (DEN));
    } else if (!volno && !celozavodka) {
      celozavodka = true;
      (new GEN_VOLNO(5 * DEN - 1))->Activate();
      if (debug) {
        unsigned long long t = Time;
        cout << setw(3) << (int)t / ROK << "r, " << setw(3)
             << (int)(t % ROK) / DEN << "d,  " << setw(2)
             << (int)(t % DEN) / HODINA << ":" << setw(2)
             << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
             << "     ::  " << ("Celozávodka započata.") << endl;
      }
      Activate(Time + (5 * DEN));
    } else {
      celozavodka = false;
    }
  }
};

class GEN_DEN : public Event {
  void Behavior() {

    // v sobotu a nedeli se nepracuje
    if ((days % 7 == 6 || (days % 7 == 0 && days != 0)) && !den) {
      (new GEN_VOLNO(DEN - 1))->Activate();
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

      // pokud nebyl den, nastav ho na true a proved aktivaci procesu
    } else if (!den) {
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

      // jednou za rok je zastavena cinnost firmy a je vyhlasena celozavodka
      if (days % 365 == 358) (new GEN_CELOZAVODKA)->Activate();

      den = true;
      days++;

      // pokud neni volno, aktivuj zamestnance
      if (!volno) {
        pocet_pracujicich_zamestnancu = 0;
        for (int i = 0; i < POCET_ZAMESTNANCU; i++) {
          if (!q_zamestnanci.Empty()) {
            pocet_pracujicich_zamestnancu++;
            (q_zamestnanci.GetFirst())->Activate();
          } else {
            if (debug)
              cout << "! Prazdny list zamestnancu ! Pocet = "
                   << to_string(pocet_pracujicich_zamestnancu) << endl;
            break;
          }
        }
      }
      Activate(Time + (8 * HODINA));
    } else {  // pokud je konec pracovniho dne, nastav volno na 16h
      den = false;
      Activate(Time + (16 * HODINA));
    }
  }
};

int main(int argc, char **argv) {
  RandomSeed(time(NULL));

  if(debug) SetOutput("stats.out");

  // inicializace experimentu
  Init(0, SIMULACNI_DOBA);

  // vygeneruj zamestnance
  (new GEN_ZAMESTNANCU)->Activate();

  // aktivace udalosti zmen dne
  (new GEN_DEN)->Activate();

    // vygeneruj zamestnance
  (new GEN_SUROVYCH_KUSU)->Activate();

  // simulace
  Run();

//  if (debug) {
    unsigned long long t = Time;
    cout << setw(3) << (int)t / ROK << "r, " << setw(3) << (int)(t % ROK) / DEN
         << "d,  " << setw(2) << (int)(t % DEN) / HODINA << ":" << setw(2)
         << (int)(t % HODINA) / MINUTA << ":" << setw(2) << (t % MINUTA)
         << "     ::  " << ("Celkovy pocet vyrobenych hrideli = " +
                            to_string(celkovyPocetHrideli))
         << endl;
// }

  // tisk statistik
  // odbaveni_letadla.Output();
  // cekani_bezp.Output();
  // odbaveni_zakaznika.Output();
}