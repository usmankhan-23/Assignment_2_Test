#include <iostream>
#include <cstring>
#include <cstdint>
#include <cstdio>
using namespace std;

class Cell{
public:
	class Chromosome{ 
	public:
		struct Gene{ 
			char allele;
			int locus;
			Gene(); 
			Gene(char allele, int locus);
			bool isDominant() const;
			char trait() const; 
			Gene& flipCase(); 
		};
		Chromosome();
		Chromosome(int id, int geneCount, int telomere);
		Chromosome(const Chromosome& other);
		~Chromosome();
		int getId() const;
		int getGeneCount() const;
		int getTelomere() const;
		void setTelomere(int t);
		bool senescent() const; 
		Chromosome& shorten(int amount);
		Gene& gene(int locus);
		const Gene& gene(int locus) const;
		static int live();
	private:
		int id;
		int geneCount;
		int telomere;
		Gene* genes;
		static int liveCount;

		Chromosome& operator=(const Chromosome&);

	};
	
	Cell();
	Cell(const char* species, int ploidy, int chromosomes, int genesPerChromosome,int telomere, const char* genome);
	Cell(const char* species, int ploidy, int chromosomes, int genesPerChromosome,const char* genome);
	Cell(const Cell& other);
	~Cell();
	bool isValid() const;
	const char* getSpecies() const;
	int getPloidy() const;
	int getChromosomeCount() const;
	int getGenesPerChromosome() const;
	int getTelomere(int chromosome) const;
	bool setTelomere(int chromosome, int value);
	Chromosome& getChromosome(int index);
	const Chromosome& getChromosome(int index) const;
	char* genomeString() const;
	char phenotype(int pair, int locus) const;
	int totalLoci() const;
	int dominantCount() const;
	double dominantRatio() const;
	int countAllele(char allele) const;
	int countAllele(char allele, int chromosome) const; 
	bool sameGenome(const Cell& other) const;
	Cell& irradiate(int bp);
	Cell& irradiate(int bp, uint64_t seed);
		Cell* mitosis();
	Cell* mitosis(uint64_t seed, int bp);
	Cell** meiosis(int& produced) const; 
	Cell** meiosis(int& produced, uint64_t seed) const; 
	static Cell* fertilize(const Cell& father, const Cell& mother);
	void print() const;
	static void setSeed(uint64_t s);
	static uint64_t getSeed();
	static void setCopyErrorRate(int bp);
	static int getCopyErrorRate();
	static void setCrossoverPercent(int p);
	static int getCrossoverPercent();
	static int getLiveCells();
	static int getTotalCreated();
	static int getTotalMutations();
	static int getTotalCrossovers();
	static int getRefusedDivisions();
	static void resetStatistics();
private:
	Cell& operator=(const Cell&);
};
int Cell::Chromosome::liveCount = 0;
Cell::Chromosome::Chromosome(){
	this->id = 0;
	this->geneCount = 0;
	this->telomere = 0;
	this->genes = nullptr;
	Cell::Chromosome::liveCount++;
}
Cell::Chromosome::Chromosome(int id, int geneCount, int telomere){
	this->id = id;
	this->geneCount = geneCount > 0 ? geneCount : 0;
	if (telomere < 0){
		this->telomere = 0;
	}
	else if (telomere > 50){
		this->telomere = 50;
	}
	else{
		this->telomere = telomere;
	}
	if (this->geneCount > 0){
		this->genes = new Gene[this->geneCount];
	}
	else{
		this->genes = nullptr;
	}
	Cell::Chromosome::liveCount++;
}
Cell::Chromosome::Chromosome(const Chromosome& other){
	this->id = other.id;
	this->geneCount = other.geneCount;
	this->telomere = other.telomere;
	if (this->geneCount > 0){
		this->genes = new Gene[this->geneCount];
		for (int i = 0; i < this->geneCount; i++){
			this->genes[i].allele = other.genes[i].allele;
			this->genes[i].locus = other.genes[i].locus;
		}
	}
	else{
		this->genes = nullptr;
	}
	Cell::Chromosome::liveCount++;
}
Cell::Chromosome::~Chromosome(){
	delete[] this->genes;
	this->genes = nullptr;
	Cell::Chromosome::liveCount--;
}
int Cell::Chromosome::live(){
	return Cell::Chromosome::liveCount;
}
int Cell::Chromosome::getId() const{
	return this->id;
}
int Cell::Chromosome::getGeneCount() const{
	return this->geneCount;
}
int Cell::Chromosome::getTelomere() const{
	return this->telomere;
}
void Cell::Chromosome::setTelomere(int t){
	if (t < 0){
		this->telomere = 0;
	}
	else if (t > 50){
		this->telomere = 50;
	}
	else{
		this->telomere = t;
	}
}
bool Cell::Chromosome::senescent() const{
	return this->telomere <= 0;
}
Cell::Chromosome& Cell::Chromosome::shorten(int amount){
	this->telomere = this->telomere - amount;
	if (this->telomere < 0){
		this->telomere = 0;
	}
	else if (this->telomere > 50){
		this->telomere = 50;
	}
	return *this;
}
Cell::Chromosome::Gene& Cell::Chromosome::gene(int locus){
	return this->genes[locus];
}
const Cell::Chromosome::Gene& Cell::Chromosome::gene(int locus) const{
	return this->genes[locus];
}
Cell::Chromosome::Gene::Gene(){
	this->allele = '?';
	this->locus = -1;
}
Cell::Chromosome::Gene::Gene(char allele, int locus){
	this->allele = allele;
	this->locus = locus;
}
bool Cell::Chromosome::Gene::isDominant() const{
	return allele >= 'A' && allele <= 'Z';
}
char Cell::Chromosome::Gene::trait() const{
	if (allele >= 'a' && allele <= 'z'){
		return allele - 'a' + 'A';
	}
	return allele;
}
Cell::Chromosome::Gene& Cell::Chromosome::Gene::flipCase(){
	if (allele >= 'A' && allele <= 'Z'){
		allele = allele - 'A' + 'a';
	}
	else if (allele >= 'a' && allele <= 'z'){
		allele = allele - 'a' + 'A';
	}
	return *this;
}
