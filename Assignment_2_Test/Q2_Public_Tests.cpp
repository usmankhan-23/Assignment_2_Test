// ----------------------------------------------------------------------------
// Q2 - Cell Division Lab - PUBLIC tests (given to students with the assignment)
// How to use (Visual Studio 2019 / 2022):
//   1. Create a "Google Test" project (or add the NuGet package
//      Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn to a Console project).
//   2. Put THIS file and the student's Q2.cpp in the same folder.
//   3. Add ONLY this test file to the project. Do NOT add Q2.cpp as a project source:
//      this file already does  #include "Q2.cpp"  and the classes would be defined twice.
//   4. Build and run (Test Explorer or the console). C++ language standard: C++17.
// Every test resets the statistics first and checks at the end that no Cell or
// Chromosome object is left alive (memory leak detection through the live counters).
// ----------------------------------------------------------------------------
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include "pch.h"
#include "Q2.cpp"

namespace {

    // repeats a unit n times (slot = which of the 4 buffers to use, so a few calls can live in one expression)
    char RB[4][1200];
    [[maybe_unused]] const char* rep(const char* u, int n, int slot) {
        int len = (int)std::strlen(u), pos = 0;
        for (int i = 0; i < n; i++) for (int k = 0; k < len; k++) RB[slot][pos++] = u[k];
        RB[slot][pos] = 0;
        return RB[slot];
    }
    void freeG(Cell** g, int n) { for (int i = 0; i < n; i++) delete g[i]; delete[] g; }
    uint64_t lcg(uint64_t s, int times) {
        for (int i = 0; i < times; i++) s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return s;
    }

}  // namespace

class CellLab : public ::testing::Test {
protected:
    void SetUp() override { Cell::resetStatistics(); }
    void TearDown() override {
        EXPECT_EQ(0, Cell::getLiveCells()) << "Cell objects were leaked (or deleted twice)";
        EXPECT_EQ(0, Cell::Chromosome::live()) << "Chromosome objects were leaked (or deleted twice)";
        Cell::resetStatistics();
    }
};

#define CAPTURE_BEGIN() testing::internal::CaptureStdout()
#define CAPTURE_END()   testing::internal::GetCapturedStdout()

// P1: Default constructor and live counter
// checks: default cell is the invalid UNKNOWN cell; default cell is not counted as created but is counted as alive; destructor uncounts
TEST_F(CellLab, P1_Default_constructor_and_live_counter) {
    CAPTURE_BEGIN();
    {
        {
            Cell c;
            c.print();
            cout << Cell::getLiveCells() << " " << Cell::getTotalCreated() << "\n";
        }
        cout << Cell::getLiveCells() << "\n";
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("UNKNOWN | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "1 0\n"
        "0\n",
        out);
}

// P2: Valid diploid cell, phenotype, counting overloads
// checks: parameterised ctor, nested Chromosome/Gene, dominance rules, bad indexes give '?', both countAllele overloads
TEST_F(CellLab, P2_Valid_diploid_cell_phenotype_counting_overloads) {
    CAPTURE_BEGIN();
    {
        Cell c("Mammoth", 2, 4, 2, 5, "AbabCDcD");
        c.print();
        cout << c.phenotype(0, 0) << c.phenotype(0, 1) << c.phenotype(1, 0) << c.phenotype(1, 1)
            << c.phenotype(2, 0) << c.phenotype(0, 2) << c.phenotype(-1, 0) << "\n";
        cout << c.dominantCount() << " " << c.countAllele('b') << " " << c.countAllele('b', 1) << " " << c.countAllele('A') << "\n";
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("Mammoth | Ploidy: 2 | Chromosomes: 4 | Genes/Chr: 2 | Telomeres: 5,5,5,5 | Dominant: 3/4 (0.75) | Valid: YES | Genome: Ab-ab-CD-cD\n"
        "AbCD???\n"
        "3 2 1 1\n",
        out);
}

// P3: Ten kinds of invalid input
// checks: every invalid case degrades to an invalid cell without crashing (whitespace species, bad ploidy, odd chromosomes, short genome, non-letter, homologue mismatch, null genome, telomere 51, null species, zero chromosomes); still counted as created
TEST_F(CellLab, P3_Ten_kinds_of_invalid_input) {
    CAPTURE_BEGIN();
    {
        Cell a("Bad Name", 2, 2, 2, 5, "ABab");  a.print();
        Cell b("Moa", 3, 2, 2, 5, "ABab");       b.print();
        Cell c("Moa", 2, 3, 2, 5, "ABabAB");     c.print();
        Cell d("Moa", 2, 2, 2, 5, "ABa");        d.print();
        Cell e("Moa", 2, 2, 2, 5, "AB1b");       e.print();
        Cell f("Moa", 2, 2, 2, 5, "ABac");       f.print();
        Cell g("Moa", 2, 2, 2, 5, nullptr);      g.print();
        Cell h("Moa", 2, 2, 2, 51, "ABab");      h.print();
        Cell i(nullptr, 1, 2, 2, 5, "ABab");     i.print();
        Cell j("Moa", 1, 0, 2, 5, "");           j.print();
        cout << Cell::getTotalCreated() << " " << Cell::getLiveCells() << "\n";
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("Bad Name | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "UNKNOWN | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "10 10\n",
        out);
}

// P4: Constructor overloading: default telomere
// checks: the constructor without a telomere argument gives every chromosome telomere 12
TEST_F(CellLab, P4_Constructor_overloading_default_telomere) {
    CAPTURE_BEGIN();
    {
        Cell c("Dodo", 1, 2, 3, "ABcdEF");
        c.print();
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("Dodo | Ploidy: 1 | Chromosomes: 2 | Genes/Chr: 3 | Telomeres: 12,12 | Dominant: 4/6 (0.67) | Valid: YES | Genome: ABc-dEF\n",
        out);
}

// P5: Copy constructor is a deep copy
// checks: copy is independent of the original; chromosome/cell live counters; copy counted as created; everything freed at scope end
TEST_F(CellLab, P5_Copy_constructor_is_a_deep_copy) {
    CAPTURE_BEGIN();
    {
        {
            Cell a("Moa", 2, 2, 2, "ABab");
            Cell b(a);
            b.setTelomere(0, 3);
            a.irradiate(10000, 7);
            a.print();
            b.print();
            cout << a.sameGenome(b) << " " << Cell::Chromosome::live() << " " << Cell::getLiveCells() << " "
                << Cell::getTotalCreated() << " " << Cell::getTotalMutations() << "\n";
        }
        cout << Cell::Chromosome::live() << " " << Cell::getLiveCells() << "\n";
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("Moa | Ploidy: 2 | Chromosomes: 2 | Genes/Chr: 2 | Telomeres: 12,12 | Dominant: 2/2 (1.00) | Valid: YES | Genome: ab-AB\n"
        "Moa | Ploidy: 2 | Chromosomes: 2 | Genes/Chr: 2 | Telomeres: 3,12 | Dominant: 2/2 (1.00) | Valid: YES | Genome: AB-ab\n"
        "0 4 2 2 4\n"
        "0 0\n",
        out);
}

// P6: Mitosis shortens telomeres and refuses at zero
// checks: parent -2, daughter equals shortened parent, clamp at 0, third division refused (invalid result, refusal counted), LCG advanced even when the error rate is 0
TEST_F(CellLab, P6_Mitosis_shortens_telomeres_and_refuses_at_zero) {
    CAPTURE_BEGIN();
    {
        Cell::setCopyErrorRate(0);
        Cell p("Moa", 2, 2, 2, 4, "ABab");
        Cell* d1 = p.mitosis(); p.print(); d1->print();
        Cell* d2 = p.mitosis(); p.print(); d2->print();
        Cell* d3 = p.mitosis(); d3->print();
        cout << Cell::getRefusedDivisions() << " " << Cell::getTotalCreated() << " " << Cell::getSeed() << "\n";
        delete d1; delete d2; delete d3;
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("Moa | Ploidy: 2 | Chromosomes: 2 | Genes/Chr: 2 | Telomeres: 2,2 | Dominant: 2/2 (1.00) | Valid: YES | Genome: AB-ab\n"
        "Moa | Ploidy: 2 | Chromosomes: 2 | Genes/Chr: 2 | Telomeres: 2,2 | Dominant: 2/2 (1.00) | Valid: YES | Genome: AB-ab\n"
        "Moa | Ploidy: 2 | Chromosomes: 2 | Genes/Chr: 2 | Telomeres: 0,0 | Dominant: 2/2 (1.00) | Valid: YES | Genome: AB-ab\n"
        "Moa | Ploidy: 2 | Chromosomes: 2 | Genes/Chr: 2 | Telomeres: 0,0 | Dominant: 2/2 (1.00) | Valid: YES | Genome: AB-ab\n"
        "UNKNOWN | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "1 3 6566661184467396264\n",
        out);
}

// P7: Mitosis copy errors: local seed vs shared seed
// checks: mitosis(seed, bp) does not touch the shared seed; mitosis() with the same seed gives the same daughter; parent genome never changes; shared seed advances 12 draws
TEST_F(CellLab, P7_Mitosis_copy_errors_local_seed_vs_shared_seed) {
    CAPTURE_BEGIN();
    {
        Cell::setCopyErrorRate(2500);
        Cell p("Moa", 2, 4, 3, 20, "ABCabcDEFdef");
        Cell* d = p.mitosis(42, 2500);
        p.print(); d->print();
        cout << Cell::getTotalMutations() << " " << Cell::getSeed() << "\n";
        Cell::setSeed(42);
        Cell* e = p.mitosis();
        e->print();
        cout << Cell::getTotalMutations() << " " << (Cell::getSeed() == lcg(42, 12)) << "\n";
        delete d; delete e;
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("Moa | Ploidy: 2 | Chromosomes: 4 | Genes/Chr: 3 | Telomeres: 18,18,18,18 | Dominant: 6/6 (1.00) | Valid: YES | Genome: ABC-abc-DEF-def\n"
        "Moa | Ploidy: 2 | Chromosomes: 4 | Genes/Chr: 3 | Telomeres: 18,18,18,18 | Dominant: 5/6 (0.83) | Valid: YES | Genome: ABC-abc-dEF-deF\n"
        "2 0\n"
        "Moa | Ploidy: 2 | Chromosomes: 4 | Genes/Chr: 3 | Telomeres: 16,16,16,16 | Dominant: 5/6 (0.83) | Valid: YES | Genome: ABC-abc-dEF-deF\n"
        "4 1\n",
        out);
}

// P8: irradiate and call chaining (this)
// checks: irradiate returns *this; the (bp) overload uses the shared seed, the (bp, seed) overload does not; bp 10000 flips every gene, bp 0 flips none
TEST_F(CellLab, P8_irradiate_and_call_chaining_this) {
    CAPTURE_BEGIN();
    {
        Cell::setSeed(5);
        Cell c("Moa", 2, 2, 4, "ABCDabcd");
        c.print();
        c.irradiate(5000, 99).irradiate(10000).irradiate(0);
        c.print();
        cout << Cell::getTotalMutations() << " " << (Cell::getSeed() == lcg(5, 16)) << "\n";
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("Moa | Ploidy: 2 | Chromosomes: 2 | Genes/Chr: 4 | Telomeres: 12,12 | Dominant: 4/4 (1.00) | Valid: YES | Genome: ABCD-abcd\n"
        "Moa | Ploidy: 2 | Chromosomes: 2 | Genes/Chr: 4 | Telomeres: 12,12 | Dominant: 3/4 (0.75) | Valid: YES | Genome: AbCd-aBcd\n"
        "13 1\n",
        out);
}

// P9: Meiosis: four haploid gametes
// checks: four gametes, crossover + independent assortment from the LCG, no temporary Cell objects (created count), telomere -1, parent unchanged
TEST_F(CellLab, P9_Meiosis_four_haploid_gametes) {
    CAPTURE_BEGIN();
    {
        Cell::setCopyErrorRate(0);
        Cell::setSeed(2024);
        Cell q("Moa", 2, 4, 3, 10, "AbCabcDeFdEF");
        int n;
        Cell** g = q.meiosis(n);
        cout << n << "\n";
        for (int i = 0; i < n; i++) g[i]->print();
        cout << Cell::getTotalCrossovers() << " " << Cell::getTotalCreated() << " " << (Cell::getSeed() == lcg(2024, 2 + 2 + 4 * 2 * 3)) << "\n";
        q.print();
        freeG(g, n);
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("4\n"
        "Moa | Ploidy: 1 | Chromosomes: 2 | Genes/Chr: 3 | Telomeres: 9,9 | Dominant: 3/6 (0.50) | Valid: YES | Genome: Abc-DeF\n"
        "Moa | Ploidy: 1 | Chromosomes: 2 | Genes/Chr: 3 | Telomeres: 9,9 | Dominant: 3/6 (0.50) | Valid: YES | Genome: Abc-DeF\n"
        "Moa | Ploidy: 1 | Chromosomes: 2 | Genes/Chr: 3 | Telomeres: 9,9 | Dominant: 3/6 (0.50) | Valid: YES | Genome: abC-dEF\n"
        "Moa | Ploidy: 1 | Chromosomes: 2 | Genes/Chr: 3 | Telomeres: 9,9 | Dominant: 3/6 (0.50) | Valid: YES | Genome: abC-dEF\n"
        "2 5 1\n"
        "Moa | Ploidy: 2 | Chromosomes: 4 | Genes/Chr: 3 | Telomeres: 10,10,10,10 | Dominant: 5/6 (0.83) | Valid: YES | Genome: AbC-abc-DeF-dEF\n",
        out);
}

// P10: Fertilization
// checks: gametes combine into a diploid zygote (father chromosome 2p, mother chromosome 2p+1), telomeres inherited, incompatible gametes give an invalid cell, nothing leaks
TEST_F(CellLab, P10_Fertilization) {
    CAPTURE_BEGIN();
    {
        Cell a("Moa", 1, 2, 3, 8, "ABcdeF"), b("Moa", 1, 2, 3, 6, "abCdEf");
        Cell* z = Cell::fertilize(a, b);
        z->print();
        for (int c = 0; c < 4; c++) cout << z->getTelomere(c) << " ";
        cout << "\n";
        Cell x("Moa", 1, 2, 3, 8, "xyzabc");
        Cell* w = Cell::fertilize(a, x);
        w->print();
        cout << Cell::getLiveCells() << "\n";
        delete z; delete w;
    }
    std::string out = CAPTURE_END();
    EXPECT_EQ("Moa | Ploidy: 2 | Chromosomes: 4 | Genes/Chr: 3 | Telomeres: 8,6,8,6 | Dominant: 5/6 (0.83) | Valid: YES | Genome: ABc-abC-deF-dEf\n"
        "8 6 8 6 \n"
        "Moa | Ploidy: 0 | Chromosomes: 0 | Genes/Chr: 0 | Telomeres: - | Dominant: 0/0 (0.00) | Valid: NO | Genome: N/A\n"
        "5\n",
        out);
}
