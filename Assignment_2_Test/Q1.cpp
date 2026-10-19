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





