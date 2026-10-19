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






