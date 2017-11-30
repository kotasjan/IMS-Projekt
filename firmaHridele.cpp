/*
 * @file firmaHridele.cpp
 * @author Jan Kotas<xkotas07@stud.fit.vutbr.cz>
 * @author Matuš Burzala<xburza@stud.fit.vutbr.cz>
 */
#include <iostream>
#include <ctime>
#include "simlib.h"

const int PRACOVNI_DOBA = 8;
const int POCET_ZAMESTNANCU = 4;
const int SERIE_KUSU = 5;

const int POCET_VRTACEK = 1;
const int POCET_SOUSTRUHU = 1;
const int POCET_FREZ = 1;
const int POCET_BRUSEK = 1;

const int DOVOLENA = 20; // 5 tydnu => 25 dnu (5 dni z toho je celozavodka)
const double PRAVDEPODOBNOST_DOVOLENE = 0.09; // pravdepodobnost, ze si zamestnanec v konkretni den vezme dovolenou

const int MINUTA = 60;
const int HODINA = 60 * MINUTA;
const int DEN = 24 * HODINA;
const int ROK = 365 * DEN;

const double SIMULACNI_DOBA = 1*ROK;

Facility vrtacka[POCET_VRTACEK];
Facility soustruh[POCET_SOUSTRUHU];
Facility freza[POCET_FREZ];
Facility bruska[POCET_BRUSEK];

Zamestnanec z[4];

bool den = false;
bool volno = false;

int days = 0;


class GEN_CELOZAVODKA : public Event {

	void Behavior() {
		if(volno && !celozavodka && (days % 7 != 1))
			Activate(Time + (DEN));
		else if (!volno && !celozavodka && (days % 7 == 1)){
			(new GEN_VOLNO(5*DEN))->Activate();
			Activate(Time + (5*DEN));
		} else {

		}

	}
};

class GEN_VOLNO : public Event {
	
	int time

	GEN_VOLNO(int time) {
		this->time = time;
	}

	void Behavior() {
		if(!volno){
			volno = true;
			Activate(Time + (this->time));
		}  else {
			volno = false;
		}
	}
};

class GEN_DEN : public Event {

	void Behavior() {

		days++;

		if(days % 358 = 0)
			(new GEN_CELOZAVODKA())->Activate();

		if(days % 7 == 6 || days % 7 == 0)
			(new GEN_VOLNO(DEN))->Activate();

		if(!den) {
			den = true;
			Activate(Time + (8*HODINA));
		} else {
			den = false;
			(new GEN_VOLNO(16*HODINA))->Activate();
			Activate(Time + (16*HODINA));
		}
	}
};

int main(int argc, char **argv) {

	RandomSeed(time(NULL));
#ifdef DEBUG
	SetOutput("stats.out");
#endif
	// inicializace experimentu
	Init(0,SIMULACNI_DOBA);   

	// aktivace udalosti zmen dne
	(new GEN_DEN)->Activate();
	
	
	// simulace
	Run();


	//tisk statistik
	//odbaveni_letadla.Output();
	//cekani_bezp.Output();
	//odbaveni_zakaznika.Output();

}  