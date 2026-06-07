#include <bits/stdc++.h>
#include <cstdint>



#define LINE_HEADER(c, addr) \
    std::setfill(' ') << std::setw(8) << std::dec \
    << (c) << ':' << std::setfill('0') << std::setw(4) << std::hex << (addr) << ':' << std::dec

#define PRINT_MOVI(c, addr, rx, i) \
     "mov_r" << (int) (rx) << ',' << std::hex << std::setfill('0') << std::setw(8) << (int) (i)

#define PRINT_MOVR(c, addr, rx, ry) \
     "mov_r" << (int) (rx) << ",r" << (int) (ry)

#define PRINT_MOVRM(c, addr, rx, ry) \
     "mov_r" << (int) (rx) << ",[r" << (int) (ry) << ']'

#define PRINT_MOVMR(c, addr, rx, ry) \
     "mov_[r" << (int) (rx) << "],r" << (int) (ry)

#define PRINT_CMP(c, addr, rx, ry) \
     "cmp_r" << (int) (rx) << "<=>r" << (int) (ry)

#define PRINT_JMP(c, addr, jmp) \
    "jmp_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JG(c, addr, jmp) \
    "jg_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JL(c, addr, jmp) \
    "jl_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_JE(c, addr, jmp) \
    "je_" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)(jmp)

#define PRINT_ADD(c, addr, rx, ry) \
     "add_r" << (int) (rx) << "+=r" << (int) (ry)

#define PRINT_SUB(c, addr, rx, ry) \
     "sub_r" << (int) (rx) << "-=r" << (int) (ry)

#define PRINT_AND(c, addr, rx, ry) \
     "and_r" << (int) (rx) << "&=r" << (int) (ry)

#define PRINT_OR(c, addr, rx, ry) \
     "or_r" << (int) (rx) << "|=r" << (int) (ry)

#define PRINT_XOR(c, addr, rx, ry) \
     "xor_r" << (int) (rx) << "^=r" << (int) (ry)

#define PRINT_SAL(c, addr, rx, i) \
     "sal_r" << (int) (rx) << "<<=" << (int) (i)

#define PRINT_SAR(c, addr, rx, i) \
     "sar_r" << (int) (rx) << ">>=" << (int) (i)

#define PRINT_REG(rx)\
    'r' << std::dec << (rx) << '=' << std::hex << std::setfill('0') << std::setw(8)

using namespace std;

namespace PQP {

    enum OPCODE : uint8_t {
        MOVI,               // MOV Rx   ,   i16      SIGN-EXTENDED
        MOVR,               // MOV Rx   ,   Ry
        MOVRM,              // MOV Rx   ,   [Ry]
        MOVMR,              // MOV [Rx] ,   [Ry]
        CMP,                // CMP Rx   ,   Ry
        JMP,                // JMP          i16      SIGN-EXTENDED + 4
        JG,                 // JG           i16      SIGN-EXTENDED + 4
        JL,                 // JL           i16      SIGN-EXTENDED + 4
        JE,                 // JE           i16      SIGN-EXTENDED + 4
        ADD,                // ADD Rx   ,   Ry
        SUB,                // SUB Rx   ,   Ry
        AND,                // AND Rx   ,   Ry
        OR,                 // OR  Rx   ,   Ry
        XOR,                // XOR Rx   ,   Ry
        SAL,                // Rx           i5       
        SAR                 // Rx           i5       
    };


    struct INSTRUCTION {
        OPCODE opcode;
        uint8_t Rx;
        uint8_t Ry;
        int16_t i16;
        uint8_t i5;

        INSTRUCTION (uint8_t raw_instruction [4]){
            this->opcode = (OPCODE) raw_instruction[0];
            this->Rx  = raw_instruction[1] >> 4;
            this->Ry  = raw_instruction[1] & 0x0F;
            this->i16 = (int16_t) (*((uint16_t*) &raw_instruction[2]));
            this->i5  = raw_instruction[3] & 0x0F;
        }

    };

    static inline bool get_raw_instruction (ifstream& input, uint8_t raw_instruction [4]){
        
        for (int i = 0; i < 4; i++){
            uint32_t n;
            if (!(input >> std::hex >> n)) return false;
            raw_instruction[i] = (uint8_t) n;
        }

        return true;
    }

    

    uint8_t pqp_mem   [256];
    uint32_t pqp_addr_counter [256];
    int32_t pqp_regs  [16] ;
    uint8_t pqp_flags =   0;
    uint32_t PC       =   0;

    map<uint32_t, pair<uint32_t, string>> mapped_instructions;


    void load_memory (ifstream& input){

        uint8_t raw_instruction [4];

        cout << "Loading PQP memory (256 Bytes)..." << endl;
        int i;
        for (i = 0; i < 256; i+=4){
            if (!get_raw_instruction(input, raw_instruction)) break;
            pqp_mem[i]   = raw_instruction[0];
            pqp_mem[i+1] = raw_instruction[1];
            pqp_mem[i+2] = raw_instruction[2];
            pqp_mem[i+3] = raw_instruction[3];
        }

        while (i++ < 256){
            pqp_mem[i] = 0;
        }

        cout << "Memory loaded! Address: 0x" << std::hex << (uint64_t) pqp_mem << endl;
    }

}

ifstream input_file;
ofstream output_file;


void jit (){

    using namespace PQP;
    while (PC < 256){
        
        INSTRUCTION i (&pqp_mem[PC]);
        switch (i.opcode) {
            case MOVI: {
                pqp_regs[i.Rx] = (int32_t) i.i16;
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else { 
                    stringstream s;
                    s << PRINT_MOVI (++pqp_addr_counter[PC], PC, i.Rx, i.i16);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case MOVR: {
                pqp_regs[i.Rx] = pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_MOVR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case MOVRM: {
                pqp_regs[i.Rx] = *((uint32_t*) (&pqp_mem[pqp_regs[i.Ry]]));
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_MOVRM(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case MOVMR: {
                *((uint32_t*) &pqp_mem[pqp_regs[i.Rx]]) = (uint32_t) pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_MOVMR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case CMP: {
                if (pqp_regs[i.Rx] < pqp_regs[i.Ry]) pqp_flags = 0b001;
                else if (pqp_regs[i.Rx] > pqp_regs[i.Ry]) pqp_flags = 0b010;
                else pqp_flags = 0b100;

                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_CMP(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case JMP: {
                uint32_t tmp = PC;
                uint32_t target = tmp + (int32_t)i.i16 + 4;
                PC += (int32_t) i.i16;
                if (mapped_instructions.count(tmp)) {
                    mapped_instructions[tmp].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_JMP(++pqp_addr_counter[tmp], tmp, target);
                    mapped_instructions[tmp] = {1, s.str()};
                }
                break;
            }
            case JG: {
                uint32_t tmp = PC;
                uint32_t target = tmp + (int32_t)i.i16 + 4;
                if (pqp_flags & 0b010) PC += (int32_t) i.i16;
                if (mapped_instructions.count(tmp)) {
                    mapped_instructions[tmp].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_JG(++pqp_addr_counter[tmp], tmp, target);
                    mapped_instructions[tmp] = {1, s.str()};
                }
                break;
            }
            case JL: {
                uint32_t tmp = PC;
                uint32_t target = tmp + (int32_t)i.i16 + 4;
                if (pqp_flags & 0b001) PC += (int32_t) i.i16;
                if (mapped_instructions.count(tmp)) {
                    mapped_instructions[tmp].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_JL(++pqp_addr_counter[tmp], tmp, target);
                    mapped_instructions[tmp] = {1, s.str()};
                }
                break;
            }
            case JE: {
                uint32_t tmp = PC;
                uint32_t target = tmp + (int32_t)i.i16 + 4;
                if (pqp_flags & 0b100) PC += (int32_t) i.i16;
                if (mapped_instructions.count(tmp)) {
                    mapped_instructions[tmp].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_JE(++pqp_addr_counter[tmp], tmp, target);
                    mapped_instructions[tmp] = {1, s.str()};
                }
                break;
            }
            case ADD: {
                pqp_regs[i.Rx] += pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_ADD(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case SUB: {
                pqp_regs[i.Rx] -= pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_SUB(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case AND: {
                pqp_regs[i.Rx] &= pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_AND(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case OR: {
                pqp_regs[i.Rx] |= pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_OR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case XOR: {
                pqp_regs[i.Rx] ^= pqp_regs[i.Ry];
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_XOR(++pqp_addr_counter[PC], PC, i.Rx, i.Ry);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case SAL: {
                pqp_regs[i.Rx] <<= i.i5;
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_SAL(++pqp_addr_counter[PC], PC, i.Rx, i.i5);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
            case SAR:{
                pqp_regs[i.Rx] >>= i.i5;
                if (mapped_instructions.count(PC)) {
                    mapped_instructions[PC].first += 1;
                } else {
                    stringstream s;
                    s << PRINT_SAR(++pqp_addr_counter[PC], PC, i.Rx, i.i5);
                    mapped_instructions[PC] = {1, s.str()};
                }
                break;
            }
        
        }

        PC += 4;

    }

    mapped_instructions[PC] = {1, ""};

}


int main (int argc, char* argv[]){
    
    memset(PQP::pqp_mem, 0, 256);
    memset(PQP::pqp_regs, 0, 16 * sizeof(int));

    if (argc > 1) input_file = ifstream (argv[1]);
    if (argc > 2) output_file = ofstream (argv[2]);

    PQP::load_memory(input_file);

    jit();


    for (auto& [addr, counter] : PQP::mapped_instructions){
        if (addr < 256) output_file << LINE_HEADER(counter.first, addr) << counter.second << endl;
        else{
            output_file << "       -:" << std::hex << std::setfill('0') << std::setw(4) << (uint16_t)addr << ":exit" << endl;
        }
        
    }

    output_file << '\n';

    for (int j = 0; j < 15; j++){
           output_file << PRINT_REG(j) << PQP::pqp_regs[j] << ',';
       }

       output_file << PRINT_REG(15) << PQP::pqp_regs[15] << endl;


    return 0;
}