#include <iostream>
#include <cstring>
#include <cstdint>

typedef uint32_t u32;
typedef uint64_t u64;

class Gadget {
private:
    char* name;
    char type;
    int bonus;
    int uses;
    static int liveCount;

    Gadget& operator=(const Gadget&);
public:
    Gadget(const char* name, char type, int bonus, int uses);
    Gadget(const Gadget& other);
    ~Gadget();

    const char* getName() const;
    char getType() const;
    int getBonus() const;
    int getUses() const;

    bool helps(char lockType) const;
    int effectiveBonus(char lockType) const;
    void consume();

    static int live();
};

int Gadget::liveCount = 0;
Gadget::Gadget(const char* name, char type, int bonus, int uses): name(new char[std::strlen(name) + 1]),type(type),bonus(bonus),uses(uses){
    std::strcpy(this->name, name);
    liveCount++;
}
Gadget::~Gadget(){
    delete[] name;
    liveCount--;
}
Gadget::Gadget(const Gadget& other): name(new char[std::strlen(other.name) + 1]),type(other.type),bonus(other.bonus),uses(other.uses){
    std::strcpy(name, other.name);
    liveCount++;
}
const char* Gadget::getName() const{
    return name;
}
char Gadget::getType() const{
    return type;
}
int Gadget::getBonus() const{
    return bonus;
}
int Gadget::getUses() const{
    return uses;
}
bool Gadget::helps(char lockType) const{
    return uses > 0 && (type == lockType || type == '*');
}
int Gadget::effectiveBonus(char lockType) const{
    if (type == '*' && lockType == 'X'){
        return bonus / 2;
    }
    return bonus;
}

void Gadget::consume(){
    if (uses > 0){
        uses--;
    }
}
int Gadget::live(){
    return liveCount;
}

class Operation;
class Lock {
public:
    u32 p1;
    u32 p2;

private:
    char* name;
    char type;
    int baseTime;
    u32 code;
    bool cracked;
    int refCount;
    static int liveCount;

    Lock(const Lock&);
    Lock& operator=(const Lock&);
public:
    Lock();
    Lock(const char* name, char type, u32 p1, u32 p2, int baseTime);
    ~Lock();

    const char* getName() const;
    char getType() const;
    int getBaseTime() const;
    bool isCracked() const;
    u32 getCode() const;
    bool verify(u32 candidate) const;
    u32 crack();
    int getRefCount() const;
    void addRef();
    bool release();
    static int live();
};

int Lock::liveCount = 0;

Lock::Lock(){
    name = new char[5];
    name[0] = 'l';
    name[1] = 'o';
    name[2] = 'c';
    name[3] = 'k';
    name[4] = '\0';

    type = 'G';
    p1 = 0;
    p2 = 0;
    baseTime = 1;
    code = 0;
    cracked = false;
    refCount = 0;
    liveCount++;
}

Lock::Lock(const char* name, char type, u32 p1, u32 p2, int baseTime){
    this->name = new char[strlen(name) + 1];
    strcpy(this->name, name);
    this->type = type;
    this->p1 = p1;
    this->p2 = p2;
    this->baseTime = baseTime;
    code = 0;
    cracked = false;
    refCount = 0;
    liveCount++;
}
Lock::~Lock(){
    delete[] name;
    liveCount--;
}

const char* Lock::getName() const{
    return name;
}
char Lock::getType() const{
    return type;
}
int Lock::getBaseTime() const{
    return baseTime;
}
bool Lock::isCracked() const{
    return cracked;
}
u32 Lock::getCode() const{
    if (cracked){
        return code;
    }
    return 0;
}
int Lock::getRefCount() const{
    return refCount;
}
void Lock::addRef(){
    refCount++;
}
bool Lock::release(){
    refCount--;
    return refCount <= 0;
}
int Lock::live(){
    return liveCount;
}

bool Lock::verify(u32 candidate) const{
    if (type == 'M'){
        if (p2 == 0 || candidate == 0 || candidate >= p2){
            return false;
        }
        return ((u64)p1 * candidate) % p2 == 1;
    }
    if (type == 'G'){
        return (candidate ^ (candidate >> 1)) == p1;
    }
    if (type == 'X'){
        u32 x = candidate;
        for (u32 i = 0; i < p2; i++){
            x = x ^ (x << 13);
            x = x ^ (x >> 17);
            x = x ^ (x << 5);
        }
        return x == p1;
    }
    return false;
}

class Member {
private:
    char* name;
    int skill;
    int jobs;
    Gadget** gadgets;
    int count;
    int capacity;
    static int liveCount;
    Member& operator=(const Member&);
public:
    Member(const char* name, int skill);
    Member(const Member& other);
    ~Member();
    const char* getName() const;
    int getSkill() const;
    int getJobs() const;
    int gadgetCount() const;
    const Gadget* getGadget(int index) const;
    int addGadget(const Gadget& gadget);
    int addGadget(const char* name, char type, int bonus, int uses);
    int effectiveSkill() const;
    int bonusFor(char lockType) const;
    int timeFor(const Lock& lock) const;
    int work(const Lock& lock);
    static int live();
};
int Member::liveCount = 0;
Member::Member(const char* name, int skill): name(new char[std::strlen(name) + 1]),skill(skill),jobs(0),gadgets(new Gadget* [2]),count(0),capacity(2){
    std::strcpy(this->name, name);
    gadgets[0] = nullptr;
    gadgets[1] = nullptr;
    liveCount++;
}
Member::Member(const Member& other): name(new char[std::strlen(other.name) + 1]),skill(other.skill),jobs(other.jobs),gadgets(new Gadget* [other.capacity]),count(other.count),capacity(other.capacity){
    std::strcpy(name, other.name);
    for (int i = 0; i < capacity; i++){
        gadgets[i] = nullptr;
    }
    for (int i = 0; i < count; i++){
        gadgets[i] = new Gadget(*other.gadgets[i]);
    }
    liveCount++;
}
Member::~Member(){
    for (int i = 0; i < count; i++){
        delete gadgets[i];
    }
    delete[] gadgets;
    delete[] name;
    liveCount--;
}
int Member::live(){
    return liveCount;
}
const char* Member::getName() const{
    return name;
}
int Member::getSkill() const{
    return skill;
}
int Member::getJobs() const{
    return jobs;
}
int Member::gadgetCount() const{
    return count;
}
const Gadget* Member::getGadget(int index) const{
    if (index < 0 || index >= count){
        return nullptr;
    }
    return gadgets[index];
}
int Member::addGadget(const Gadget& gadget){
    if (count >= 5){
        return -1;
    }
    if (count == capacity){
        int newCapacity = capacity * 2;
        Gadget** newGadgets = new Gadget * [newCapacity];
        for (int i = 0; i < count; i++){
            newGadgets[i] = gadgets[i];
        }
        for (int i = count; i < newCapacity; i++){
            newGadgets[i] = nullptr;
        }
        delete[] gadgets;
        gadgets = newGadgets;
        capacity = newCapacity;
    }
    gadgets[count] = new Gadget(gadget);
    int index = count;
    count++;
    return index;
}
int Member::addGadget(const char* name, char type, int bonus, int uses){
    Gadget gadget(name, type, bonus, uses);
    return addGadget(gadget);
}

int Member::effectiveSkill() const{
    int result = skill - jobs / 3;
    if (result < 1){
        result = 1;
    }
    return result;
}

int Member::bonusFor(char lockType) const{
    int total = 0;
    for (int i = 0; i < count; i++){
        if (gadgets[i]->helps(lockType)){
            total += gadgets[i]->effectiveBonus(lockType);
        }
    }
    if (total > 100){
        total = 100;
    }
    return total;
}
int Member::timeFor(const Lock& lock) const{
    if (lock.isCracked()){
        return 0;
    }
    int skillValue = effectiveSkill();
    int bonusValue = bonusFor(lock.getType());
    int denominator = 100 + 10 * skillValue + bonusValue;
    int numerator = lock.getBaseTime() * 100;
    return (numerator + denominator - 1) / denominator;
}
int Member::work(const Lock& lock){
    int time = timeFor(lock);
    for (int i = 0; i < count; i++){
        if (gadgets[i]->helps(lock.getType())){
            gadgets[i]->consume();
        }
    }
    jobs++;
    return time;
}

class Vault {
private:
    char* name;
    u32 baseValue;
    u64 exponent;
    u32 modulus;
    int startTime;
    int endTime;
    Lock** locks;
    int count;
    int lockCapacity;
    static int liveCount;
    Vault& operator=(const Vault&);
public:
    Vault(const char* name, u32 base, u64 exp, u32 m,int windowStart, int windowEnd);
    Vault(const Vault& other);
    ~Vault();
    const char* getName() const;
    int windowStart() const;
    int windowEnd() const;
    int addLock(Lock* lock);
    int addLock(const char* name, char type, u32 p1, u32 p2,int baseTime);
    bool removeLock(int index);
    int findLock(const char* name) const;
    int lockCount() const;
    int capacity() const;
    int crackedCount() const;
    const Lock* getLock(int index) const;
    Lock* getLock(int index);
    bool isOpen() const;
    u32 key() const;
    int noise() const;
    u32 loot() const;
    static int live();
};
int Vault::liveCount = 0;
Vault::Vault(const char* name, u32 base, u64 exp, u32 m,int windowStart, int windowEnd): name(new char[std::strlen(name) + 1]),baseValue(base),exponent(exp),modulus(m),startTime(windowStart),endTime(windowEnd),locks(new Lock* [2]),count(0),lockCapacity(2){
    std::strcpy(this->name, name);
    locks[0] = nullptr;
    locks[1] = nullptr;
    liveCount++;
}
Vault::Vault(const Vault& other): name(new char[std::strlen(other.name) + 1]),baseValue(other.baseValue),exponent(other.exponent),modulus(other.modulus),startTime(other.startTime),endTime(other.endTime),locks(new Lock* [other.lockCapacity]),count(other.count),lockCapacity(other.lockCapacity){
    std::strcpy(name, other.name);
    for (int i = 0; i < lockCapacity; i++){
        locks[i] = nullptr;
    }
    for (int i = 0; i < count; i++){
        locks[i] = other.locks[i];
        locks[i]->addRef();
    }
    liveCount++;
}
Vault::~Vault(){
    for (int i = 0; i < count; i++){
        if (locks[i]->release()){
            delete locks[i];
        }
    }
    delete[] locks;
    delete[] name;
    liveCount--;
}
const char* Vault::getName() const{
    return name;
}
int Vault::windowStart() const{
    return startTime;
}
int Vault::windowEnd() const{
    return endTime;
}
int Vault::live(){
    return liveCount;
}
int Vault::addLock(Lock* lock){
    if (lock == nullptr || count >= 20){
        return -1;
    }
    for (int i = 0; i < count; i++){
        if (locks[i] == lock){
            return -1;
        }
    }
    if (count == lockCapacity){
        int newCapacity = lockCapacity * 2;
        if (newCapacity > 20){
            newCapacity = 20;
        }
        Lock** newLocks = new Lock * [newCapacity];
        for (int i = 0; i < count; i++){
            newLocks[i] = locks[i];
        }
        for (int i = count; i < newCapacity; i++){
            newLocks[i] = nullptr;
        }
        delete[] locks;
        locks = newLocks;
        lockCapacity = newCapacity;
    }
    locks[count] = lock;
    lock->addRef();
    int index = count;
    count++;
    return index;
}
int Vault::addLock(const char* name, char type, u32 p1, u32 p2,int baseTime) {
    if (count >= 20){
        return -1;
    }
    Lock* lock = new Lock(name, type, p1, p2, baseTime);
    int index = addLock(lock);
    if (index == -1){
        delete lock;
    }
    return index;
}
bool Vault::removeLock(int index){
    if (index < 0 || index >= count){
        return false;
    }
    Lock* lock = locks[index];
    if (lock->release()){
        delete lock;
    }
    for (int i = index; i < count - 1; i++){
        locks[i] = locks[i + 1];
    }
    count--;
    locks[count] = nullptr;
    return true;
}
int Vault::findLock(const char* name) const{
    if (name == nullptr){
        return -1;
    }
    for (int i = 0; i < count; i++){
        if (std::strcmp(locks[i]->getName(), name) == 0){
            return i;
        }
    }
    return -1;
}
int Vault::lockCount() const{
    return count;
}
int Vault::capacity() const{
    return lockCapacity;
}
int Vault::crackedCount() const{
    int result = 0;
    for (int i = 0; i < count; i++){
        if (locks[i]->isCracked()){
            result++;
        }
    }
    return result;
}
const Lock* Vault::getLock(int index) const{
    if (index < 0 || index >= count){
        return nullptr;
    }
    return locks[index];
}
Lock* Vault::getLock(int index){
    if (index < 0 || index >= count){
        return nullptr;
    }
    return locks[index];
}
bool Vault::isOpen() const{
    for (int i = 0; i < count; i++){
        if (!locks[i]->isCracked()){
            return false;
        }
    }
    return true;
}
u32 Vault::key() const{
    if (!isOpen()){
        return 0;
    }
    u32 k = 0;
    for (int i = 0; i < count; i++){
        k = (k << 5) | (k >> 27);
        k = k ^ locks[i]->getCode();
    }
    return k;
}
int Vault::noise() const{
    u32 value = key();
    int result = 0;
    while (value > 0){
        result += value & 1u;
        value = value >> 1;
    }
    return result;
}
u32 Vault::loot() const{
    if (modulus == 0){
        return 0;
    }
    u32 result = 1 % modulus;
    u32 factor = baseValue % modulus;
    u64 power = exponent;
    while (power > 0){
        if (power % 2 == 1){
            result = (u64(result) * factor) % modulus;
        }
        factor = (u64(factor) * factor) % modulus;
        power = power / 2;
    }
    return result;
}

class Crew {
private:
    char* name;
    Member** members;
    int count;
    int capacity;
    static int liveCount;
    Crew& operator=(const Crew&);
public:
    Crew(const char* name);
    Crew(const Crew& other);
    ~Crew();
    const char* getName() const;
    int memberCount() const;
    int addMember(const Member& member);
    int addMember(const char* name, int skill);
    const Member* getMember(int index) const;
    Member* getMember(int index);
    int findMember(const char* name) const;
    int estimate(const Vault& vault) const;
    int execute(Vault& vault);
    static int live();
};
int Crew::liveCount = 0;
Crew::Crew(const char* name): name(new char[std::strlen(name) + 1]),members(new Member* [2]),count(0),capacity(2){
    std::strcpy(this->name, name);
    members[0] = nullptr;
    members[1] = nullptr;
    liveCount++;
}
Crew::Crew(const Crew& other): name(new char[std::strlen(other.name) + 1]),members(new Member* [other.capacity]),count(other.count),capacity(other.capacity){
    std::strcpy(name, other.name);
    for (int i = 0; i < capacity; i++){
        members[i] = nullptr;
    }
    for (int i = 0; i < count; i++){
        members[i] = new Member(*other.members[i]);
    }
    liveCount++;
}
Crew::~Crew(){
    for (int i = 0; i < count; i++){
        delete members[i];
    }
    delete[] members;
    delete[] name;
    liveCount--;
}
const char* Crew::getName() const{
    return name;
}
int Crew::memberCount() const{
    return count;
}
int Crew::live(){
    return liveCount;
}
const Member* Crew::getMember(int index) const{
    if (index < 0 || index >= count){
        return nullptr;
    }
    return members[index];
}
Member* Crew::getMember(int index){
    if (index < 0 || index >= count){
        return nullptr;
    }
    return members[index];
}
int Crew::findMember(const char* name) const{
    if (name == nullptr){
        return -1;
    }
    for (int i = 0; i < count; i++){
        if (std::strcmp(members[i]->getName(), name) == 0){
            return i;
        }
    }
    return -1;
}

int Crew::addMember(const Member& member){
    if (count >= 20) {
        return -1;
    }
    if (count == capacity){
        int newCapacity = capacity * 2;
        if (newCapacity > 20){
            newCapacity = 20;
        }
        Member** newMembers = new Member * [newCapacity];
        for (int i = 0; i < count; i++){
            newMembers[i] = members[i];
        }
        for (int i = count; i < newCapacity; i++){
            newMembers[i] = nullptr;
        }
        delete[] members;
        members = newMembers;
        capacity = newCapacity;
    }
    members[count] = new Member(member);
    int index = count;
    count++;
    return index;
}
int Crew::addMember(const char* name, int skill){
    if (count >= 20 || name == nullptr){
        return -1;
    }
    Member member(name, skill);
    return addMember(member);
}
int Crew::estimate(const Vault& vault) const{
    if (count == 0){
        return -1;
    }
    Member** tempMembers = new Member * [count];
    int* finish = new int[count];
    for (int i = 0; i < count; i++){
        tempMembers[i] = new Member(*members[i]);
        finish[i] = 0;
    }
    int totalLocks = vault.lockCount();
    const Lock** locksToDo = new const Lock * [totalLocks];
    int lockCount = 0;
    for (int i = 0; i < totalLocks; i++){
        const Lock* lock = vault.getLock(i);
        if (!lock->isCracked()){
            locksToDo[lockCount] = lock;
            lockCount++;
        }
    }
    for (int i = 0; i < lockCount - 1; i++){
        for (int j = 0; j < lockCount - i - 1; j++){
            if (locksToDo[j]->getBaseTime() <
                locksToDo[j + 1]->getBaseTime()){
                const Lock* temp = locksToDo[j];
                locksToDo[j] = locksToDo[j + 1];
                locksToDo[j + 1] = temp;
            }
        }
    }
    int penalty = 0;
    if (count > 3){
        penalty = 1;
    }
    for (int i = 0; i < lockCount; i++){
        int bestMember = 0;
        int bestFinish = finish[0] +
            tempMembers[0]->timeFor(*locksToDo[i]) + penalty;
        for (int j = 1; j < count; j++){
            int candidate = finish[j] +
                tempMembers[j]->timeFor(*locksToDo[i]) + penalty;
            if (candidate < bestFinish){
                bestFinish = candidate;
                bestMember = j;
            }
        }
        finish[bestMember] = bestFinish;
        tempMembers[bestMember]->work(*locksToDo[i]);
    }
    int duration = 1;
    for (int i = 0; i < count; i++){
        if (finish[i] > duration){
            duration = finish[i];
        }
    }
    for (int i = 0; i < count; i++){
        delete tempMembers[i];
    }
    delete[] tempMembers;
    delete[] finish;
    delete[] locksToDo;
    return duration;
}









