#include <iostream>
#include <cstdint>
using namespace std;

class Registers {
    int8_t r[8];

    public:
        void initialize() {
            for (int i = 0; i < 8; i++) {
                r[i] = 0;
            }
        }

        int8_t getRegister(int index) {
            if (index >= 0 && index < 8) {
                return r[index];
            } else {
                throw out_of_range("Invalid register index");
            }
        }

        void setRegister(int index, int8_t value) {
            if (index >= 0 && index < 8) {
                r[index] = value;
            } else {
                throw out_of_range("Invalid register index");
            }
        }

};

class Memory {
    int8_t mem[64];

    public:
        Memory() {
            for (int i = 0; i < 64; i++) {
                mem[i] = (int8_t)0;
            }
        }

        int8_t read(int address) {
        if (address < 0 || address >= 64) throw out_of_range("Invalid Address");
        return mem[address];
        }

        void write(int address, int8_t value) {
            if (address < 0 || address >= 64) throw out_of_range("Invalid Address");
            mem[address] = value;
        }
};

class Flags {

    public:
    bool Z, C, O, U;
        Flags(){
            initFlags();
        }
        void initFlags() {
            Z = C = O = U = false;
        }
};

class Stack {
    int8_t stack[8];
    int top = -1;

    public:
        Stack() {
            for (int i = 0; i < 8; i++) {
                stack[i] = 0;
            }
        }


        void push(int8_t value) {
            if (top < 7) {
                stack[++top] = value;
            } else {
                throw overflow_error("Stack overflow");
            }
        }

        int8_t pop() {
            if (top >= 0) {
                return stack[top--];
            } else {
                throw underflow_error("Stack underflow");
            }
        }

        int getCount() {
            return top + 1;
        }
};

class VirtualMachine {
    private:
        Registers registers;
        Memory memory;
        Flags flags;
        Stack stack;

        uint8_t programCounter;
        uint8_t stackIndex;

    public:
        VirtualMachine() {
        reset();
        }

        void reset() {
            registers.initialize();
            flags.initFlags();
            programCounter = 0;
            stackIndex = 0;
        }

        int8_t getReg(int index) { 
            return registers.getRegister(index); 
        }
        void setReg(int index, int8_t value) { 
            registers.setRegister(index, value); 
        }
    
        int8_t readMem(int address) { 
            return memory.read(address); 
        }
        void writeMem(int address, int8_t value) { 
            memory.write(address, value); 
        }

        void setPC(uint8_t val) { 
            programCounter = val; 
        }
        uint8_t getPC() { 
            return programCounter; 
        }

        void pushStack(int8_t value) {
            stack.push(value);
            stackIndex = stack.getCount(); 
        }

        int8_t popStack() {
            int8_t val = stack.pop();
            stackIndex = stack.getCount(); 
            return val;
        }

        void dumpState() {
            cout << "=========================================\n";
            cout << "PC: " << (int)programCounter << " | SI: " << (int)stackIndex << "\n";
            cout << "Flags -> Z: " << flags.Z << " C: " << flags.C 
                << " O: " << flags.O << " U: " << flags.U << "\n";
            cout << "Registers: ";
            for (int i = 0; i < 8; i++) {
                cout << "R" << i << "=" << (int)registers.getRegister(i) << " ";
            }
            cout << "\n=========================================\n";
        }
};

int main(){
try {
        VirtualMachine vm;
        cout << "--- Keadaan Awal VM ---" << endl;
        vm.dumpState();

        // 1. Uji Register: Set R1 kepada 42 dan R5 kepada -10
        vm.setReg(1, 42);
        vm.setReg(5, -10);

        // 2. Uji Memori: Tulis nilai R1 ke dalam alamat memori 20
        int8_t data = vm.getReg(1);
        vm.writeMem(20, data);

        // 3. Uji Stack: PUSH nilai R5 ke dalam Stack
        vm.pushStack(vm.getReg(5));
        
        cout << "--- Keadaan Selepas Manipulasi Asas ---" << endl;
        vm.dumpState();

        // 4. Uji POP Stack: Ambil balik data dari stack dan letak kat R2
        int8_t dataDulu = vm.popStack();
        vm.setReg(2, dataDulu);

        cout << "--- Keadaan Selepas POP Stack ---" << endl;
        vm.dumpState();

    } catch (const exception& e) {
        cerr << "Ralat dikesan: " << e.what() << endl;
    }

    return 0;
}