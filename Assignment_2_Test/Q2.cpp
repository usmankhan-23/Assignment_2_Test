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
		friend class Cell;
		int id;
		int geneCount;
		int telomere;
		Gene* genes;
		static int liveCount;

		Chromosome& operator=(const Chromosome&);

	};
	
	Cell();
	Cell(const char* species, int ploidy, int chromosomes, int genesPerChromosome,int telomere, const char* genome);
	Cell(const char* species, int ploidy, int chromosomes, int genesPerChromosome, const char* genome);
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
	char* const species;
	int ploidy;
	int chromosomeCount;
	int genesPerChromosome;
	Chromosome* chromosomes;
	bool valid;
	static int liveCells;
	static int totalCreated;
	static int totalMutations;
	static int totalCrossovers;
	static int refusedDivisions;
	static uint64_t seed;
	static int copyErrorRate;
	static int crossoverPercent;
	bool validSpecies(const char* species) const;
	static uint64_t nextRandom(uint64_t& state);
	static void applyCopyErrors(Cell& target, int bp, uint64_t& state);
	Cell** meiosisImpl(int& produced, uint64_t& state) const;
	Cell& operator=(const Cell&);
};
int Cell::liveCells = 0;
int Cell::totalCreated = 0;
int Cell::totalMutations = 0;
int Cell::totalCrossovers = 0;
int Cell::refusedDivisions = 0;
uint64_t Cell::seed = 0;
int Cell::copyErrorRate = 100;
int Cell::crossoverPercent = 50;
int Cell::Chromosome::liveCount = 0;
bool Cell::validSpecies(const char* species) const{
	if (species == nullptr){
		return false;
	}
	int length = 0;
	while (species[length] != '\0'){
		char ch = species[length];
		if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')){
			return false;
		}
		length++;
		if (length > 20){
			return false;
		}
	}
	return length >= 1 && length <= 20;
}
Cell::Cell(const char* species, int ploidy, int chromosomes,int genesPerChromosome, int telomere, const char* genome): species(new char[strlen(species == nullptr ? "UNKNOWN" : species) + 1]){
	strcpy(this->species, species == nullptr ? "UNKNOWN" : species);
	this->ploidy = 0;
	this->chromosomeCount = 0;
	this->genesPerChromosome = 0;
	this->chromosomes = nullptr;
	this->valid = false;
	Cell::liveCells++;
	Cell::totalCreated++;
	if (!this->validSpecies(species)){
		return;
	}
	if (ploidy != 1 && ploidy != 2){
		return;
	}
	if (chromosomes < 1 || chromosomes > 16){
		return;
	}
	if (ploidy == 2 && chromosomes % 2 != 0){
		return;
	}
	if (genesPerChromosome < 1 || genesPerChromosome > 12){
		return;
	}
	if (telomere < 0 || telomere > 50){
		return;
	}
	if (genome == nullptr){
		return;
	}
	int expectedLength = chromosomes * genesPerChromosome;
	if (static_cast<int>(strlen(genome)) != expectedLength){
		return;
	}
	for (int i = 0; i < expectedLength; i++){
		if (!((genome[i] >= 'A' && genome[i] <= 'Z') || (genome[i] >= 'a' && genome[i] <= 'z'))){
			return;
		}
	}
	if (ploidy == 2){
		for (int pair = 0; pair < chromosomes / 2; pair++){
			for (int locus = 0; locus < genesPerChromosome; locus++){
				int first = (pair * 2) * genesPerChromosome + locus;
				int second = (pair * 2 + 1) * genesPerChromosome + locus;
				char a = genome[first];
				char b = genome[second];
				if (a >= 'a' && a <= 'z'){
					a = a - 'a' + 'A';
				}
				if (b >= 'a' && b <= 'z'){
					b = b - 'a' + 'A';
				}
				if (a != b){
					return;
				}
			}
		}
	}
	this->ploidy = ploidy;
	this->chromosomeCount = chromosomes;
	this->genesPerChromosome = genesPerChromosome;
	this->chromosomes = new Chromosome[chromosomes];
	for (int i = 0; i < chromosomes; i++){
		this->chromosomes[i].id = i;
		this->chromosomes[i].geneCount = genesPerChromosome;
		this->chromosomes[i].telomere = telomere;
		this->chromosomes[i].genes = new Chromosome::Gene[genesPerChromosome];
		for (int j = 0; j < genesPerChromosome; j++){
			this->chromosomes[i].genes[j].allele =genome[i * genesPerChromosome + j];
			this->chromosomes[i].genes[j].locus = j;
		}
	}
	this->valid = true;
}
Cell::Cell(const char* species, int ploidy, int chromosomes,int genesPerChromosome, const char* genome): Cell(species, ploidy, chromosomes, genesPerChromosome, 12, genome){}
Cell::Cell(const Cell& other): species(new char[strlen(other.species) + 1]){
	strcpy(this->species, other.species);
	this->ploidy = other.ploidy;
	this->chromosomeCount = other.chromosomeCount;
	this->genesPerChromosome = other.genesPerChromosome;
	this->valid = other.valid;
	this->chromosomes = nullptr;
	Cell::liveCells++;
	Cell::totalCreated++;
	if (other.chromosomeCount > 0 && other.chromosomes != nullptr){
		this->chromosomes = new Chromosome[other.chromosomeCount];
		for (int i = 0; i < other.chromosomeCount; i++){
			this->chromosomes[i].id = other.chromosomes[i].id;
			this->chromosomes[i].geneCount = other.chromosomes[i].geneCount;
			this->chromosomes[i].telomere = other.chromosomes[i].telomere;
			int count = other.chromosomes[i].geneCount;
			if (count > 0){
				this->chromosomes[i].genes = new Chromosome::Gene[count];
				for (int j = 0; j < count; j++){
					this->chromosomes[i].genes[j].allele = other.chromosomes[i].genes[j].allele;
					this->chromosomes[i].genes[j].locus = other.chromosomes[i].genes[j].locus;
				}
			}
		}
	}
}
Cell::~Cell(){
	delete[] this->chromosomes;
	delete[] this->species;
	Cell::liveCells--;
}
bool Cell::isValid() const{
	return this->valid;
}
const char* Cell::getSpecies() const{
	return this->species;
}
int Cell::getPloidy() const{
	return this->ploidy;
}
int Cell::getChromosomeCount() const{
	return this->chromosomeCount;
}
int Cell::getGenesPerChromosome() const{
	return this->genesPerChromosome;
}
int Cell::getTelomere(int chromosome) const{
	if (!this->valid){
		return 0;
	}
	return this->chromosomes[chromosome].getTelomere();
}
bool Cell::setTelomere(int chromosome, int value){
	if (!this->valid){
		return false;
	}
	if (chromosome < 0 || chromosome >= this->chromosomeCount){
		return false;
	}
	if (value < 0 || value > 50){
		return false;
	}
	this->chromosomes[chromosome].setTelomere(value);
	return true;
}
Cell::Chromosome& Cell::getChromosome(int index)
{
	return this->chromosomes[index];
}

const Cell::Chromosome& Cell::getChromosome(int index) const
{
	return this->chromosomes[index];
}
char* Cell::genomeString() const{
	if (!this->valid){
		char* result = new char[4];
		strcpy(result, "N/A");
		return result;
	}
	int length = this->chromosomeCount * this->genesPerChromosome;
	int totalLength = length + this->chromosomeCount;
	char* result = new char[totalLength + 1];
	int position = 0;
	for (int i = 0; i < this->chromosomeCount; i++){
		for (int j = 0; j < this->genesPerChromosome; j++){
			result[position] = this->chromosomes[i].gene(j).allele;
			position++;
		}
		if (i < this->chromosomeCount - 1){
			result[position] = '-';
			position++;
		}
	}
	result[position] = '\0';
	return result;
}
char Cell::phenotype(int pair, int locus) const{
	if (!this->valid){
		return '?';
	}
	if (locus < 0 || locus >= this->genesPerChromosome){
		return '?';
	}
	if (this->ploidy == 1){
		if (pair < 0 || pair >= this->chromosomeCount){
			return '?';
		}
		return this->chromosomes[pair].gene(locus).allele;
	}
	if (pair < 0 || pair >= this->chromosomeCount / 2){
		return '?';
	}
	char first = this->chromosomes[pair * 2].gene(locus).allele;
	char second = this->chromosomes[pair * 2 + 1].gene(locus).allele;
	if ((first >= 'A' && first <= 'Z') || (second >= 'A' && second <= 'Z')){
		if (first >= 'a' && first <= 'z'){
			first = first - 'a' + 'A';
		}
		return first;
	}
	if (first >= 'A' && first <= 'Z'){
		first = first - 'A' + 'a';
	}
	return first;
}
int Cell::totalLoci() const{
	if (!this->valid){
		return 0;
	}
	if (this->ploidy == 2){
		return (this->chromosomeCount / 2) *
			this->genesPerChromosome;
	}
	return this->chromosomeCount * this->genesPerChromosome;
}
int Cell::dominantCount() const{
	if (!this->valid){
		return 0;
	}
	int count = 0;
	if (this->ploidy == 1){
		for (int chromosome = 0; chromosome < this->chromosomeCount; chromosome++){
			for (int locus = 0; locus < this->genesPerChromosome; locus++){
				char allele = this->phenotype(chromosome, locus);
				if (allele >= 'A' && allele <= 'Z'){
					count++;
				}
			}
		}
	}
	else{
		for (int pair = 0; pair < this->chromosomeCount / 2; pair++){
			for (int locus = 0; locus < this->genesPerChromosome; locus++){
				char allele = this->phenotype(pair, locus);
				if (allele >= 'A' && allele <= 'Z'){
					count++;
				}
			}
		}
	}
	return count;
}
double Cell::dominantRatio() const{
	int total = this->totalLoci();
	if (total == 0){
		return 0.0;
	}
	return static_cast<double>(this->dominantCount()) / total;
}
int Cell::countAllele(char allele) const{
	if (!this->valid){
		return 0;
	}
	int count = 0;
	for (int i = 0; i < this->chromosomeCount; i++){
		for (int j = 0; j < this->genesPerChromosome; j++){
			if (this->chromosomes[i].gene(j).allele == allele){
				count++;
			}
		}
	}
	return count;
}
int Cell::countAllele(char allele, int chromosome) const{
	if (!this->valid){
		return 0;
	}
	if (chromosome < 0 || chromosome >= this->chromosomeCount){
		return 0;
	}
	int count = 0;
	for (int j = 0; j < this->genesPerChromosome; j++){
		if (this->chromosomes[chromosome].gene(j).allele == allele){
			count++;
		}
	}
	return count;
}
bool Cell::sameGenome(const Cell& other) const{
	if (!this->valid || !other.valid){
		return false;
	}
	if (strcmp(this->species, other.species) != 0){
		return false;
	}
	if (this->ploidy != other.ploidy ||this->chromosomeCount != other.chromosomeCount ||this->genesPerChromosome != other.genesPerChromosome){
		return false;
	}
	for (int i = 0; i < this->chromosomeCount; i++){
		for (int j = 0; j < this->genesPerChromosome; j++){
			if (this->chromosomes[i].gene(j).allele !=other.chromosomes[i].gene(j).allele){
				return false;
			}
		}
	}
	return true;
}
Cell& Cell::irradiate(int bp){
	if (!this->valid){
		return *this;
	}
	if (bp < 0){
		bp = 0;
	}
	else if (bp > 10000){
		bp = 10000;
	}
	for (int i = 0; i < this->chromosomeCount; i++){
		for (int j = 0; j < this->genesPerChromosome; j++){
			Cell::seed = Cell::seed * 6364136223846793005ULL+ 1442695040888963407ULL;
			uint64_t d = Cell::seed;
			if (((d >> 33) % 10000) < static_cast<uint64_t>(bp)){
				this->chromosomes[i].gene(j).flipCase();
				Cell::totalMutations++;
			}
		}
	}
	return *this;
}
Cell& Cell::irradiate(int bp, uint64_t seed){
	if (!this->valid){
		return *this;
	}
	if (bp < 0){
		bp = 0;
	}
	else if (bp > 10000){
		bp = 10000;
	}
	uint64_t localState = seed;
	for (int i = 0; i < this->chromosomeCount; i++){
		for (int j = 0; j < this->genesPerChromosome; j++){
			localState = localState * 6364136223846793005ULL+ 1442695040888963407ULL;
			uint64_t d = localState;
			if (((d >> 33) % 10000) < static_cast<uint64_t>(bp)){
				this->chromosomes[i].gene(j).flipCase();
				Cell::totalMutations++;
			}
		}
	}
	return *this;
}

Cell* Cell::mitosis(){
	if (!this->valid){
		return new Cell();
	}
	for (int i = 0; i < this->chromosomeCount; i++){
		if (this->chromosomes[i].senescent()){
			Cell::refusedDivisions++;
			return new Cell();
		}
	}
	for (int i = 0; i < this->chromosomeCount; i++){
		this->chromosomes[i].shorten(2);
	}
	Cell* daughter = new Cell(*this);
	daughter->irradiate(Cell::copyErrorRate);
	return daughter;
}
Cell* Cell::mitosis(uint64_t seed, int bp){
	if (!this->valid){
		return new Cell();
	}
	for (int i = 0; i < this->chromosomeCount; i++){
		if (this->chromosomes[i].senescent()){
			Cell::refusedDivisions++;
			return new Cell();
		}
	}
	for (int i = 0; i < this->chromosomeCount; i++){
		this->chromosomes[i].shorten(2);
	}
	Cell* daughter = new Cell(*this);
	daughter->irradiate(bp, seed);
	return daughter;
}
Cell** Cell::meiosisImpl(int& produced, uint64_t& state) const{
	produced = 0;
	if (!this->valid || this->ploidy != 2){
		return nullptr;
	}
	for (int i = 0; i < this->chromosomeCount; i++){
		if (this->chromosomes[i].senescent()){
			Cell::refusedDivisions++;
			return nullptr;
		}
	}
	int pairs = this->chromosomeCount / 2;
	int g = this->genesPerChromosome;
	int totalGenes = this->chromosomeCount * g;
	char* work = new char[totalGenes];
	for (int i = 0; i < this->chromosomeCount; i++){
		for (int j = 0; j < g; j++){
			work[i * g + j] =
				this->chromosomes[i].gene(j).allele;
		}
	}
	for (int p = 0; p < pairs; p++){
		uint64_t d = Cell::nextRandom(state);
		if (g >= 2 && ((d >> 33) % 100) < static_cast<uint64_t>(Cell::crossoverPercent)){
			int cp = 1 + static_cast<int>((d >> 20) % (g - 1));
			for (int j = cp; j < g; j++){
				char temp = work[(p * 2) * g + j];
				work[(p * 2) * g + j] = work[(p * 2 + 1) * g + j];
				work[(p * 2 + 1) * g + j] = temp;
			}
			Cell::totalCrossovers++;
		}
	}
	int* pole1 = new int[pairs];
	int* pole2 = new int[pairs];
	for (int p = 0; p < pairs; p++){
		uint64_t d = Cell::nextRandom(state);
		if (((d >> 40) & 1) == 0){
			pole1[p] = p * 2;
			pole2[p] = p * 2 + 1;
		}
		else{
			pole1[p] = p * 2 + 1;
			pole2[p] = p * 2;
		}
	}
	Cell** gametes = new Cell * [4];
	for (int k = 0; k < 4; k++){
		char* gameteGenome = new char[pairs * g + 1];
		int position = 0;
		for (int p = 0; p < pairs; p++){
			int chromosome;
			if (k < 2){
				chromosome = pole1[p];
			}
			else{
				chromosome = pole2[p];
			}
			for (int j = 0; j < g; j++){
				gameteGenome[position] = work[chromosome * g + j];
				position++;
			}
		}
		gameteGenome[position] = '\0';
		gametes[k] = new Cell(this->species, 1, pairs, g, 12, gameteGenome
		);
		delete[] gameteGenome;
		for (int p = 0; p < pairs; p++){
			int chromosome;
			if (k < 2){
				chromosome = pole1[p];
			}
			else{
				chromosome = pole2[p];
			}
			int telomere = this->chromosomes[chromosome].getTelomere() - 1;
			if (telomere < 0){
				telomere = 0;
			}
			gametes[k]->setTelomere(p, telomere);
		}
	}
	for (int k = 0; k < 4; k++){
		Cell::applyCopyErrors(*gametes[k], Cell::copyErrorRate, state
		);
	}
	delete[] work;
	delete[] pole1;
	delete[] pole2;
	produced = 4;
	return gametes;
}
Cell** Cell::meiosis(int& produced) const{
	return Cell::meiosisImpl(produced, Cell::seed);
}
Cell** Cell::meiosis(int& produced, uint64_t seed) const{
	uint64_t localState = seed;
	return Cell::meiosisImpl(produced, localState);
}
Cell* Cell::fertilize(const Cell& father, const Cell& mother){
	if (!father.valid || !mother.valid){
		return new Cell();
	}
	if (father.ploidy != 1 || mother.ploidy != 1){
		return new Cell();
	}
	if (strcmp(father.species, mother.species) != 0){
		return new Cell();
	}
	if (father.chromosomeCount != mother.chromosomeCount || father.genesPerChromosome != mother.genesPerChromosome){
		return new Cell();
	}
	if (father.chromosomeCount * 2 > 16){
		return new Cell();
	}
	int chromosomes = father.chromosomeCount * 2;
	int genes = father.genesPerChromosome;
	int totalGenes = chromosomes * genes;
	char* genome = new char[totalGenes + 1];
	for (int p = 0; p < father.chromosomeCount; p++){
		for (int j = 0; j < genes; j++){
			genome[(p * 2) * genes + j] = father.chromosomes[p].gene(j).allele;
			genome[(p * 2 + 1) * genes + j] = mother.chromosomes[p].gene(j).allele;
		}
	}
	genome[totalGenes] = '\0';
	Cell* child = new Cell(father.species,2,chromosomes,genes,12,genome);
	delete[] genome;
	for (int p = 0; p < father.chromosomeCount; p++){
		int fatherTelomere = father.chromosomes[p].getTelomere();
		int motherTelomere = mother.chromosomes[p].getTelomere();
		child->setTelomere(p * 2, fatherTelomere);
		child->setTelomere(p * 2 + 1, motherTelomere);
	}
	return child;
}
void Cell::setSeed(uint64_t s){
	Cell::seed = s;
}
uint64_t Cell::getSeed(){
	return Cell::seed;
}
void Cell::setCopyErrorRate(int bp){
	if (bp < 0){
		bp = 0;
	}
	else if (bp > 10000){
		bp = 10000;
	}
	Cell::copyErrorRate = bp;
}
int Cell::getCopyErrorRate(){
	return Cell::copyErrorRate;
}
void Cell::setCrossoverPercent(int p){
	if (p < 0){
		p = 0;
	}
	else if (p > 100){
		p = 100;
	}
	Cell::crossoverPercent = p;
}
int Cell::getCrossoverPercent(){
	return Cell::crossoverPercent;
}
int Cell::getLiveCells(){
	return Cell::liveCells;
}
int Cell::getTotalCreated(){
	return Cell::totalCreated;
}
int Cell::getTotalMutations(){
	return Cell::totalMutations;
}
int Cell::getTotalCrossovers(){
	return Cell::totalCrossovers;
}
int Cell::getRefusedDivisions(){
	return Cell::refusedDivisions;
}
void Cell::resetStatistics(){
	Cell::totalCreated = 0;
	Cell::totalMutations = 0;
	Cell::totalCrossovers = 0;
	Cell::refusedDivisions = 0;
	Cell::seed = 0;
	Cell::copyErrorRate = 100;
	Cell::crossoverPercent = 50;
}
void Cell::print() const{
	printf("%s | Ploidy: %d | Chromosomes: %d | Genes/Chr: %d | Telomeres: ",this->species,this->valid ? this->ploidy : 0,this->valid ? this->chromosomeCount : 0,this->valid ? this->genesPerChromosome : 0);
	if (!this->valid){
		printf("-");
	}
	else{
		for (int i = 0; i < this->chromosomeCount; i++){
			if (i > 0){
				printf(",");
			}
			printf("%d", this->chromosomes[i].getTelomere());
		}
	}
	int dominant = this->valid ? this->dominantCount() : 0;
	int total = this->valid ? this->totalLoci() : 0;
	double ratio = total > 0? static_cast<double>(dominant) / total	: 0.0;
	printf(" | Dominant: %d/%d (%.2f) | Valid: %s | Genome: ",dominant, total, ratio,this->valid ? "YES" : "NO");
	if (!this->valid){
		printf("N/A\n");
	}
	else{
		char* genome = this->genomeString();
		printf("%s\n", genome);
		delete[] genome;
	}
}






uint64_t Cell::nextRandom(uint64_t& state){
	state = state * 6364136223846793005ULL + 1442695040888963407ULL;
	return state;
}
void Cell::applyCopyErrors(Cell& target, int bp, uint64_t& state){
	if (!target.valid){
		return;
	}
	if (bp < 0){
		bp = 0;
	}
	else if (bp > 10000){
		bp = 10000;
	}
	for (int i = 0; i < target.chromosomeCount; i++){
		for (int j = 0; j < target.genesPerChromosome; j++){
			uint64_t d = Cell::nextRandom(state);
			if (((d >> 33) % 10000) < static_cast<uint64_t>(bp)){
				target.chromosomes[i].gene(j).flipCase();
				Cell::totalMutations++;
			}
		}
	}
}







Cell::Cell(): species(new char[8]){strcpy(this->species, "UNKNOWN");
	this->ploidy = 0;
	this->chromosomeCount = 0;
	this->genesPerChromosome = 0;
	this->chromosomes = nullptr;
	this->valid = false;
	Cell::liveCells++;
}
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
