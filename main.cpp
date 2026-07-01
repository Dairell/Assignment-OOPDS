// =============================================================================
// CCP6124 Object-Oriented Programming and Data Structures
// Virtual Machine and Assembly Language Interpreter
// Tutorial: TT7L
// Group: 07
// File: TT07L_G07.cpp
// Description: Single-file implementation of a simplified virtual machine
//              with an assembly language runner/interpreter.
// Members:
//   - Emil Shadiq (252UC241Q3) — Custom data structures, CONCRETE INSTRUCTION CLASSES(Arithmetic, Shift, Load, Store, Push, Pop)
//   - Dairell Hannan (252UC24246) — Register classes, CONCRETE INSTRUCTION CLASSES(Input, Display, Mov, IncDec, Rotate, Reset)
//   - Arfa Mirza (252UC2425R) — Memory class, Instruction class hierarchy, Main
//   - Muhammad Irfan Zikry (252UC242RA) — CPU class, Runner class
// =============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <iomanip>

using namespace std;

// =============================================================================
// SECTION 1: CUSTOM DATA STRUCTURES
// Written by: [Emil Shadiq, 252UC241Q3]
// =============================================================================

// -----------------------------------------------------------------------------
// 1A. Custom Dynamic Array (replaces std::vector)
// Used to store lines from the .asm file and instruction objects.
// -----------------------------------------------------------------------------
template <typename T>
class MyList {
private:
    T*     data;
    int    capacity;
    int    size_;

    // Double the internal buffer when full
    void resize() {
        capacity *= 2;
        T* newData = new T[capacity];
        for (int i = 0; i < size_; i++)
            newData[i] = data[i];
        delete[] data;
        data = newData;
    }

public:
    // Constructor
    MyList() : capacity(8), size_(0) {
        data = new T[capacity];
    }

    // Copy constructor
    MyList(const MyList& other) : capacity(other.capacity), size_(other.size_) {
        data = new T[capacity];
        for (int i = 0; i < size_; i++)
            data[i] = other.data[i];
    }

    // Destructor
    ~MyList() {
        delete[] data;
    }

    // Add element to the end
    void push_back(const T& val) {
        if (size_ >= capacity) resize();
        data[size_++] = val;
    }

    // Access element by index
    T& operator[](int idx) {
        return data[idx];
    }

    const T& operator[](int idx) const {
        return data[idx];
    }

    // Return current number of elements
    int size() const { return size_; }

    // Check if empty
    bool empty() const { return size_ == 0; }

    // Remove all elements
    void clear() { size_ = 0; }
};


// -----------------------------------------------------------------------------
// 1B. Custom Stack (replaces std::stack)
// Used for the virtual machine's stack (PUSH/POP operations).
// Fixed capacity of 8 bytes as per assignment spec.
// -----------------------------------------------------------------------------
class MyStack {
private:
    signed char data[8]; // Stack holds up to 8 signed bytes
    int         top_;    // Index of the next free slot

public:
    // Constructor — initialize stack to 0
    MyStack() : top_(0) {
        for (int i = 0; i < 8; i++) data[i] = 0;
    }

    // Push a value onto the stack
    // Returns false if stack is full
    bool push(signed char val) {
        if (top_ >= 8) return false;
        data[top_++] = val;
        return true;
    }

    // Pop a value from the stack
    // Returns false if stack is empty (crash condition)
    bool pop(signed char& out) {
        if (top_ <= 0) return false;
        out = data[--top_];
        return true;
    }

    // Peek at the top value without removing it
    signed char peek() const { return data[top_ - 1]; }

    // Return the current stack index (SI register mirrors this)
    int getIndex() const { return top_; }

    // Check if stack is empty
    bool empty() const { return top_ == 0; }

    // Check if stack is full
    bool full() const { return top_ >= 8; }

    // Get value at a specific index (for display purposes)
    signed char getAt(int i) const { return data[i]; }
};


// -----------------------------------------------------------------------------
// 1C. Custom Queue (replaces std::queue)
// Used to optionally store the program instructions for execution order.
// -----------------------------------------------------------------------------
template <typename T>
class MyQueue {
private:
    T*   data;
    int  capacity;
    int  front_;
    int  back_;
    int  size_;

    void resize() {
        int newCap = capacity * 2;
        T* newData = new T[newCap];
        for (int i = 0; i < size_; i++)
            newData[i] = data[(front_ + i) % capacity];
        delete[] data;
        data    = newData;
        front_  = 0;
        back_   = size_;
        capacity = newCap;
    }

public:
    MyQueue() : capacity(8), front_(0), back_(0), size_(0) {
        data = new T[capacity];
    }

    ~MyQueue() { delete[] data; }

    void enqueue(const T& val) {
        if (size_ >= capacity) resize();
        data[back_] = val;
        back_ = (back_ + 1) % capacity;
        size_++;
    }

    T dequeue() {
        T val = data[front_];
        front_ = (front_ + 1) % capacity;
        size_--;
        return val;
    }

    bool empty() const { return size_ == 0; }
    int  size()  const { return size_; }
};


// =============================================================================
// SECTION 2: REGISTER CLASSES
// Written by: [Dairell Hannan, 252UC24246]
// =============================================================================

// -----------------------------------------------------------------------------
// 2A. Base Register class
// Encapsulates an 8-bit signed value. All members private; access via getters/setters.
// -----------------------------------------------------------------------------
class Register {
private:
    signed char value; // Stores a signed byte (-128 to 127)

public:
    // Constructor — default value 0
    Register() : value(0) {}

    // Getter
    signed char getValue() const { return value; }

    // Setter — stores the raw signed byte
    void setValue(signed char v) { value = v; }

    // Reset to zero
    void reset() { value = 0; }
};


// -----------------------------------------------------------------------------
// 2B. GeneralRegister — Derived from Register
// Represents one of R0–R7. No extra behaviour needed beyond the base class,
// but the inheritance relationship is required by the assignment OOP spec.
// -----------------------------------------------------------------------------
class GeneralRegister : public Register {
private:
    int id; // Register number (0–7)

public:
    GeneralRegister() : Register(), id(0) {}

    void setId(int i) { id = i; }
    int  getId() const { return id; }
};


// -----------------------------------------------------------------------------
// 2C. FlagRegister — Manages the 4 flag bits (CF, OF, UF, ZF)
// Aggregated by the CPU class.
// -----------------------------------------------------------------------------
class FlagRegister {
private:
    int OF; // Overflow Flag  — set when result > 127
    int UF; // Underflow Flag — set when result < -128
    int CF; // Carry Flag     — set when result exceeds 8-bit capacity
    int ZF; // Zero Flag      — set when result == 0

public:
    FlagRegister() : OF(0), UF(0), CF(0), ZF(0) {}

    // --- Getters ---
    int getOF() const { return OF; }
    int getUF() const { return UF; }
    int getCF() const { return CF; }
    int getZF() const { return ZF; }

    // --- Setters ---
    void setOF(int v) { OF = v; }
    void setUF(int v) { UF = v; }
    void setCF(int v) { CF = v; }
    void setZF(int v) { ZF = v; }

    // Reset a named flag to 0
    // Returns false if flag name is unrecognised
    bool resetFlag(const string& name) {
        if      (name == "OF") { OF = 0; return true; }
        else if (name == "UF") { UF = 0; return true; }
        else if (name == "CF") { CF = 0; return true; }
        else if (name == "ZF") { ZF = 0; return true; }
        return false;
    }

    // Update all flags based on a computed integer result
    // 'rawResult' is the untruncated arithmetic result (may exceed 8 bits)
    void updateFlags(int rawResult) {
        // Zero flag
        ZF = (rawResult == 0) ? 1 : 0;

        // Overflow (positive side)
        OF = (rawResult > 127) ? 1 : 0;

        // Underflow (negative side)
        UF = (rawResult < -128) ? 1 : 0;

        // Carry: result doesn't fit in 8 bits
        CF = (rawResult > 127 || rawResult < -128) ? 1 : 0;
    }
};


// =============================================================================
// SECTION 3: MEMORY CLASS
// Written by: [Arfa Mirza, 252UC2425R]
// Description: Manages 64 bytes of signed memory (addresses 0–63).
//              Composed inside the CPU class.
// =============================================================================
class Memory {
private:
    signed char mem[64]; // 64 signed bytes

public:
    // Constructor — initialise all bytes to 0
    Memory() {
        for (int i = 0; i < 64; i++) mem[i] = 0;
    }

    // Read a byte from memory; returns false for out-of-range address
    bool read(int address, signed char& out) const {
        if (address < 0 || address > 63) return false;
        out = mem[address];
        return true;
    }

    // Write a byte to memory; returns false for out-of-range address
    bool write(int address, signed char val) {
        if (address < 0 || address > 63) return false;
        mem[address] = val;
        return true;
    }

    // Direct read for display (no error check needed)
    signed char getAt(int address) const { return mem[address]; }
};


// =============================================================================
// SECTION 4: INSTRUCTION CLASS HIERARCHY
// Written by: [Arfa Mirza, 252UC2425R]
// Description: Abstract base class 'Instruction' with virtual execute().
//              Derived classes implement specific behaviours (polymorphism).
// =============================================================================

// Forward-declare CPU so Instruction subclasses can reference it
class CPU;

// -----------------------------------------------------------------------------
// 4A. Abstract base class — Instruction
// -----------------------------------------------------------------------------
class Instruction {
protected:
    string opcode; // e.g. "MOV", "ADD", "PUSH"
    string arg1;   // First operand  (destination)
    string arg2;   // Second operand (source), may be empty

public:
    Instruction(const string& op, const string& a1, const string& a2)
        : opcode(op), arg1(a1), arg2(a2) {}

    virtual ~Instruction() {}

    // Pure virtual — each subclass must implement execution logic
    virtual void execute(CPU& cpu) = 0;

    // Getters
    string getOpcode() const { return opcode; }
    string getArg1()   const { return arg1;   }
    string getArg2()   const { return arg2;   }
};


// =============================================================================
// SECTION 5: CPU CLASS
// Written by: [Muhammad Irfan Zikry, 252UC242RA]
// Description: Central class that owns Memory (composition), aggregates
//              FlagRegister, GeneralRegisters, PC, SI, and the Stack.
//              All instruction execute() methods operate through CPU's public API.
// =============================================================================
class CPU {
private:
    GeneralRegister registers[8]; // R0–R7 (composition)
    FlagRegister    flags;        // Flag bits (aggregation)
    Memory          memory;       // 64-byte memory (composition)
    MyStack         stack;        // VM stack — 8 bytes (composition)
    int             PC;           // Program Counter
    int             SI;           // Stack Index (mirrors stack depth)

    // Output log for DISPLAY instruction results (shown in output window)
    MyList<int> displayLog;

public:
    CPU() : PC(0), SI(0) {
        for (int i = 0; i < 8; i++)
            registers[i].setId(i);
    }

    // Reset the CPU to initial state (for loading a new program)
    void reset() {
        for (int i = 0; i < 8; i++) registers[i].reset();
        PC = 0;
        SI = 0;
        flags = FlagRegister();
        memory = Memory();
        stack  = MyStack();
        displayLog.clear();
    }

    // --- Program Counter ---
    int  getPC() const   { return PC; }
    void incrementPC()   { PC++; }
    void setPC(int val)  { PC = val; }

    // --- Stack Index ---
    int  getSI() const  { return SI; }

    // --- Register Access ---
    // Parse "R0".."R7" → index 0..7; returns -1 if invalid
    int parseRegisterIndex(const string& name) const {
        if (name.size() == 2 && name[0] == 'R') {
            int idx = name[1] - '0';
            if (idx >= 0 && idx <= 7) return idx;
        }
        return -1;
    }

    // Get register value by index
    signed char getRegister(int idx) const {
        return registers[idx].getValue();
    }

    // Set register value by index (also updates flags)
    void setRegister(int idx, int rawResult) {
        flags.updateFlags(rawResult);
        // Truncate to signed byte range for storage
        signed char stored;
        if (rawResult > 127)       stored = 127;
        else if (rawResult < -128) stored = -128;
        else                        stored = (signed char)rawResult;
        registers[idx].setValue(stored);
    }

    // Set register value WITHOUT touching flags (e.g. for MOV immediate)
    void setRegisterRaw(int idx, signed char val) {
        registers[idx].setValue(val);
    }

    // --- Flag Access ---
    FlagRegister& getFlags() { return flags; }

    // --- Memory Access ---
    Memory& getMemory() { return memory; }

    // --- Stack Operations ---
    // Push a value; returns false if full
    bool stackPush(signed char val) {
        if (!stack.push(val)) return false;
        SI = stack.getIndex();
        return true;
    }

    // Pop a value; returns false if empty (triggers crash)
    bool stackPop(signed char& out) {
        if (!stack.pop(out)) return false;
        SI = stack.getIndex();
        return true;
    }

    // --- Display Log ---
    void logDisplay(int val) { displayLog.push_back(val); }

    // Print everything in the display log
    void printDisplayLog() const {
        for (int i = 0; i < displayLog.size(); i++)
            cout << displayLog[i] << endl;
    }

    // --- Dump (output the VM state in the required format) ---
    void dump(ostream& out) const {
        out << "#Begin#" << endl;

        // Registers line
        out << "#Registers#";
        for (int i = 0; i < 8; i++) {
            int val = (int)registers[i].getValue(); // signed value, e.g. -128..127
            if (val < 0) {
                out << "-" << setw(3) << setfill('0') << (val * -1) << "#";
            } else {
                out << setw(4) << setfill('0') << val << "#";
            }
        }
        out << endl;

        // Flags line
        out << "#Flags#"
            << "OF#" << flags.getOF() << "#"
            << "UF#" << flags.getUF() << "#"
            << "CF#" << flags.getCF() << "#"
            << "ZF#" << flags.getZF() << "#" << endl;

        // PC line
        out << "#PC#" << setw(4) << setfill('0') << PC << "#" << endl;

        // Memory block (64 bytes, 8 per row)
        out << "#Memory#" << endl;
        for (int row = 0; row < 8; row++) {
            out << "#";
            for (int col = 0; col < 8; col++) {
                int addr = row * 8 + col;
                int val  = (int)memory.getAt(addr); // signed value
                if (val < 0) {
                    out << "-" << setw(3) << setfill('0') << abs(val) << "#";
                } else {
                    out << setw(4) << setfill('0') << val << "#";
                }
            }
            out << endl;
        }

        out << "#End#" << endl;
    }
};


// =============================================================================
// SECTION 6: CONCRETE INSTRUCTION CLASSES
// Written by: [Emil Shadiq, 252UC241Q3, Dairell Hannan, 252UC24246]
// Each class is a derived Instruction implementing a specific opcode.
// Polymorphism: the Runner stores Instruction* pointers and calls execute().
// =============================================================================

// Helper: strip brackets from "[R1]" → "R1", or "[20]" → "20"
static string stripBrackets(const string& s) {
    if (s.size() >= 2 && s.front() == '[' && s.back() == ']')
        return s.substr(1, s.size() - 2);
    return s;
}

// Helper: check if a string is a valid integer literal (may have leading '-')
static bool isInteger(const string& s) {
    if (s.empty()) return false;
    int start = (s[0] == '-') ? 1 : 0;
    if (start == 1 && s.size() == 1) return false;
    for (int i = start; i < (int)s.size(); i++)
        if (!isdigit(s[i])) return false;
    return true;
}

// Helper: check if string is a register name "R0".."R7"
static bool isRegister(const string& s) {
    return (s.size() == 2 && s[0] == 'R' && s[1] >= '0' && s[1] <= '7');
}

// Helper: check if string is a bracketed register "[R0]".."[R7]"
static bool isBracketedRegister(const string& s) {
    return (s.size() == 4 && s[0] == '[' && s[1] == 'R'
            && s[2] >= '0' && s[2] <= '7' && s[3] == ']');
}

// Helper: check if string is a bracketed integer "[n]"
static bool isBracketedInteger(const string& s) {
    if (s.size() < 3 || s.front() != '[' || s.back() != ']') return false;
    return isInteger(s.substr(1, s.size() - 2));
}


// -----------------------------------------------------------------------------
// 6.1 — INPUT Instruction
// Reads a value from stdin and stores it in the destination register.
// Updates OF, UF, ZF flags.
// -----------------------------------------------------------------------------
class InputInstruction : public Instruction {
public:
    InputInstruction(const string& a1)
        : Instruction("INPUT", a1, "") {}

    void execute(CPU& cpu) override {
        int idx = cpu.parseRegisterIndex(arg1);
        if (idx < 0) {
            cerr << "ERROR: INPUT — invalid register '" << arg1 << "'" << endl;
            return;
        }

        int value = 0;
        bool valid = false;
        while (!valid) {
            cout << "?" << endl;
            cin >> value;
            if (cin.fail()) {
                cin.clear();
                cin.ignore(1000, '\n');
                cerr << "Invalid input — enter a value between -128 and 127." << endl;
            } else {
                valid = true;
            }
        }

        // Flag updates for out-of-range input
        if (value > 127)       cpu.getFlags().setOF(1);
        else if (value < -128) cpu.getFlags().setUF(1);
        else                   cpu.getFlags().setZF(value == 0 ? 1 : 0);

        // Clamp to signed byte range
        if (value > 127)       value = 127;
        else if (value < -128) value = -128;

        cpu.setRegisterRaw(idx, (signed char)value);
    }
};


// -----------------------------------------------------------------------------
// 6.2 — DISPLAY Instruction
// Prints the value in the source register to stdout.
// -----------------------------------------------------------------------------
class DisplayInstruction : public Instruction {
public:
    DisplayInstruction(const string& a1)
        : Instruction("DISPLAY", a1, "") {}

    void execute(CPU& cpu) override {
        int idx = cpu.parseRegisterIndex(arg1);
        if (idx < 0) {
            cerr << "ERROR: DISPLAY — invalid register '" << arg1 << "'" << endl;
            return;
        }
        int val = (int)cpu.getRegister(idx);
        cout << val << endl;
        cpu.logDisplay(val);
    }
};


// -----------------------------------------------------------------------------
// 6.3 — MOV Instruction
// Three modes:
//   MOV Rd, #imm    — load immediate value
//   MOV Rd, Rs      — copy register
//   MOV Rd, [Rs]    — load from memory address stored in Rs
// -----------------------------------------------------------------------------
class MovInstruction : public Instruction {
public:
    MovInstruction(const string& a1, const string& a2)
        : Instruction("MOV", a1, a2) {}

    void execute(CPU& cpu) override {
        int destIdx = cpu.parseRegisterIndex(arg1);
        if (destIdx < 0) {
            cerr << "ERROR: MOV — invalid destination '" << arg1 << "'" << endl;
            return;
        }

        if (isInteger(arg2)) {
            // MOV Rd, immediate
            int v = atoi(arg2.c_str());
            cpu.setRegisterRaw(destIdx, (signed char)v);

        } else if (isRegister(arg2)) {
            // MOV Rd, Rs
            int srcIdx = cpu.parseRegisterIndex(arg2);
            cpu.setRegisterRaw(destIdx, cpu.getRegister(srcIdx));

        } else if (isBracketedRegister(arg2)) {
            // MOV Rd, [Rs]  — register-indirect from memory
            string inner = stripBrackets(arg2);
            int srcIdx   = cpu.parseRegisterIndex(inner);
            int address  = (int)(unsigned char)cpu.getRegister(srcIdx);
            signed char memVal = 0;
            if (!cpu.getMemory().read(address, memVal)) {
                cerr << "ERROR: MOV — memory address out of range: " << address << endl;
                return;
            }
            cpu.setRegisterRaw(destIdx, memVal);

        } else {
            cerr << "ERROR: MOV — unrecognised source operand '" << arg2 << "'" << endl;
        }
    }
};


// -----------------------------------------------------------------------------
// 6.4 — Arithmetic base class (ADD, SUB, MUL, DIV)
// Derived from Instruction; subclasses only differ in the operation performed.
// -----------------------------------------------------------------------------
class ArithmeticInstruction : public Instruction {
public:
    ArithmeticInstruction(const string& op, const string& a1, const string& a2)
        : Instruction(op, a1, a2) {}

    void execute(CPU& cpu) override {
        int destIdx = cpu.parseRegisterIndex(arg1);
        if (destIdx < 0) {
            cerr << "ERROR: " << opcode << " — invalid destination '" << arg1 << "'" << endl;
            return;
        }

        int srcVal = 0;

        // Source can be a register or an immediate integer
        if (isRegister(arg2)) {
            int srcIdx = cpu.parseRegisterIndex(arg2);
            srcVal = (int)cpu.getRegister(srcIdx);
        } else if (isInteger(arg2)) {
            srcVal = atoi(arg2.c_str());
        } else {
            cerr << "ERROR: " << opcode << " — invalid source '" << arg2 << "'" << endl;
            return;
        }

        int destVal = (int)cpu.getRegister(destIdx);
        int result  = 0;

        if (opcode == "ADD") {
            result = destVal + srcVal;
        } else if (opcode == "SUB") {
            result = destVal - srcVal;
        } else if (opcode == "MUL") {
            result = destVal * srcVal;
        } else if (opcode == "DIV") {
            if (srcVal == 0) {
                cerr << "ERROR: DIV — division by zero" << endl;
                return;
            }
            result = destVal / srcVal;
        }

        cpu.setRegister(destIdx, result); // updates flags automatically
    }
};


// -----------------------------------------------------------------------------
// 6.5 — INC / DEC Instructions
// Adds or subtracts 1 from a register; updates flags.
// -----------------------------------------------------------------------------
class IncDecInstruction : public Instruction {
public:
    IncDecInstruction(const string& op, const string& a1)
        : Instruction(op, a1, "") {}

    void execute(CPU& cpu) override {
        int idx = cpu.parseRegisterIndex(arg1);
        if (idx < 0) {
            cerr << "ERROR: " << opcode << " — invalid register '" << arg1 << "'" << endl;
            return;
        }
        int result = (int)cpu.getRegister(idx) + (opcode == "INC" ? 1 : -1);
        cpu.setRegister(idx, result);
    }
};


// -----------------------------------------------------------------------------
// 6.6 — ROL / ROR Shift Instructions
// Convert to 8-bit binary representation, rotate, convert back.
// The spec says to implement your own algorithm.
// -----------------------------------------------------------------------------
class RotateInstruction : public Instruction {
public:
    RotateInstruction(const string& op, const string& a1, const string& a2)
        : Instruction(op, a1, a2) {}

    void execute(CPU& cpu) override {
        int idx = cpu.parseRegisterIndex(arg1);
        if (idx < 0) {
            cerr << "ERROR: " << opcode << " — invalid register '" << arg1 << "'" << endl;
            return;
        }
        if (!isInteger(arg2)) {
            cerr << "ERROR: " << opcode << " — count must be an integer" << endl;
            return;
        }

        int count = atoi(arg2.c_str()) % 8; // Normalise count to 0–7
        if (count < 0) count = (count + 8) % 8;

        // Get raw 8-bit unsigned representation
        unsigned char bits = (unsigned char)cpu.getRegister(idx);

        if (opcode == "ROL") {
            // Rotate left: MSB wraps to LSB
            bits = (bits << count) | (bits >> (8 - count));
        } else { // ROR
            // Rotate right: LSB wraps to MSB
            bits = (bits >> count) | (bits << (8 - count));
        }

        // Store back as signed char
        cpu.setRegisterRaw(idx, (signed char)bits);
    }
};


// -----------------------------------------------------------------------------
// 6.7 — SHL / SHR Shift Instructions
// Logical shift: vacated positions filled with 0.
// -----------------------------------------------------------------------------
class ShiftInstruction : public Instruction {
public:
    ShiftInstruction(const string& op, const string& a1, const string& a2)
        : Instruction(op, a1, a2) {}

    void execute(CPU& cpu) override {
        int idx = cpu.parseRegisterIndex(arg1);
        if (idx < 0) {
            cerr << "ERROR: " << opcode << " — invalid register '" << arg1 << "'" << endl;
            return;
        }
        if (!isInteger(arg2)) {
            cerr << "ERROR: " << opcode << " — count must be an integer" << endl;
            return;
        }

        int count = atoi(arg2.c_str());
        // Shifting 7 or more times yields 0 (spec says shifting 7 times → 0)
        if (count >= 8 || count <= -8) {
            cpu.setRegisterRaw(idx, (signed char)0);
            return;
        }

        unsigned char bits = (unsigned char)cpu.getRegister(idx);

        if (opcode == "SHL") {
            bits = (count >= 8) ? 0 : (bits << count);
        } else { // SHR
            bits = (count >= 8) ? 0 : (bits >> count);
        }

        cpu.setRegisterRaw(idx, (signed char)bits);
    }
};


// -----------------------------------------------------------------------------
// 6.8 — LOAD Instruction
// Two forms:
//   LOAD Rd, [addr]   — direct address load
//   LOAD Rd, [Rs]     — register-indirect load
// -----------------------------------------------------------------------------
class LoadInstruction : public Instruction {
public:
    LoadInstruction(const string& a1, const string& a2)
        : Instruction("LOAD", a1, a2) {}

    void execute(CPU& cpu) override {
        int destIdx = cpu.parseRegisterIndex(arg1);
        if (destIdx < 0) {
            cerr << "ERROR: LOAD — invalid destination register '" << arg1 << "'" << endl;
            return;
        }

        int address = -1;

        if (isBracketedInteger(arg2)) {
            // LOAD Rd, [imm]
            string inner = stripBrackets(arg2);
            address = atoi(inner.c_str());

        } else if (isBracketedRegister(arg2)) {
            // LOAD Rd, [Rs]
            string inner = stripBrackets(arg2);
            int srcIdx   = cpu.parseRegisterIndex(inner);
            address = (int)(unsigned char)cpu.getRegister(srcIdx);

        } else {
            cerr << "ERROR: LOAD — unrecognised source '" << arg2 << "'" << endl;
            return;
        }

        signed char val = 0;
        if (!cpu.getMemory().read(address, val)) {
            cerr << "ERROR: LOAD — address out of range: " << address << endl;
            return;
        }
        cpu.setRegisterRaw(destIdx, val);
    }
};


// -----------------------------------------------------------------------------
// 6.9 — STORE Instruction
// Two forms:
//   STORE Rd, addr    — store to direct address
//   STORE [Rd], Rs    — store to address in register
//
// NOTE: The assignment spec example "STORE 20, R3" stores R3 into address 20.
//       We also support "STORE R1, 43" and "STORE R1, [R2]".
// -----------------------------------------------------------------------------
class StoreInstruction : public Instruction {
public:
    StoreInstruction(const string& a1, const string& a2)
        : Instruction("STORE", a1, a2) {}

    void execute(CPU& cpu) override {
        // Form 1: STORE Rd, addr  (arg1=register, arg2=integer address)
        if (isRegister(arg1) && isInteger(arg2)) {
            int srcIdx  = cpu.parseRegisterIndex(arg1);
            int address = atoi(arg2.c_str());
            if (!cpu.getMemory().write(address, cpu.getRegister(srcIdx))) {
                cerr << "ERROR: STORE — address out of range: " << address << endl;
            }

        // Form 2: STORE Rd, [Rs]  (arg1=register, arg2=bracketed register)
        } else if (isRegister(arg1) && isBracketedRegister(arg2)) {
            int srcIdx  = cpu.parseRegisterIndex(arg1);
            string inner = stripBrackets(arg2);
            int addrIdx  = cpu.parseRegisterIndex(inner);
            int address  = (int)(unsigned char)cpu.getRegister(addrIdx);
            if (!cpu.getMemory().write(address, cpu.getRegister(srcIdx))) {
                cerr << "ERROR: STORE — address out of range: " << address << endl;
            }

        // Form 3: STORE addr, Rs  (from the spec example: STORE 20, R3)
        } else if (isInteger(arg1) && isRegister(arg2)) {
            int address = atoi(arg1.c_str());
            int srcIdx  = cpu.parseRegisterIndex(arg2);
            if (!cpu.getMemory().write(address, cpu.getRegister(srcIdx))) {
                cerr << "ERROR: STORE — address out of range: " << address << endl;
            }

        } else {
            cerr << "ERROR: STORE — unrecognised operand forms '"
                 << arg1 << "', '" << arg2 << "'" << endl;
        }
    }
};


// -----------------------------------------------------------------------------
// 6.10 — RESET Instruction
// Resets a named flag (CF, OF, UF, ZF) to 0.
// -----------------------------------------------------------------------------
class ResetInstruction : public Instruction {
public:
    ResetInstruction(const string& a1)
        : Instruction("RESET", a1, "") {}

    void execute(CPU& cpu) override {
        if (!cpu.getFlags().resetFlag(arg1)) {
            cerr << "ERROR: RESET — unknown flag '" << arg1 << "'" << endl;
        }
    }
};


// -----------------------------------------------------------------------------
// 6.11 — PUSH Instruction
// Pushes a register value onto the stack; increments SI.
// -----------------------------------------------------------------------------
class PushInstruction : public Instruction {
public:
    PushInstruction(const string& a1)
        : Instruction("PUSH", a1, "") {}

    void execute(CPU& cpu) override {
        int idx = cpu.parseRegisterIndex(arg1);
        if (idx < 0) {
            cerr << "ERROR: PUSH — invalid register '" << arg1 << "'" << endl;
            return;
        }
        if (!cpu.stackPush(cpu.getRegister(idx))) {
            cerr << "ERROR: PUSH — stack overflow" << endl;
        }
    }
};


// -----------------------------------------------------------------------------
// 6.12 — POP Instruction
// Pops the top stack value into a register; decrements SI.
// Crashes (exits) if the stack is empty.
// -----------------------------------------------------------------------------
class PopInstruction : public Instruction {
public:
    PopInstruction(const string& a1)
        : Instruction("POP", a1, "") {}

    void execute(CPU& cpu) override {
        int idx = cpu.parseRegisterIndex(arg1);
        if (idx < 0) {
            cerr << "ERROR: POP — invalid register '" << arg1 << "'" << endl;
            return;
        }
        signed char val = 0;
        if (!cpu.stackPop(val)) {
            cerr << "FATAL: POP from empty stack — system crash." << endl;
            exit(1); // As per spec: "Any attempt to pop from an empty stack will cause system to stop and crash."
        }
        cpu.setRegisterRaw(idx, val);
    }
};


// =============================================================================
// SECTION 7: RUNNER (INTERPRETER)
// Written by: [Muhammad Irfan Zikry, 252UC242RA]
// Description: Reads a .asm file, parses each line into Instruction objects,
//              stores them in a MyList<Instruction*>, and executes them via CPU.
// =============================================================================
class Runner {
private:
    CPU                  cpu;          // The virtual machine
    MyList<string>       sourceLines;  // Raw lines from the .asm file
    MyList<Instruction*> instructions; // Parsed instruction objects (polymorphic)
    MyQueue<string>      programQueue; // Queue of non-empty source lines

    // -------------------------------------------------------------------------
    // Trim leading/trailing whitespace from a string
    // -------------------------------------------------------------------------
    string trim(const string& s) const {
        int start = 0, end = (int)s.size() - 1;
        while (start <= end && isspace((unsigned char)s[start])) start++;
        while (end >= start && isspace((unsigned char)s[end]))   end--;
        if (start > end) return "";
        return s.substr(start, end - start + 1);
    }

    // -------------------------------------------------------------------------
    // Convert a string to uppercase in-place
    // -------------------------------------------------------------------------
    string toUpper(const string& s) const {
        string result = s;
        for (int i = 0; i < (int)result.size(); i++)
            result[i] = (char)toupper((unsigned char)result[i]);
        return result;
    }

    // -------------------------------------------------------------------------
    // Split a line into tokens separated by spaces and commas.
    // Returns the token list as a MyList<string>.
    // -------------------------------------------------------------------------
    MyList<string> tokenise(const string& line) const {
        MyList<string> tokens;
        string token;
        for (int i = 0; i < (int)line.size(); i++) {
            char c = line[i];
            if (c == ',' || c == ' ' || c == '\t') {
                string t = trim(token);
                if (!t.empty()) tokens.push_back(t);
                token.clear();
            } else {
                token += c;
            }
        }
        string t = trim(token);
        if (!t.empty()) tokens.push_back(t);
        return tokens;
    }

    // -------------------------------------------------------------------------
    // Parse a single trimmed line into an Instruction* (heap-allocated).
    // Returns nullptr and prints an error if the line is malformed.
    // -------------------------------------------------------------------------
    Instruction* parseLine(const string& rawLine, int lineNum) {
        string line = trim(rawLine);
        if (line.empty()) return nullptr; // skip blank lines

        // Skip full-line comments (starting with ';' or '//')
        if (line[0] == ';') return nullptr;
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/') return nullptr;

        // Strip inline trailing comments, e.g. "MOV R0, 1 ; set counter"
        size_t semiPos = line.find(';');
        if (semiPos != string::npos) line = trim(line.substr(0, semiPos));
        size_t slashPos = line.find("//");
        if (slashPos != string::npos) line = trim(line.substr(0, slashPos));
        if (line.empty()) return nullptr;

        MyList<string> tokens = tokenise(line);
        if (tokens.size() == 0) return nullptr;

        string op = toUpper(tokens[0]);

        // ---- Instructions with 0 additional operands ----
        if (op == "INPUT" || op == "DISPLAY" || op == "INC" || op == "DEC"
            || op == "PUSH" || op == "POP"   || op == "RESET") {
            if (tokens.size() != 2) {
                cerr << "ERROR line " << lineNum << ": '" << op
                     << "' requires exactly 1 operand." << endl;
                return nullptr;
            }
            string a1 = toUpper(tokens[1]);
            if (op == "INPUT")   return new InputInstruction(a1);
            if (op == "DISPLAY") return new DisplayInstruction(a1);
            if (op == "INC")     return new IncDecInstruction("INC", a1);
            if (op == "DEC")     return new IncDecInstruction("DEC", a1);
            if (op == "PUSH")    return new PushInstruction(a1);
            if (op == "POP")     return new PopInstruction(a1);
            if (op == "RESET")   return new ResetInstruction(a1);
        }

        // ---- Instructions with 2 operands ----
        if (tokens.size() != 3) {
            cerr << "ERROR line " << lineNum << ": '" << op
                 << "' requires exactly 2 operands (found "
                 << (tokens.size() - 1) << ")." << endl;
            return nullptr;
        }

        string a1 = toUpper(tokens[1]);
        // Preserve case for arg2 only if it starts with '[' (addresses/registers)
        // but uppercase the register name inside brackets
        string a2 = tokens[2];
        // Uppercase the whole arg2 for register matching
        a2 = toUpper(a2);

        if (op == "MOV")   return new MovInstruction(a1, a2);
        if (op == "ADD")   return new ArithmeticInstruction("ADD", a1, a2);
        if (op == "SUB")   return new ArithmeticInstruction("SUB", a1, a2);
        if (op == "MUL")   return new ArithmeticInstruction("MUL", a1, a2);
        if (op == "DIV")   return new ArithmeticInstruction("DIV", a1, a2);
        if (op == "ROL")   return new RotateInstruction("ROL", a1, a2);
        if (op == "ROR")   return new RotateInstruction("ROR", a1, a2);
        if (op == "SHL")   return new ShiftInstruction("SHL", a1, a2);
        if (op == "SHR")   return new ShiftInstruction("SHR", a1, a2);
        if (op == "LOAD")  return new LoadInstruction(a1, a2);
        if (op == "STORE") return new StoreInstruction(a1, a2);

        cerr << "ERROR line " << lineNum << ": Unknown opcode '" << op << "'" << endl;
        return nullptr;
    }

    // -------------------------------------------------------------------------
    // Free all heap-allocated Instruction objects
    // -------------------------------------------------------------------------
    void freeInstructions() {
        for (int i = 0; i < instructions.size(); i++) {
            delete instructions[i];
        }
        instructions.clear();
    }

public:
    Runner() {}

    ~Runner() { freeInstructions(); }

    // -------------------------------------------------------------------------
    // Load a .asm file from disk.
    // Returns true on success.
    // -------------------------------------------------------------------------
    bool loadFile(const string& filename) {
        ifstream file(filename.c_str());
        if (!file.is_open()) {
            cerr << "ERROR: Cannot open file '" << filename << "'" << endl;
            return false;
        }

        sourceLines.clear();
        freeInstructions();
        cpu.reset();

        string line;
        int lineNum = 0;

        while (getline(file, line)) {
            lineNum++;
            string trimmed = trim(line);
            if (trimmed.empty()) continue; // skip blank lines as per spec

            // Enqueue the line for processing
            programQueue.enqueue(trimmed);
            sourceLines.push_back(trimmed);
        }
        file.close();

        // Parse all queued lines into instruction objects
        int parseNum = 0;
        while (!programQueue.empty()) {
            parseNum++;
            string rawLine = programQueue.dequeue();
            Instruction* instr = parseLine(rawLine, parseNum);
            if (instr != nullptr) {
                instructions.push_back(instr);
            }
            // If nullptr, an error was printed but we continue loading
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // Execute all loaded instructions sequentially.
    // The PC increments after each instruction.
    // -------------------------------------------------------------------------
    void run() {
        cpu.setPC(0);
        int total = instructions.size();

        while (cpu.getPC() < total) {
            int pc = cpu.getPC();
            instructions[pc]->execute(cpu);
            cpu.incrementPC();
        }
    }

    // -------------------------------------------------------------------------
    // Write the VM state dump to a file and also print to stdout.
    // -------------------------------------------------------------------------
    void dumpToFile(const string& filename) {
        // Print to stdout
        cpu.dump(cout);

        // Write to file
        ofstream out(filename.c_str());
        if (!out.is_open()) {
            cerr << "ERROR: Cannot write output file '" << filename << "'" << endl;
            return;
        }
        cpu.dump(out);
        out.close();
        cout << "\n[Output also saved to '" << filename << "']" << endl;
    }

    // -------------------------------------------------------------------------
    // Convenience: run a file and dump results
    // -------------------------------------------------------------------------
    void runFile(const string& asmFile, const string& outFile) {
        if (!loadFile(asmFile)) return;
        run();
        dumpToFile(outFile);
    }
};


// =============================================================================
// SECTION 8: MAIN
// Written by: [Arfa Mirza, 252UC2425R]
// Usage: ./TT07L_G07 <sample.asm> [output.txt]
//   If no arguments given, defaults to "sample.asm" and "output.txt"
// =============================================================================
int main(int argc, char* argv[]) {
    string inputFile  = "sample.asm";
    string outputFile = "output.txt";


    if (argc >= 2) inputFile  = argv[1];
    if (argc >= 3) outputFile = argv[2];

    cout << "====================================" << endl;
    cout << "=== Virtual Machine Interpreter ===" << endl;
    cout << "Loading: " << inputFile << endl;
    cout << "Output : " << outputFile << endl;
    cout << "====================================" << endl;

    Runner runner;
    runner.runFile(inputFile, outputFile);

    return 0;
}
