// ----------------------------------------------------------------------------
// How to use (Visual Studio 2019 / 2022):
//   1. Create a "Google Test" project (or add the NuGet package
//      Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn to a Console project).
//   2. Put THIS file and the student's Q1.cpp in the same folder.
//   3. Add ONLY this test file to the project. Do NOT add Q1.cpp as a project source:
//      this file already does  #include "Q1.cpp"  and the classes would be defined twice.
//   4. Build and run (Test Explorer or the console). C++ language standard: C++17.
// Memory leaks are detected through the live() counters of the six classes.
// ----------------------------------------------------------------------------
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include "Q1.cpp"

namespace {

uint32_t t_gray(uint32_t c) { return c ^ (c >> 1); }
[[maybe_unused]] uint32_t t_scramble(uint32_t x) { x ^= (x << 13); x ^= (x >> 17); x ^= (x << 5); return x; }
[[maybe_unused]] uint32_t t_rotl(uint32_t x, int r) { return (x << r) | (x >> (32 - r)); }

// a gray lock whose code is exactly `code`
[[maybe_unused]] Lock* t_grayLock(const char* name, uint32_t code, int base) { return new Lock(name, 'G', t_gray(code), 0, base); }

}  // namespace

class QuietLedger : public ::testing::Test {
protected:
    int g0, m0, l0, v0, c0, o0;
    void SetUp() override {
        Operation::resetHeat();
        g0 = Gadget::live(); m0 = Member::live(); l0 = Lock::live();
        v0 = Vault::live();  c0 = Crew::live();   o0 = Operation::live();
    }
    void TearDown() override {
        EXPECT_EQ(g0, Gadget::live())    << "Gadget objects were leaked (or deleted twice)";
        EXPECT_EQ(m0, Member::live())    << "Member objects were leaked (or deleted twice)";
        EXPECT_EQ(l0, Lock::live())      << "Lock objects were leaked (or deleted twice)";
        EXPECT_EQ(v0, Vault::live())     << "Vault objects were leaked (or deleted twice)";
        EXPECT_EQ(c0, Crew::live())      << "Crew objects were leaked (or deleted twice)";
        EXPECT_EQ(o0, Operation::live()) << "Operation objects were leaked (or deleted twice)";
        Operation::resetHeat();
    }
};

#define CAPTURE_BEGIN() testing::internal::CaptureStdout()
#define CAPTURE_END()   testing::internal::GetCapturedStdout()


// =============================== Gadget =====================================

TEST_F(QuietLedger, Gadget_StoresValuesAndItsOwnCopyOfTheName) {
    char buf[16];
    std::strcpy(buf, "Drill");
    Gadget g(buf, 'M', 40, 3);
    std::strcpy(buf, "XXXXX");
    EXPECT_STREQ("Drill", g.getName());
    EXPECT_EQ('M', g.getType());
    EXPECT_EQ(40, g.getBonus());
    EXPECT_EQ(3, g.getUses());
}

TEST_F(QuietLedger, Gadget_HelpsOnlyWithUsesAndMatchingType) {
    Gadget m("m", 'M', 10, 2), star("s", '*', 10, 1), empty("e", 'G', 10, 0);
    EXPECT_TRUE(m.helps('M'));
    EXPECT_FALSE(m.helps('G'));
    EXPECT_FALSE(m.helps('X'));
    EXPECT_TRUE(star.helps('M'));
    EXPECT_TRUE(star.helps('G'));
    EXPECT_TRUE(star.helps('X'));
    EXPECT_FALSE(empty.helps('G'));
}

TEST_F(QuietLedger, Gadget_EffectiveBonusHalvesStarAgainstX) {
    Gadget star("s", '*', 51, 1), g("g", 'G', 40, 1);
    EXPECT_EQ(51, star.effectiveBonus('M'));
    EXPECT_EQ(51, star.effectiveBonus('G'));
    EXPECT_EQ(25, star.effectiveBonus('X'));
    EXPECT_EQ(40, g.effectiveBonus('G'));
}

TEST_F(QuietLedger, Gadget_ConsumeNeverGoesBelowZero) {
    Gadget g("g", 'M', 10, 2);
    g.consume();
    EXPECT_EQ(1, g.getUses());
    g.consume();
    g.consume();
    g.consume();
    EXPECT_EQ(0, g.getUses());
}

TEST_F(QuietLedger, Gadget_CopyIsIndependentAndLiveCounterWorks) {
    int before = Gadget::live();
    {
        Gadget a("A", 'M', 10, 2);
        Gadget b(a);
        EXPECT_EQ(before + 2, Gadget::live());
        b.consume();
        EXPECT_EQ(2, a.getUses());
        EXPECT_EQ(1, b.getUses());
        EXPECT_STREQ("A", b.getName());
        EXPECT_NE(a.getName(), b.getName());  // different buffers: deep copy of the name
    }
    EXPECT_EQ(before, Gadget::live());
}

// ================================ Member ====================================

TEST_F(QuietLedger, Member_NewMemberStartsEmpty) {
    Member m("Ann", 5);
    EXPECT_STREQ("Ann", m.getName());
    EXPECT_EQ(5, m.getSkill());
    EXPECT_EQ(0, m.getJobs());
    EXPECT_EQ(0, m.gadgetCount());
    EXPECT_EQ(5, m.effectiveSkill());
}

TEST_F(QuietLedger, Member_TimeAndWorkFollowTheSpecExample) {
    Lock lk("lk", 'G', 13, 0, 20);               // a gray lock, base time 20
    Member m("Ann", 5);
    EXPECT_EQ(0, m.addGadget("Pick", 'G', 50, 1));
    EXPECT_EQ(50, m.bonusFor('G'));
    EXPECT_EQ(0, m.bonusFor('M'));
    EXPECT_EQ(10, m.timeFor(lk));                // ceil(2000 / (100 + 50 + 50))
    EXPECT_EQ(10, m.work(lk));
    EXPECT_FALSE(lk.isCracked());                // working does not crack
    EXPECT_EQ(1, m.getJobs());
    EXPECT_EQ(0, m.getGadget(0)->getUses());     // the gadget was used up
    EXPECT_EQ(14, m.timeFor(lk));                // ceil(2000 / 150)
}

TEST_F(QuietLedger, Member_TiredMembersGetSlower) {
    Lock quick("q", 'G', 0, 0, 1);
    Member m("Ann", 5);
    for (int i = 0; i < 2; i++) m.work(quick);
    EXPECT_EQ(5, m.effectiveSkill());
    m.work(quick);                               // 3 jobs
    EXPECT_EQ(4, m.effectiveSkill());
    for (int i = 0; i < 3; i++) m.work(quick);   // 6 jobs
    EXPECT_EQ(3, m.effectiveSkill());
    EXPECT_EQ(5, m.getSkill());                  // the original skill never changes
    Member weak("Bob", 1);
    for (int i = 0; i < 9; i++) weak.work(quick);
    EXPECT_EQ(1, weak.effectiveSkill());         // never below 1
}

TEST_F(QuietLedger, Member_BonusIsCappedAt100) {
    Member m("Ann", 5);
    m.addGadget("a", 'M', 60, 1);
    m.addGadget("b", 'M', 60, 1);
    m.addGadget("c", '*', 40, 1);
    EXPECT_EQ(100, m.bonusFor('M'));
    EXPECT_EQ(20, m.bonusFor('X'));              // only the '*' gadget, halved
}

TEST_F(QuietLedger, Member_HoldsAtMostFiveGadgets) {
    Member m("Ann", 5);
    for (int i = 0; i < 5; i++) EXPECT_EQ(i, m.addGadget("g", 'M', 10, 1));
    EXPECT_EQ(-1, m.addGadget("g", 'M', 10, 1));
    Gadget extra("x", 'G', 10, 1);
    EXPECT_EQ(-1, m.addGadget(extra));
    EXPECT_EQ(5, m.gadgetCount());
    EXPECT_TRUE(m.getGadget(5) == nullptr);
    EXPECT_TRUE(m.getGadget(-1) == nullptr);
    EXPECT_TRUE(m.getGadget(4) != nullptr);
}

TEST_F(QuietLedger, Member_AddGadgetFromGadgetStoresItsOwnCopy) {
    Member m("Ann", 5);
    Gadget g("Pick", 'G', 30, 2);
    EXPECT_EQ(0, m.addGadget(g));
    g.consume();
    EXPECT_EQ(2, m.getGadget(0)->getUses());
    EXPECT_NE(&g, m.getGadget(0));
}

TEST_F(QuietLedger, Member_CopyIsDeepAndKeepsJobs) {
    Lock lk("lk", 'G', 13, 0, 5);
    int before = Gadget::live();
    Member a("Ann", 5);
    a.addGadget("g1", 'G', 20, 3);
    a.addGadget("g2", 'M', 10, 1);
    a.work(lk);
    Member b(a);
    EXPECT_EQ(before + 4, Gadget::live());
    EXPECT_EQ(1, b.getJobs());
    EXPECT_EQ(2, b.gadgetCount());
    b.work(lk);
    EXPECT_EQ(1, a.getJobs());
    EXPECT_EQ(2, a.getGadget(0)->getUses());     // the copy used its own gadget
    EXPECT_EQ(1, b.getGadget(0)->getUses());
    EXPECT_NE(a.getGadget(0), b.getGadget(0));
}

TEST_F(QuietLedger, Member_CrackedLockCostsNoTime) {
    Lock lk("lk", 'G', 13, 0, 20);
    lk.crack();
    Member m("Ann", 5);
    EXPECT_EQ(0, m.timeFor(lk));
}

// ================================= Lock =====================================

TEST_F(QuietLedger, Lock_DefaultConstructor) {
    Lock lk;
    EXPECT_STREQ("lock", lk.getName());
    EXPECT_EQ('G', lk.getType());
    EXPECT_EQ(1, lk.getBaseTime());
    EXPECT_FALSE(lk.isCracked());
    EXPECT_EQ(0u, lk.getCode());
    EXPECT_EQ(0, lk.getRefCount());
}

TEST_F(QuietLedger, Lock_ModularCode) {
    Lock lk("m", 'M', 3, 7, 10);
    EXPECT_EQ(10, lk.getBaseTime());
    EXPECT_EQ(0u, lk.getCode());                 // not cracked yet
    EXPECT_EQ(5u, lk.crack());
    EXPECT_TRUE(lk.isCracked());
    EXPECT_EQ(5u, lk.getCode());
    Lock big("big", 'M', 123456789u, 2000000000u, 5);
    EXPECT_EQ(109890109u, big.crack());
}

TEST_F(QuietLedger, Lock_GrayCode) {
    Lock lk("g", 'G', 13, 0, 4);
    EXPECT_EQ(9u, lk.crack());
}

TEST_F(QuietLedger, Lock_XorshiftCode) {
    Lock one("x1", 'X', 270369u, 1u, 4);
    EXPECT_EQ(1u, one.crack());
    Lock three("x3", 'X', 3472693697u, 3u, 4);   // three rounds applied to 5
    EXPECT_EQ(5u, three.crack());
}

TEST_F(QuietLedger, Lock_CrackingIsIdempotentAndRaisesHeatOnce) {
    Lock lk("g", 'G', 13, 0, 4);
    EXPECT_EQ(0, Operation::getHeat());
    EXPECT_EQ(9u, lk.crack());
    EXPECT_EQ(1, Operation::getHeat());
    EXPECT_EQ(9u, lk.crack());
    EXPECT_EQ(1, Operation::getHeat());
}

TEST_F(QuietLedger, Lock_HardenedLocksCostMoreHeat) {
    Lock plain("p", 'X', 1234703271u, 49999u, 4);    // k = 49999: not hardened
    Lock hard("h", 'X', 913710193u, 50000u, 4);      // k = 50000: hardened
    EXPECT_EQ(3735928559u, plain.crack());
    EXPECT_EQ(1, Operation::getHeat());
    EXPECT_EQ(3735928559u, hard.crack());
    EXPECT_EQ(4, Operation::getHeat());
}

TEST_F(QuietLedger, Lock_VerifyChecksACandidateWithoutCracking) {
    Lock lk("m", 'M', 3, 7, 10);
    EXPECT_TRUE(lk.verify(5));
    EXPECT_FALSE(lk.verify(4));
    EXPECT_FALSE(lk.isCracked());
    Lock g("g", 'G', 13, 0, 1);
    EXPECT_TRUE(g.verify(9));
    EXPECT_FALSE(g.verify(13));
}

TEST_F(QuietLedger, Lock_ReferenceCounting) {
    Lock* lk = new Lock("lk", 'G', 0, 0, 1);
    EXPECT_EQ(0, lk->getRefCount());
    lk->addRef();
    lk->addRef();
    EXPECT_EQ(2, lk->getRefCount());
    EXPECT_FALSE(lk->release());
    EXPECT_EQ(1, lk->getRefCount());
    EXPECT_TRUE(lk->release());
    delete lk;
}

// ================================= Vault ====================================

TEST_F(QuietLedger, Vault_LootIsAModularPower) {
    Vault v("v", 3, 5, 7, 0, 10);
    EXPECT_EQ(5u, v.loot());                     // 3^5 = 243 = 34 * 7 + 5
    Vault w("w", 2, 10, 1000, 0, 10);
    EXPECT_EQ(24u, w.loot());
    EXPECT_STREQ("v", v.getName());
    EXPECT_EQ(0, v.windowStart());
    EXPECT_EQ(10, v.windowEnd());
}

TEST_F(QuietLedger, Vault_AddFindAndCapacityDoubling) {
    Vault v("v", 3, 5, 7, 0, 10);
    EXPECT_EQ(2, v.capacity());
    EXPECT_EQ(0, v.addLock("a", 'G', 1, 0, 3));
    EXPECT_EQ(1, v.addLock("b", 'G', 2, 0, 3));
    EXPECT_EQ(2, v.capacity());
    EXPECT_EQ(2, v.addLock("c", 'M', 3, 7, 3));
    EXPECT_EQ(4, v.capacity());
    EXPECT_EQ(3, v.lockCount());
    EXPECT_EQ(1, v.findLock("b"));
    EXPECT_EQ(-1, v.findLock("zzz"));
    EXPECT_STREQ("c", v.getLock(2)->getName());
    EXPECT_TRUE(v.getLock(3) == nullptr);
    EXPECT_TRUE(v.getLock(-1) == nullptr);
}

TEST_F(QuietLedger, Vault_AddLockPointerSharesTheLock) {
    Lock* lk = new Lock("lk", 'G', 5, 0, 3);
    Vault v("v", 3, 5, 7, 0, 10);
    EXPECT_EQ(0, v.addLock(lk));
    EXPECT_EQ(1, lk->getRefCount());
    EXPECT_EQ(lk, v.getLock(0));
    EXPECT_EQ(-1, v.addLock(static_cast<Lock*>(nullptr)));
    EXPECT_EQ(1, v.lockCount());
}

TEST_F(QuietLedger, Vault_OpenKeyAndNoise) {
    Vault v("v", 3, 5, 7, 0, 10);
    EXPECT_TRUE(v.isOpen());                     // no locks: open
    EXPECT_EQ(0u, v.key());
    EXPECT_EQ(0, v.noise());
    v.addLock("a", 'M', 3, 7, 3);                // code 5
    v.addLock("b", 'G', 13, 0, 3);               // code 9
    EXPECT_FALSE(v.isOpen());
    EXPECT_EQ(0u, v.key());
    v.getLock(0)->crack();
    EXPECT_EQ(1, v.crackedCount());
    EXPECT_FALSE(v.isOpen());
    v.getLock(1)->crack();
    EXPECT_TRUE(v.isOpen());
    EXPECT_EQ(169u, v.key());                    // rotl(5, 5) ^ 9 = 160 ^ 9
    EXPECT_EQ(4, v.noise());
}

TEST_F(QuietLedger, Vault_CopyHasItsOwnArrayButSharesTheLocks) {
    int before = Lock::live();
    Vault a("v", 3, 5, 7, 0, 10);
    a.addLock("a", 'G', 1, 0, 3);
    a.addLock("b", 'G', 2, 0, 3);
    a.addLock("c", 'G', 3, 0, 3);
    Vault b(a);
    EXPECT_EQ(before + 3, Lock::live());         // no lock was copied
    EXPECT_EQ(3, b.lockCount());
    EXPECT_EQ(a.capacity(), b.capacity());
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(a.getLock(i), b.getLock(i));
        EXPECT_EQ(2, a.getLock(i)->getRefCount());
    }
    a.getLock(1)->crack();
    EXPECT_TRUE(b.getLock(1)->isCracked());      // cracked through one vault, cracked for both
}

TEST_F(QuietLedger, Vault_SharedLockSurvivesUntilTheLastVaultLetsGo) {
    int before = Lock::live();
    Vault* a = new Vault("v", 3, 5, 7, 0, 10);
    a->addLock("a", 'G', 1, 0, 3);
    Vault* b = new Vault(*a);
    delete a;
    EXPECT_EQ(before + 1, Lock::live());         // still alive in b
    EXPECT_STREQ("a", b->getLock(0)->getName());
    EXPECT_EQ(1, b->getLock(0)->getRefCount());
    delete b;
    EXPECT_EQ(before, Lock::live());
}

TEST_F(QuietLedger, Vault_RemoveLockShiftsTheOthers) {
    int before = Lock::live();
    Vault v("v", 3, 5, 7, 0, 10);
    v.addLock("a", 'G', 1, 0, 3);
    v.addLock("b", 'G', 2, 0, 3);
    v.addLock("c", 'G', 3, 0, 3);
    EXPECT_TRUE(v.removeLock(1));
    EXPECT_EQ(2, v.lockCount());
    EXPECT_STREQ("c", v.getLock(1)->getName());
    EXPECT_EQ(before + 2, Lock::live());         // "b" was destroyed (nobody else held it)
    EXPECT_FALSE(v.removeLock(5));
    EXPECT_FALSE(v.removeLock(-1));
    EXPECT_EQ(2, v.lockCount());
}

TEST_F(QuietLedger, Vault_ConstAndNonConstGetLock) {
    Vault v("v", 3, 5, 7, 0, 10);
    v.addLock("a", 'G', 1, 0, 3);
    const Vault& cv = v;
    const Lock* p = cv.getLock(0);
    Lock* q = v.getLock(0);
    EXPECT_EQ(p, q);
}

// ================================== Crew ====================================

TEST_F(QuietLedger, Crew_AddFindAndLimit) {
    Crew c("Alpha");
    EXPECT_STREQ("Alpha", c.getName());
    EXPECT_EQ(0, c.addMember("Ann", 5));
    Member bob("Bob", 3);
    EXPECT_EQ(1, c.addMember(bob));
    EXPECT_EQ(2, c.memberCount());
    EXPECT_EQ(1, c.findMember("Bob"));
    EXPECT_EQ(-1, c.findMember("Zed"));
    EXPECT_TRUE(c.getMember(2) == nullptr);
    EXPECT_NE(&bob, c.getMember(1));             // the crew stores its own copy
    for (int i = 2; i < 20; i++) EXPECT_EQ(i, c.addMember("m", 1));
    EXPECT_EQ(-1, c.addMember("one too many", 1));
    EXPECT_EQ(20, c.memberCount());
}

TEST_F(QuietLedger, Crew_EstimateGivesTheDurationAndChangesNothing) {
    Crew c("Alpha");
    c.addMember("Ann", 5);
    c.addMember("Bob", 3);
    c.getMember(0)->addGadget("Pick", 'M', 20, 2);
    Vault v("V1", 3, 5, 7, 0, 20);
    v.addLock("L1", 'M', 3, 7, 10);
    v.addLock("L2", 'G', 13, 0, 6);
    int members = Member::live(), gadgets = Gadget::live();
    EXPECT_EQ(6, c.estimate(v));                 // Ann: 10 -> ceil(1000/170) = 6, Bob: L2 -> 5
    EXPECT_EQ(6, c.estimate(v));                 // asking twice gives the same answer
    EXPECT_EQ(members, Member::live());
    EXPECT_EQ(gadgets, Gadget::live());
    EXPECT_EQ(0, c.getMember(0)->getJobs());
    EXPECT_EQ(2, c.getMember(0)->getGadget(0)->getUses());
    EXPECT_FALSE(v.getLock(0)->isCracked());
    EXPECT_EQ(0, Operation::getHeat());
}

TEST_F(QuietLedger, Crew_EstimateOfAnEmptyCrewIsMinusOne) {
    Crew c("Nobody");
    Vault v("V1", 3, 5, 7, 0, 20);
    v.addLock("L1", 'G', 13, 0, 6);
    EXPECT_EQ(-1, c.estimate(v));
}

TEST_F(QuietLedger, Crew_ExecuteCracksTheLocksAndReallyWorks) {
    Crew c("Alpha");
    c.addMember("Ann", 5);
    c.addMember("Bob", 3);
    Vault v("V1", 3, 5, 7, 0, 20);
    v.addLock("L1", 'M', 3, 7, 10);
    v.addLock("L2", 'G', 13, 0, 6);
    EXPECT_EQ(7, c.estimate(v));
    EXPECT_EQ(7, c.execute(v));
    EXPECT_TRUE(v.isOpen());
    EXPECT_EQ(5u, v.getLock(0)->getCode());
    EXPECT_EQ(9u, v.getLock(1)->getCode());
    EXPECT_EQ(1, c.getMember(0)->getJobs());     // Ann took the big lock
    EXPECT_EQ(1, c.getMember(1)->getJobs());     // Bob took the small one
    EXPECT_EQ(2, Operation::getHeat());          // two locks cracked
    EXPECT_EQ(1, c.execute(v));                  // nothing left to crack: max(1, 0)
}

TEST_F(QuietLedger, Crew_CopyIsDeep) {
    int members = Member::live();
    Crew a("Alpha");
    a.addMember("Ann", 5);
    a.getMember(0)->addGadget("Pick", 'M', 20, 2);
    Crew b(a);
    EXPECT_EQ(members + 2, Member::live());
    EXPECT_NE(a.getMember(0), b.getMember(0));
    Lock lk("l", 'M', 3, 7, 5);
    b.getMember(0)->work(lk);
    EXPECT_EQ(0, a.getMember(0)->getJobs());
    EXPECT_EQ(2, a.getMember(0)->getGadget(0)->getUses());
}

// =============================== Operation ==================================

TEST_F(QuietLedger, Operation_AddFindAndGetters) {
    Operation op("Dawn");
    EXPECT_STREQ("Dawn", op.getName());
    EXPECT_EQ(0, op.addCrew("Alpha"));
    Crew beta("Beta");
    beta.addMember("Bob", 4);
    EXPECT_EQ(1, op.addCrew(beta));
    EXPECT_EQ(0, op.addVault("V1", 3, 5, 7, 0, 20));
    Vault w("V2", 2, 3, 11, 5, 30);
    w.addLock("lk", 'G', 3, 0, 4);
    EXPECT_EQ(1, op.addVault(w));
    EXPECT_EQ(2, op.crewCount());
    EXPECT_EQ(2, op.vaultCount());
    EXPECT_EQ(1, op.findCrew("Beta"));
    EXPECT_EQ(1, op.findVault("V2"));
    EXPECT_EQ(-1, op.findCrew("Nope"));
    EXPECT_EQ(-1, op.findVault("Nope"));
    EXPECT_TRUE(op.getCrew(2) == nullptr);
    EXPECT_TRUE(op.getVault(-1) == nullptr);
    EXPECT_EQ(1, op.getCrew(1)->memberCount());
    EXPECT_NE(&w, op.getVault(1));
    EXPECT_EQ(w.getLock(0), op.getVault(1)->getLock(0));    // addVault(Vault) shares the locks
    EXPECT_EQ(2, w.getLock(0)->getRefCount());
}

TEST_F(QuietLedger, Operation_SetNameKeepsItsOwnCopy) {
    Operation op("Dawn");
    char buf[16];
    std::strcpy(buf, "Dusk");
    op.setName(buf);
    std::strcpy(buf, "XXXX");
    EXPECT_STREQ("Dusk", op.getName());
}

TEST_F(QuietLedger, Operation_HeatIsClampedBetween0And100) {
    Operation::addHeat(30);
    EXPECT_EQ(30, Operation::getHeat());
    Operation::addHeat(-50);
    EXPECT_EQ(0, Operation::getHeat());
    Operation::addHeat(250);
    EXPECT_EQ(100, Operation::getHeat());
    Operation::resetHeat();
    EXPECT_EQ(0, Operation::getHeat());
}

TEST_F(QuietLedger, Operation_DestroyingLowersHeatByTwo) {
    Operation::addHeat(10);
    {
        Operation op("Dawn");
        EXPECT_EQ(10, Operation::getHeat());
    }
    EXPECT_EQ(8, Operation::getHeat());
    Operation::resetHeat();
    {
        Operation op("Dawn");
    }
    EXPECT_EQ(0, Operation::getHeat());          // never below 0
}

TEST_F(QuietLedger, Operation_BackupPlanSharesLocksAndRaisesHeat) {
    int locks = Lock::live();
    Operation a("Dawn");
    a.addCrew("Alpha");
    a.getCrew(0)->addMember("Ann", 5);
    a.addVault("V1", 3, 5, 7, 0, 20);
    a.getVault(0)->addLock("L1", 'G', 13, 0, 6);
    {
        Operation b(a);
        EXPECT_EQ(3, Operation::getHeat());
        EXPECT_EQ(locks + 1, Lock::live());      // the lock was not copied
        EXPECT_EQ(a.getVault(0)->getLock(0), b.getVault(0)->getLock(0));
        EXPECT_EQ(2, a.getVault(0)->getLock(0)->getRefCount());
        EXPECT_NE(a.getCrew(0), b.getCrew(0));   // crews are deep copies
        EXPECT_NE(a.getCrew(0)->getMember(0), b.getCrew(0)->getMember(0));
        EXPECT_STREQ("Dawn", b.getName());
        b.setName("Dawn (backup)");
        EXPECT_STREQ("Dawn", a.getName());
        a.getVault(0)->getLock(0)->crack();
        EXPECT_TRUE(b.getVault(0)->getLock(0)->isCracked());
    }
    EXPECT_EQ(1, a.getVault(0)->getLock(0)->getRefCount());
    EXPECT_EQ(locks + 1, Lock::live());
}

// A small world used by the next tests:
//   crew Alpha = Ann (skill 5), Bob (skill 3);  vault V1: loot 5, window [0,20), locks M(3,7) base 10 and G(13) base 6.
static void buildSmallWorld(Operation& op) {
    op.addCrew("Alpha");
    op.getCrew(0)->addMember("Ann", 5);
    op.getCrew(0)->addMember("Bob", 3);
    op.addVault("V1", 3, 5, 7, 0, 20);
    op.getVault(0)->addLock("L1", 'M', 3, 7, 10);
    op.getVault(0)->addLock("L2", 'G', 13, 0, 6);
}

TEST_F(QuietLedger, Operation_PrintPlan) {
    Operation op("Dawn");
    buildSmallWorld(op);
    CAPTURE_BEGIN();
    op.printPlan();
    std::string out = CAPTURE_END();
    EXPECT_EQ("PLAN Dawn\nCrew Alpha -> V1[0,7) loot=5\nUnassigned: none\n", out);
    EXPECT_EQ(0, Operation::getHeat());          // planning changes nothing
    EXPECT_FALSE(op.getVault(0)->getLock(0)->isCracked());
}

TEST_F(QuietLedger, Operation_ExecutePrintsAndReturnsTheTotalLoot) {
    Operation op("Dawn");
    buildSmallWorld(op);
    EXPECT_FALSE(op.isDone());
    EXPECT_EQ(0u, op.totalLoot());
    CAPTURE_BEGIN();
    uint64_t total = op.execute();
    std::string out = CAPTURE_END();
    EXPECT_EQ("[Alpha] V1 start=0 finish=7 OK heat=2\nTOTAL LOOT 5\nHEAT 2\n", out);
    EXPECT_EQ(5u, total);
    EXPECT_EQ(5u, op.totalLoot());
    EXPECT_TRUE(op.isDone());
    EXPECT_EQ(2, Operation::getHeat());
}

TEST_F(QuietLedger, Operation_ExecuteTwiceDoesNothingTheSecondTime) {
    Operation op("Dawn");
    buildSmallWorld(op);
    CAPTURE_BEGIN();
    op.execute();
    CAPTURE_END();
    CAPTURE_BEGIN();
    uint64_t again = op.execute();
    std::string out = CAPTURE_END();
    EXPECT_EQ("", out);
    EXPECT_EQ(5u, again);
    EXPECT_EQ(2, Operation::getHeat());
}

TEST_F(QuietLedger, Operation_ACrewWithNothingToDoIsPrintedAsIdle) {
    Operation op("Quiet");
    op.addCrew("A");
    op.getCrew(0)->addMember("Ann", 5);
    op.addCrew("B");
    op.getCrew(1)->addMember("Bob", 5);
    op.addVault("V1", 3, 5, 7, 0, 20);
    op.getVault(0)->addLock("L1", 'G', 13, 0, 10);
    CAPTURE_BEGIN();
    op.printPlan();
    std::string out = CAPTURE_END();
    EXPECT_EQ("PLAN Quiet\nCrew A -> V1[0,7) loot=5\nCrew B -> (idle) loot=0\nUnassigned: none\n", out);
}

TEST_F(QuietLedger, Operation_EmptyOperationPrintsAnEmptyPlan) {
    Operation op("Nothing");
    CAPTURE_BEGIN();
    op.printPlan();
    std::string out = CAPTURE_END();
    EXPECT_EQ("PLAN Nothing\nUnassigned: none\n", out);
    CAPTURE_BEGIN();
    uint64_t total = op.execute();
    out = CAPTURE_END();
    EXPECT_EQ("TOTAL LOOT 0\nHEAT 0\n", out);
    EXPECT_EQ(0u, total);
}

TEST_F(QuietLedger, Operation_ManyCopiesAndDestroysLeaveNothingBehind) {
    for (int round = 0; round < 50; round++) {
        Operation a("Dawn");
        buildSmallWorld(a);
        Operation b(a);
        Operation c(b);
        b.getVault(0)->removeLock(0);
        CAPTURE_BEGIN();
        a.execute();
        CAPTURE_END();
    }
    // heat is global and accumulates, so only the object counters are checked (by the fixture)
}
